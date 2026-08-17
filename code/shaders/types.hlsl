
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

	float shadowmapDepthBias;
	float time;
	float unused1;
	float unused2;

	uint2 sceneResolution;
	int2 mousePosition;
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

