#include "globals.hlsl"

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
	float depth : SV_Depth;
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

		gridColor = lerp( gridColor, float3(1.0, 0.5, 0.3), abs(p.z) < drawWidth.x ? 1.0 : 0.0 );
		gridColor = lerp( gridColor, float3(0.3, 0.5, 1.0), abs(p.x) < drawWidth.y ? 1.0 : 0.0 );
		gridAlpha *= abs(uv.x) < drawWidth.x || abs(uv.y) < drawWidth.y ? 4.0 : 1.0;

		// factor to hide the horizon glitch
		gridAlpha = gridAlpha == 1.0 && abs(dot(d,float3(0,1,0))) < 0.1 ? 0.0 : gridAlpha;

		float4 projectedPos = mul(mul(globals.cameraProj, globals.cameraView), float4(p,1));
		finalDepth = projectedPos.z / projectedPos.w;
	}

	// Only grid
	float4 finalColor = float4(gridColor, gridAlpha);

	PixelOutput OUT;
	OUT.color = finalColor;
	OUT.depth = finalDepth;
	return OUT;
}

