#include "globals.hlsl"

struct VertexInput
{
	float3 position : POSITION;
	float2 texCoord : TEXCOORD0;
	float4 color : COLOR0;
};

struct VertexOutput
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
	float4 color : COLOR0;
};

VertexOutput VSMain(VertexInput IN, uint instanceID : SV_InstanceID)
{
	VertexOutput OUT;
	OUT.position = mul(globals.cameraProj, mul(globals.cameraView, float4(IN.position, 1.0)));
	OUT.texCoord = IN.texCoord;
	OUT.color = IN.color;
	return OUT;
}

struct PixelInput
{
	float2 texCoord : TEXCOORD0;
	float4 color : COLOR0;
};

struct PixelOutput
{
	float4 color;
};

SamplerState debugDrawSampler : REGISTER_S(3, 0);
Texture2D<float4> debugDrawTexture : REGISTER_T(3, 1);

PixelOutput PSMain(PixelInput IN) : SV_Target
{
	PixelOutput OUT = (PixelOutput)0;
	OUT.color = IN.color * debugDrawTexture.Sample(debugDrawSampler, IN.texCoord);
	return OUT;
}

