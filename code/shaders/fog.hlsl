#include "globals.hlsl"

// Cheap pseudo-random value in [0, 1) for a 1D, 2D or 3D position. No trigonometry and no texture
// lookups, just a few multiplies plus a frac, so they are fine to call several times per pixel.
// These are hashes, not smooth noises: neighbouring positions are uncorrelated, so feed them
// floored cell coordinates and interpolate if you need smooth features.

#if 0
float Noise(float p)
{
	p = frac(p * 0.1031);
	p *= p + 33.33;
	return frac(p * (p + p));
}

float Noise(float2 p)
{
	p = frac(p * 0.3183099 + 0.1);
	p *= 17.0;
	return frac(p.x * p.y * (p.x + p.y));
}

float Noise(float3 p)
{
	p = frac(p * 0.3183099 + 0.1);
	p *= 17.0;
	return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}

#else
float Noise(float p)
{
	p = frac(p * 0.1031);
	p *= p + 33.33;
	return frac(p * (p + p));
}

float Noise(float2 p)
{
	float n = noise2D.Sample(pointSampler, p/32.0).x;
	return n;
	//float3 p3 = frac(p.xyx * 0.1031);
	//p3 += dot(p3, p3.yzx + 33.33);
	//return frac((p3.x + p3.y) * p3.z);
}

float Noise(float3 p)
{
	float3 p3 = frac(p * 0.1031);
	p3 += dot(p3, p3.zyx + 31.32); // .zyx, not .yzx: the latter makes the dot symmetric in x and y
	return frac((p3.x + p3.y) * p3.z);
}
#endif

// Value noise: hashes the four corners of the lattice cell holding p and blends them with a
// smoothstep fade, giving a continuous field whose features are one cell wide. The interpolation is
// what makes this usable as an fbm octave; summing raw hashes would only ever give white noise.
float ValueNoise(float2 p)
{
	float2 cell = floor(p);
	float2 f = frac(p);
	float2 w = f * f * (3.0 - 2.0 * f); // Fade with zero derivative at the cell edges, so no creases

	float a = Noise(cell);
	float b = Noise(cell + float2(1.0, 0.0));
	float c = Noise(cell + float2(0.0, 1.0));
	float d = Noise(cell + float2(1.0, 1.0));

	return lerp(lerp(a, b, w.x), lerp(c, d, w.x), w.y);
}

// Same thing over a volume: eight corners and a trilinear blend. Twice the hashes of the 2D version,
// so prefer scrolling the 2D one when a third dimension is only wanted to animate the field.
float ValueNoise(float3 p)
{
	float3 cell = floor(p);
	float3 f = frac(p);
	float3 w = f * f * (3.0 - 2.0 * f);

	float c000 = Noise(cell);
	float c100 = Noise(cell + float3(1.0, 0.0, 0.0));
	float c010 = Noise(cell + float3(0.0, 1.0, 0.0));
	float c110 = Noise(cell + float3(1.0, 1.0, 0.0));
	float c001 = Noise(cell + float3(0.0, 0.0, 1.0));
	float c101 = Noise(cell + float3(1.0, 0.0, 1.0));
	float c011 = Noise(cell + float3(0.0, 1.0, 1.0));
	float c111 = Noise(cell + float3(1.0, 1.0, 1.0));

	float z0 = lerp(lerp(c000, c100, w.x), lerp(c010, c110, w.x), w.y);
	float z1 = lerp(lerp(c001, c101, w.x), lerp(c011, c111, w.x), w.y);

	return lerp(z0, z1, w.z);
}

// Fractal brownian motion: octaves of value noise, each one doubling in frequency and halving in
// amplitude. Dividing by the total amplitude keeps the result in [0, 1) regardless of octave count.
#define FBM_OCTAVES 5

float Fbm(float2 p)
{
	float sum = 0.0;
	float amplitude = 0.5;
	float totalAmplitude = 0.0;

	for (int i = 0; i < FBM_OCTAVES; ++i)
	{
		sum += amplitude * ValueNoise(p);
		totalAmplitude += amplitude;

		// The offset shifts each octave off the previous one's lattice. Without it every octave lines
		// up at the world origin and leaves a visible knot there.
		p = p * 2.0 + float2(17.0, 11.0);
		amplitude *= 0.5;
	}

	return sum / totalAmplitude;
}

