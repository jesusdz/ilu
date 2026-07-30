#include "types.hlsl"
#include "bindings.hlsl"

// Space 0: Globals
ConstantBuffer<Globals> globals         : REGISTER_B(BIND_GROUP_GLOBAL, BINDING_GLOBALS);
SamplerState pointSampler               : REGISTER_S(BIND_GROUP_GLOBAL, BINDING_SAMPLER_POINT);
SamplerState linearSampler              : REGISTER_S(BIND_GROUP_GLOBAL, BINDING_SAMPLER_LINEAR);
ByteAddressBuffer entities              : REGISTER_T(BIND_GROUP_GLOBAL, BINDING_ENTITIES);
ByteAddressBuffer spriteData            : REGISTER_T(BIND_GROUP_GLOBAL, BINDING_SPRITE_DATA);
ByteAddressBuffer tileData              : REGISTER_T(BIND_GROUP_GLOBAL, BINDING_TILE_DATA);
Texture2D<float4> shadowmap             : REGISTER_T(BIND_GROUP_GLOBAL, BINDING_SHADOWMAP);
SamplerComparisonState shadowmapSampler : REGISTER_S(BIND_GROUP_GLOBAL, BINDING_SHADOWMAP_SAMPLER);
Texture2D<float4> noise2D               : REGISTER_T(BIND_GROUP_GLOBAL, BINDING_NOISE2D);

// Space 1: Materials
ConstantBuffer<SMaterial> material      : REGISTER_B(BIND_GROUP_MATERIAL, BINDING_MATERIAL);
Texture2D<float4> albedoTexture         : REGISTER_T(BIND_GROUP_MATERIAL, BINDING_ALBEDO);

