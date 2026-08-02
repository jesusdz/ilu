#include "globals.hlsl"

struct VertexInput
{
	float3 position : POSITION;
};

struct VertexOutput
{
	float4 position : SV_Position;
};

uint EntityId(uint entityHandle)
{
	return entityHandle>>16;
}

VertexOutput VSMain(VertexInput IN, uint entityHandle : SV_InstanceID)
{
	VertexOutput OUT;
	uint entityId = EntityId(entityHandle);
	float4x4 worldMatrix = entities.Load<SEntity>(entityId * sizeof(SEntity)).world;
	float4 positionWs = mul(worldMatrix, float4(IN.position, 1.0f));
	OUT.position = mul(globals.sunProj, mul(globals.sunView, positionWs));
	return OUT;
}

void PSMain() : SV_Target
{
	// Do nothing, we're just writing to depth buffer
}

