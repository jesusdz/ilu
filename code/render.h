#ifndef RENDER_H
#define RENDER_H

struct Engine;

////////////////////////////////////////////////////////////////////////
// Immediate draw

void DrawSprite(SpriteH spriteH, float2 worldPos, float4 pcolor);
void DrawBox(float2 pos, float2 size, float4 color);
void DrawBoxOutline(float2 pos, float2 size, float4 color);


////////////////////////////////////////////////////////////////////////
// Camera math

float3 UpDirectionFromAngles(const float2 &angles);
float3 ForwardDirectionFromAngles(const float2 &angles);
float3 RightDirectionFromAngles(const float2 &angles);
float4x4 ViewMatrixFromCamera(const Camera &camera);


////////////////////////////////////////////////////////////////////////
// Frame rendering

bool RenderGraphics(Engine &engine);

#endif // RENDER_H
