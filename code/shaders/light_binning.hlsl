#include "globals.hlsl"

RWByteAddressBuffer lightGridRW : REGISTER_U(3, 0);

// The frustum corners live in view space, so the lights are brought over there rather
// than reconstructing world-space cell bounds.
float3 LightPositionVs(SLight light)
{
	return mul(globals.cameraView, float4(light.positionWs, 1.0)).xyz;
}

// Orthographic: the cell is a view-space XY rectangle extruded along Z. Ignoring the Z
// extent only over-includes, and the shading pass does the exact distance test anyway.
bool LightTouchesCellOrtho(float3 lightVs, float radius, float2 cellMin, float2 cellMax)
{
	const float2 closest = clamp(lightVs.xy, cellMin, cellMax);
	const float2 delta = lightVs.xy - closest;
	return dot(delta, delta) < radius * radius;
}

// Perspective: the cell is a pyramid from the eye through its near-plane rectangle. All
// four side planes pass through the origin, so each one is just an inward normal.
bool LightTouchesCellPerspective(float3 lightVs, float radius, float2 cellMin, float2 cellMax, float nearZ)
{
	// Wholly between the eye and the near plane
	if (lightVs.z - radius > nearZ) {
		return false;
	}

	const float3 c00 = float3(cellMin.x, cellMin.y, nearZ);
	const float3 c10 = float3(cellMax.x, cellMin.y, nearZ);
	const float3 c11 = float3(cellMax.x, cellMax.y, nearZ);
	const float3 c01 = float3(cellMin.x, cellMax.y, nearZ);

	const float d0 = dot(normalize(cross(c10, c00)), lightVs);
	const float d1 = dot(normalize(cross(c11, c10)), lightVs);
	const float d2 = dot(normalize(cross(c01, c11)), lightVs);
	const float d3 = dot(normalize(cross(c00, c01)), lightVs);

	return min(min(d0, d1), min(d2, d3)) > -radius;
}

[numthreads(8, 8, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
	const uint2 cell = dtid.xy;
	if (cell.x >= globals.lightGridSize.x || cell.y >= globals.lightGridSize.y) {
		return;
	}

	const float3 cornerA = globals.cameraFrustumTopLeft.xyz;
	const float3 cornerB = globals.cameraFrustumBottomRight.xyz;

	const float2 cellSize = float2(LIGHT_TILE_SIZE, LIGHT_TILE_SIZE) / float2(globals.sceneResolution);
	const float2 t0 = saturate(float2(cell) * cellSize);
	const float2 t1 = saturate(float2(cell + uint2(1, 1)) * cellSize);

	const float2 corner0 = lerp(cornerA.xy, cornerB.xy, t0);
	const float2 corner1 = lerp(cornerA.xy, cornerB.xy, t1);
	const float2 cellMin = min(corner0, corner1);
	const float2 cellMax = max(corner0, corner1);

	uint mask[4] = { 0, 0, 0, 0 };

	const uint lightCount = min(globals.lightCount, (uint)MAX_VISIBLE_LIGHTS);
	for (uint i = 0; i < lightCount; ++i)
	{
		const SLight light = lights.Load<SLight>(i * sizeof(SLight));
		const float3 lightVs = LightPositionVs(light);

		const bool touches = globals.projectionType == PROJECTION_ORTHOGRAPHIC
			? LightTouchesCellOrtho(lightVs, light.radius, cellMin, cellMax)
			: LightTouchesCellPerspective(lightVs, light.radius, cellMin, cellMax, cornerA.z);

		if (touches) {
			mask[i >> 5] |= 1u << (i & 31);
		}
	}

	const uint cellIndex = cell.y * globals.lightGridSize.x + cell.x;
	lightGridRW.Store4(cellIndex * LIGHT_CELL_STRIDE, uint4(mask[0], mask[1], mask[2], mask[3]));
}
