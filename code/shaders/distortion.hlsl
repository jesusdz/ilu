#include "globals.hlsl"

struct PixelInput
{
	float2 texCoord : TEXCOORD0;
};

struct PixelOutput
{
	float4 color;
};

SamplerState imageSampler : REGISTER_S(3, 0);
Texture2D<float4> sceneTexture : REGISTER_T(3, 1);
Texture2D<float2> distortionTexture : REGISTER_T(3, 2);

// Bounds the accumulation of overlapping particles, in scene UV
static const float MaxDistortion = 0.05;

PixelOutput PSMain(PixelInput IN) : SV_Target
{
	float2 offset = distortionTexture.Sample(imageSampler, IN.texCoord);
	const float len = length(offset);
	offset *= min( 1.0, MaxDistortion / len );

	PixelOutput OUT = (PixelOutput)0;
	OUT.color = sceneTexture.Sample(imageSampler, IN.texCoord + offset);
	return OUT;
}

