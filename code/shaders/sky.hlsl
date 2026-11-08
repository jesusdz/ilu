#include "globals.hlsl"

SamplerState skySampler : REGISTER_S(3, 0);
Texture2D<float4> skyTexture : REGISTER_T(3, 1);

struct VertexInput
{
	float3 position : POSITION;
	float2 texCoord : TEXCOORD0;
};

struct Interpolators
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
};

struct PixelOutput
{
	float4 color : SV_Target;
};

typedef Interpolators VertexOutput;
typedef Interpolators PixelInput;

VertexOutput VSMain(VertexInput IN, uint instanceID : SV_InstanceID)
{
	float2 texCoord = IN.texCoord.xy;

	VertexOutput OUT;
	OUT.position = float4(IN.position, 1.0);
	OUT.texCoord = texCoord;
	return OUT;
}

#define PI 3.14159265359

PixelOutput PSMain(PixelInput IN)
{
	float3 frustumCornerA = globals.cameraFrustumTopLeft.xyz;
	float3 frustumCornerB = globals.cameraFrustumBottomRight.xyz;
	float3 lerpFactor = float3(IN.texCoord, 0.0); // Z is constant in the near plane
	float4 pixelDirView = float4(lerp(frustumCornerA, frustumCornerB, lerpFactor), 0.0);
	float3 pixelDirWorld = normalize(mul(globals.cameraViewInv, pixelDirView).xyz);

#if 1

	float yaw = atan2(pixelDirWorld.z, pixelDirWorld.x); // [-PI, PI]
	float pitch = asin(pixelDirWorld.y); // [-PI/2, PI/2]

	float yawNormalized = (yaw / PI) * 0.5 + 0.5;
	float pitchNormalized = 1.0 - pitch * 2.0 / PI;

	float skyDarkening = lerp(0.2, 1.0, pitchNormalized * pitchNormalized * pitchNormalized);
	float2 skyTexCoord = float2(yawNormalized, pitchNormalized);
	float3 skyTexColor = skyTexture.SampleLevel(skySampler, skyTexCoord, 0.0).rgb;
	float3 skyColor = skyTexColor * skyDarkening;

	float3 groundColor = 0.05.xxx;
	float groundFactor = smoothstep(1.0, 1.02, skyTexCoord.y);
	float3 backgroundColor = lerp(skyColor, groundColor, groundFactor);

#else

	float3 gridColor = float3(1.0, 1.0, 1.0);
	float gridAlpha = 0.0;
	float finalDepth = 0.0;

	// view direction to plane intersection
	float3 d = pixelDirWorld;
	float3 o = globals.eyePosition.xyz;
	float  t = - o.y / pixelDirWorld.y;
	if (t > 0.0)
	{
		float3 p = o + t*d;

		float lineWidth = 0.005;

		float2 uv = p.xz;
		float2 duv = fwidth(uv);
		float2 drawWidth = clamp(lineWidth, duv, 0.5);
		float2 lineAA = duv * 1.5;
		float2 lineUV = 1.0 - abs(frac(uv)*2.0 - 1.0);
		float2 grid = smoothstep(drawWidth + lineAA, drawWidth - lineAA, lineUV);
		grid *= saturate(lineWidth / drawWidth);
		grid = lerp(grid, lineWidth, saturate(duv * 2.0 - 1.0));
		gridAlpha = lerp(grid.x, 1.0, grid.y);
		if (gridAlpha == 1.0) gridAlpha = 0.0;

		gridColor = lerp( gridColor, float3(1.0, 0.5, 0.3), abs(p.x) < drawWidth.x ? 1.0 : 0.0 );
		gridColor = lerp( gridColor, float3(0.3, 0.5, 1.0), abs(p.z) < drawWidth.y ? 1.0 : 0.0 );
		gridAlpha *= abs(uv.x) < drawWidth.x || abs(uv.y) < drawWidth.y ? 4.0 : 1.0;

		float4 projectedPos = mul(mul(globals.cameraProj, globals.cameraView), float4(p,1));
		finalDepth = projectedPos.z / projectedPos.w;
	}

#endif

	// Only background
	float4 finalColor = float4(backgroundColor, 1.0);

	// Only grid
	//float finalDepth = 0.0;
	//float4 finalColor = float4(gridColor, gridAlpha);

	// Grid and background (wrong composition though)
	//float3 finalRGB = lerp(backgroundColor, gridColor, gridAlpha);
	//float4 finalColor = float4(finalRGB, 1.0);

	PixelOutput OUT;
	OUT.color = finalColor;
	return OUT;
}

