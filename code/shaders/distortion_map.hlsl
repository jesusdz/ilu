#include "globals.hlsl"

struct VertexInput
{
	float3 position : POSITION;
	float2 texCoord : TEXCOORD0;
	float strength : TEXCOORD1;
};

struct VertexOutput
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
	float strength : TEXCOORD1;
};

VertexOutput VSMain(VertexInput IN, uint instanceID : SV_InstanceID)
{
	VertexOutput OUT;
	OUT.position = mul(globals.cameraProj, mul(globals.cameraView, float4(IN.position, 1.0)));
	OUT.texCoord = IN.texCoord;
	OUT.strength = IN.strength;
	return OUT;
}

struct PixelInput
{
	float2 texCoord : TEXCOORD0;
	float strength : TEXCOORD1;
};

struct PixelOutput
{
	float2 offset;
};

PixelOutput PSMain(PixelInput IN) : SV_Target
{
	// Lens profile: zero at the center and the rim, peaking halfway
	const float2 d = IN.texCoord * 2.0 - 1.0;
	const float r = length(d);
	const float2 quadOffset = -2.0 * IN.strength * d * saturate(1.0 - r);

	// From quad UV to scene UV, whatever the particle size on screen
	const float2 quadUvPerPixel = float2(ddx(IN.texCoord.x), ddy(IN.texCoord.y));
	const float2 sceneOffset = quadOffset / (quadUvPerPixel * float2(globals.sceneResolution));

	PixelOutput OUT;
	OUT.offset = sceneOffset;
	return OUT;
}

