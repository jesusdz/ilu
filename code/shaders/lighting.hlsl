#ifndef LIGHTING_HLSL
#define LIGHTING_HLSL

#include "globals.hlsl"

uint LightCellIndex(float2 pixelCoord)
{
	const uint2 lastCell = max(globals.lightGridSize, uint2(1, 1)) - uint2(1, 1);
	const uint2 cell = min(uint2(pixelCoord) / LIGHT_TILE_SIZE, lastCell);
	return cell.y * globals.lightGridSize.x + cell.x;
}

uint4 LightCellMask(float2 pixelCoord)
{
	return lightGrid.Load4(LightCellIndex(pixelCoord) * LIGHT_CELL_STRIDE);
}

SLight GetLight(uint lightIndex)
{
	return lights.Load<SLight>(lightIndex * sizeof(SLight));
}

float LightAttenuation(float dist, float radius)
{
	float res = 1.0 - saturate(dist / radius);
	res = res * res;
	res = res * res;
	return res;
}

// Unlit sprites: no normal to work with, so the light only falls off with distance.
float3 AccumulateLights2D(float3 positionWs, float4 pixelCoord)
{
	positionWs.z *= 2; // Hacky tweak to have background layers darker

	float3 res = float3(0.0, 0.0, 0.0);

	uint4 mask = LightCellMask(pixelCoord.xy);
	[unroll] for (uint w = 0; w < 4; ++w)
	{
		uint bits = mask[w];
		while (bits)
		{
			const uint bit = firstbitlow(bits);
			bits &= bits - 1;

			const SLight light = GetLight(w * 32 + bit);
			const float dist = length(light.positionWs - positionWs);
			res += light.color * light.intensity * LightAttenuation(dist, light.radius);
		}
	}

	return res;
}

// Lambert only. The sun keeps its own specular term in the 3D shader.
float3 AccumulateLights3D(float3 positionWs, float3 N, float4 pixelCoord)
{
	float3 res = float3(0.0, 0.0, 0.0);

	uint4 mask = LightCellMask(pixelCoord.xy);
	[unroll] for (uint w = 0; w < 4; ++w)
	{
		uint bits = mask[w];
		while (bits)
		{
			const uint bit = firstbitlow(bits);
			bits &= bits - 1;

			const SLight light = GetLight(w * 32 + bit);
			const float3 lightVector = light.positionWs - positionWs;
			const float dist = length(lightVector);
			const float3 L = lightVector / max(dist, 0.0001);
			const float NoL = saturate(dot(L, N));
			res += light.color * light.intensity * NoL * LightAttenuation(dist, light.radius);
		}
	}

	return res;
}

#endif // LIGHTING_HLSL
