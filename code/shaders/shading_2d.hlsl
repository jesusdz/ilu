#include "defines.hlsl"
#include "globals.hlsl"
#include "lighting.hlsl"

Texture2D<float4> spriteTexture : REGISTER_T(2, 0);

struct VertexInput
{
	float3 position : POSITION;
	float3 normal   : NORMAL0;
	float2 texCoord : TEXCOORD0;
};

struct Interpolators
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
	float depth : POSITION0;
	float3 positionWs : POSITION1;
#if USE_ENTITY_SELECTION
	nointerpolation bool isSelected : POSITION2;
#endif
};

typedef Interpolators VertexOutput;
typedef Interpolators PixelInput;

uint EntityId(uint entityHandle)
{
	return entityHandle>>16;
}

VertexOutput VSMain(VertexInput IN, uint instanceID : SV_InstanceID)
{
	VertexOutput OUT;

	float2 texCoord = IN.texCoord;

#if USE_TILE_RENDERING

	uint tileIndex = instanceID;
	STileData tile = tileData.Load<STileData>(tileIndex * sizeof(STileData));
	SSpriteData sprite = spriteData.Load<SSpriteData>(tile.spriteIndex * sizeof(SSpriteData));
	float3 posOs = float3(IN.position.xy * sprite.worldSize, IN.position.z);
	float3 disp = tile.pos;
	float4 posWs = float4(posOs + disp, 1.0);

#elif USE_ENTITY_RENDERING

	uint entityIndex = EntityId(instanceID);
	SEntity entityData = entities.Load<SEntity>(entityIndex * sizeof(SEntity));
	SSpriteData sprite = spriteData.Load<SSpriteData>(entityData.spriteIndex * sizeof(SSpriteData));
	float3 posOs = float3(IN.position.xy * sprite.worldSize, IN.position.z);
	float4 posWs = mul(entityData.world, float4(posOs, 1.0));

	if (entityData.flipX)
		texCoord.x = 1.0 - texCoord.x;

#if USE_ENTITY_SELECTION
	OUT.isSelected = instanceID == globals.selectedEntity;
#endif

#else
#error "Need to specify the type of geometry being rendered"
#endif

	OUT.position = mul(globals.cameraProj, mul(globals.cameraView, posWs));
	OUT.texCoord = sprite.uvOffset + texCoord * sprite.uvSize;
	OUT.depth = posWs.z;
	OUT.positionWs = posWs.xyz;

	return OUT;
}

float4 PSMain(PixelInput IN) : SV_Target
{
	float4 albedo = spriteTexture.Sample(pointSampler, IN.texCoord);

	if (albedo.a == 0.0)
		discard;

	const float3 lighting = globals.ambientLight.rgb + AccumulateLights2D(IN.positionWs, IN.position);
	albedo.rgb *= lighting;

#if USE_ENTITY_SELECTION
	if (IN.isSelected)
	{
		float3 orange = float3(1.0, 0.7, 0.0);
		albedo.rgb = lerp(albedo.rgb, orange, sin(globals.time * 2.0) * 0.1f + 0.2f);
	}
#endif

	return albedo;
}
