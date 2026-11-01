#include "globals.hlsl"

struct PixelInput
{
	float2 texCoord : TEXCOORD0;
};

SamplerState bloomSampler : REGISTER_S(3, 0);
Texture2D<float4> sourceTexture : REGISTER_T(3, 1);

static const float Threshold = 1.0;
static const float Knee = 0.25;

// The upsamples add BLOOM_LEVEL_COUNT levels together, so this also normalizes them
static const float BloomIntensity = 0.2;

float2 SourceTexelSize()
{
	float2 size;
	sourceTexture.GetDimensions(size.x, size.y);
	return 1.0 / size;
}

float3 Tap(float2 texCoord, float2 texelSize, float x, float y)
{
	return sourceTexture.Sample(bloomSampler, texCoord + texelSize * float2(x, y)).rgb;
}

float3 ApplyThreshold(float3 color)
{
	const float brightness = max(color.r, max(color.g, color.b));
	const float soft = clamp(brightness - Threshold + Knee, 0.0, 2.0 * Knee);
	const float excess = max(soft * soft / (4.0 * Knee), brightness - Threshold);

	return color * excess / max(brightness, 0.0001);
}

// Thresholding the taps and not their average: small bright sources like particles
// would otherwise be diluted by their dark neighbours and never reach the threshold.
float3 PrefilterTap(float2 texCoord, float2 texelSize, float x, float y)
{
	return ApplyThreshold(Tap(texCoord, texelSize, x, y));
}

// 13 bilinear taps weighted as 5 overlapping 2x2 boxes (Jimenez, SIGGRAPH 2014)
#define SAMPLE_13_TAPS(TapFunction, uv, texel) ( \
	0.125   *  TapFunction(uv, texel,  0.0,  0.0) + \
	0.125   * (TapFunction(uv, texel, -1.0, -1.0) + TapFunction(uv, texel,  1.0, -1.0) + \
	           TapFunction(uv, texel, -1.0,  1.0) + TapFunction(uv, texel,  1.0,  1.0)) + \
	0.0625  * (TapFunction(uv, texel,  0.0, -2.0) + TapFunction(uv, texel, -2.0,  0.0) + \
	           TapFunction(uv, texel,  2.0,  0.0) + TapFunction(uv, texel,  0.0,  2.0)) + \
	0.03125 * (TapFunction(uv, texel, -2.0, -2.0) + TapFunction(uv, texel,  2.0, -2.0) + \
	           TapFunction(uv, texel, -2.0,  2.0) + TapFunction(uv, texel,  2.0,  2.0)) )

float4 PSPrefilter(PixelInput IN) : SV_Target
{
	const float2 texel = SourceTexelSize();
	return float4(SAMPLE_13_TAPS(PrefilterTap, IN.texCoord, texel), 1.0);
}

float4 PSDownsample(PixelInput IN) : SV_Target
{
	const float2 texel = SourceTexelSize();
	return float4(SAMPLE_13_TAPS(Tap, IN.texCoord, texel), 1.0);
}

float3 SampleTent(float2 uv)
{
	const float2 texel = SourceTexelSize();

	float3 color = Tap(uv, texel, 0.0, 0.0) * 4.0;
	color += Tap(uv, texel, -1.0,  0.0) * 2.0;
	color += Tap(uv, texel,  1.0,  0.0) * 2.0;
	color += Tap(uv, texel,  0.0, -1.0) * 2.0;
	color += Tap(uv, texel,  0.0,  1.0) * 2.0;
	color += Tap(uv, texel, -1.0, -1.0);
	color += Tap(uv, texel,  1.0, -1.0);
	color += Tap(uv, texel, -1.0,  1.0);
	color += Tap(uv, texel,  1.0,  1.0);

	return color / 16.0;
}

float4 PSUpsample(PixelInput IN) : SV_Target
{
	return float4(SampleTent(IN.texCoord), 1.0);
}

float4 PSComposite(PixelInput IN) : SV_Target
{
	return float4(BloomIntensity * SampleTent(IN.texCoord), 1.0);
}
