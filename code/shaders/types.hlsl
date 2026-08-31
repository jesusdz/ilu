

#define LIGHT_TILE_SIZE 16
#define MAX_VISIBLE_LIGHTS 128
#define MAX_LIGHT_GRID_WIDTH 256
#define MAX_LIGHT_GRID_HEIGHT 160
#define MAX_LIGHT_GRID_CELLS (MAX_LIGHT_GRID_WIDTH * MAX_LIGHT_GRID_HEIGHT)
#define LIGHT_CELL_STRIDE 16

// Must match the ProjectionType enum in graphics.h
#define PROJECTION_PERSPECTIVE 0
#define PROJECTION_ORTHOGRAPHIC 1

struct Globals
{
	float4x4 cameraView;
	float4x4 cameraViewInv;
	float4x4 cameraProj;
	float4x4 camera2dProj;
	float4x4 viewportRotationMatrix;
	float4 cameraFrustumTopLeft;
	float4 cameraFrustumBottomRight;
	float4x4 sunView;
	float4x4 sunProj;
	float4 sunDir;
	float4 eyePosition;
	float4 ambientLight;

	float shadowmapDepthBias;
	float time;
	uint2 lightGridSize;

	uint2 sceneResolution;
	int2 mousePosition;
	uint lightCount;
	uint projectionType;
	uint selectedEntity;
};

struct SEntity
{
	float4x4 world;
	uint spriteIndex;
	bool flipX;
};

struct SSpriteData
{
	float2 uvOffset;
	float2 uvSize;
	float2 worldSize;
};

struct STileData
{
	float3 pos;
	uint spriteIndex;
};

struct SMaterial
{
	float uvScale;
};


struct SLight
{
	float3 positionWs;
	float radius;
	float3 color;
	float intensity;
};