float Fbm(float3 p)
{
	float sum = 0.0;
	float amplitude = 0.5;
	float totalAmplitude = 0.0;

	for (int i = 0; i < FBM_OCTAVES; ++i)
	{
		sum += amplitude * ValueNoise(p);
		totalAmplitude += amplitude;

		p = p * 2.0 + float3(17.0, 11.0, 23.0);
		amplitude *= 0.5;
	}

	return sum / totalAmplitude;
}

// Fbm evolving over time. The spatial axes get the usual per-octave frequency doubling, but t is only
// shifted, never scaled, so every octave evolves at the same rate. Passing time as the third axis of
// the isotropic Fbm above instead makes the finest octave churn 2^(FBM_OCTAVES-1) times faster than
// the coarsest, and since the smoothstep fade stalls at every lattice plane that fast traversal reads
// as popping rather than as evolution.
float Fbm(float2 p, float t)
{
	float sum = 0.0;
	float amplitude = 0.5;
	float totalAmplitude = 0.0;

	for (int i = 0; i < FBM_OCTAVES; ++i)
	{
		sum += amplitude * ValueNoise(float3(p, t));
		totalAmplitude += amplitude;

		p = p * 2.0 + float2(17.0, 11.0);
		t += 23.0; // Shifted, not scaled, and large enough to land in an unrelated slab of the volume
		amplitude *= 0.5;
	}

	return sum / totalAmplitude;
}

struct VertexInput
{
	float3 position : POSITION;
	float2 texCoord : TEXCOORD0;
};

struct VertexOutput
{
	float4 position : SV_Position;
	float2 positionWs : POSITION0;
	float2 texCoord : TEXCOORD0;
};

VertexOutput VSMain(VertexInput IN, uint instanceID : SV_InstanceID)
{
	float2 texCoord = IN.texCoord;

	// The screen geometry covers the whole viewport, so lerping the camera frustum corners gives
	// the view space position seen by each pixel. The 2D camera is orthographic, so this mapping
	// is affine and interpolating it across the triangle is exact.
	float3 posVs = lerp(globals.cameraFrustumTopLeft.xyz, globals.cameraFrustumBottomRight.xyz, float3(texCoord, 0.0));

	// The 2D view matrix is a pure translation, so undoing it takes us to world space.
	float2 cameraTranslation = float2(globals.cameraView[0][3], globals.cameraView[1][3]);

	VertexOutput OUT;
	OUT.position = float4(IN.position.xy, 0.50, 1.0);
	OUT.positionWs = posVs.xy - cameraTranslation;
	OUT.texCoord = IN.texCoord;
	return OUT;
}

struct PixelInput
{
	float2 positionWs : POSITION0;
	float2 texCoord : TEXCOORD0;
};

struct PixelOutput
{
	float4 color;
};

PixelOutput PSMain(PixelInput IN) : SV_Target
{
	PixelOutput OUT = (PixelOutput)0;

	float time = 0.3 * globals.time;

	// The frustum corners are in world units and sceneResolution is in pixels, so their ratio is the
	// scene's pixels per world unit. That maps the world position onto the scene pixel grid, giving
	// one integer cell per rendered pixel at any camera zoom.
	float2 viewportSizeMeters = abs(globals.cameraFrustumTopLeft.xy - globals.cameraFrustumBottomRight.xy);
	float2 viewportSizePixels = float2(320, 180); //float2(globals.sceneResolution);
	//float pixelsPerMeter = 32.0;
	//float2 noiseInput = floor(IN.positionWs * pixelsPerMeter);
	float alpha = 1.0 / (2.0 * IN.positionWs.y * IN.positionWs.y);// * IN.positionWs.y);
	float layer1 = Fbm(IN.positionWs * float2(1.0, 2.0) + time * float2(0.5, -0.4)) * ( 0.2 * sin(globals.time) + 1.0 );
	float layer2 = Fbm(IN.positionWs * float2(1.0, 2.0) + time * float2(-0.5, -0.3));
	float fog = (layer1 + 0.5 * layer2);
	float fog2 = 2.0f * fog * fog;
	OUT.color = float4(fog2.xxx, alpha );
	return OUT;
}

