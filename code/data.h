#ifndef DATA_H
#define DATA_H

////////////////////////////////////////////////////////////////////////
// Text data

// Types

enum AssetFlags
{
	AssetFlag_Builtin = 1 << 0,
};

enum ShaderType
{
	ShaderTypeVertex,
	ShaderTypeFragment,
	ShaderTypeCompute
};

struct ShaderSourceDesc
{
	ShaderType type;
	const char *filename;
	const char *entryPoint;
	const char *name;
	const char *defines;
};

struct TextureDesc
{
	const char *name;
	const char *filename;
	u8 mipmap;
	AssetFlags flags;
};

struct MaterialDesc
{
	const char *name;
	const char *textureName;
	const char *pipelineName;
	float uvScale;
	AssetFlags flags;
};

struct SpriteDesc
{
	const char *name;
	const char *textureName;
	uint2 pos;
	uint2 size;
	u32 frameCount;
	u32 fps;
	u8 loop;
};

enum GeometryType
{
	GeometryTypeCube,
	GeometryTypePlane,
	GeometryTypeScreen,
	GeometryTypeQuad,
	GeometryTypeSprite,
};

struct EntityDesc
{
	const char *name;
	// 3D entity
	const char *materialName;
	GeometryType geometryType;
	// Sprite entity
	const char *spriteName;
	i32 layer;
	// Common
	float3 pos;
	float scale;
};

#define MAX_LAYERS 4

struct TileDesc
{
	u16 x;
	u16 y;
	union
	{
		u16 spriteIndex; // index into AssetDescriptors::spriteDescs
		u16 collider;
	};
};

struct LayerDesc
{
	const char *name;
	i32 order;
	bool visible;
	bool isCollider;
	uint2 size;
	TileDesc *tiles; // non-empty grid cells only
	u32 tileCount;
};

struct RoomDesc
{
	const char *name;
	int2 pos;
	LayerDesc layers[MAX_LAYERS];
	u32 layerCount;
};

struct AudioClipDesc
{
	const char *name;
	const char *filename;
	AssetFlags flags;
};

struct MusicFileDesc
{
	const char *name;
	const char *filename;
	AssetFlags flags;
};

struct AssetDescriptors
{
	ShaderSourceDesc *shaderDescs;
	u32 shaderDescCount;

	TextureDesc *textureDescs;
	u32 textureDescCount;

	SpriteDesc *spriteDescs;
	u32 spriteDescCount;

	MaterialDesc *materialDescs;
	u32 materialDescCount;

	EntityDesc *entityDescs;
	u32 entityDescCount;

	RoomDesc *roomDescs;
	u32 roomDescCount;

	AudioClipDesc *audioClipDescs;
	u32 audioClipDescCount;

	MusicFileDesc *musicFileDescs;
	u32 musicFileDescCount;
};

// Functions

#if USE_DATA_BUILD
void CompileShaders();
bool CompileModifiedShaders();
void SaveAssetDescriptors(const char *path, const AssetDescriptors &assetDescriptors);
AssetDescriptors ParseDescriptors(const char *filepath, Arena &arena);
#endif // USE_DATA_BUILD



////////////////////////////////////////////////////////////////////////
// Binary data

// Types

#pragma pack(push, 1)

struct BinLocation
{
	u32 offset;
	u32 size;
};

struct BinShaderDesc
{
	const char *name;
	const char *entryPoint;
	ShaderType type;
	BinLocation location;
};

struct BinImageDesc
{
	const char *name;
	u16 width;
	u16 height;
	u8  channels;
	u8  mipmap;
	u16 unused;
	BinLocation location;
};

struct BinAudioClipDesc
{
	u32 sampleCount;
	u32 samplingRate;
	u16 sampleSize;
	u16 channelCount;
	BinLocation location;
};

struct BinMusicFileDesc
{
	const char *name;
	BinLocation location;
};

struct BinMaterialDesc
{
	const char *name;
	const char *textureName;
	const char *pipelineName;
	float uvScale;
};

struct BinSpriteDesc
{
	const char *name;
	const char *textureName;
	uint2 pos;
	uint2 size;
	u32 frameCount;
	u32 fps;
	u8 loop;
	u8 _pad[3];
};

struct BinEntityDesc
{
	const char *name;
	const char *materialName;
	const char *spriteName;
	float3 pos;
	float scale;
	i32 layer;
	GeometryType geometryType;
};

struct BinLayerDesc
{
	const char *name;
	i32 order;
	u8 visible;
	u8 isCollider;
	uint2 size;
	BinLocation tiles; // payload of TileDesc entries; count == tiles.size / sizeof(TileDesc)
};

struct BinRoomDesc
{
	const char *name;
	int2 pos;
	u32 layerCount;
	BinLayerDesc layers[MAX_LAYERS];
};

constexpr u32 BinAssetsVersion = 2;

struct BinAssetsHeader
{
	u32 magicNumber;
	u32 version;
	u32 shadersOffset;
	u32 shaderCount;
	u32 imagesOffset;
	u32 imageCount;
	u32 audioClipsOffset;
	u32 audioClipCount;
	u32 musicFilesOffset;
	u32 musicFileCount;
	u32 materialsOffset;
	u32 materialCount;
	u32 spritesOffset;
	u32 spriteCount;
	u32 entitiesOffset;
	u32 entityCount;
	u32 roomsOffset;
	u32 roomCount;
	u32 stringPoolOffset;
	u32 stringPoolSize;
};

struct BinShader
{
	BinShaderDesc *desc;
	byte *spirv;
};

struct BinImage
{
	BinImageDesc *desc;
	byte *pixels;
};

struct BinAudioClip
{
	BinAudioClipDesc *desc;
};

struct BinMusicFile
{
	BinMusicFileDesc *desc;
};

struct BinMaterial
{
	BinMaterialDesc *desc;
};

struct BinSprite
{
	BinSpriteDesc *desc;
};

struct BinEntity
{
	BinEntityDesc *desc;
};

struct BinRoom
{
	BinRoomDesc *desc;
	TileDesc *tiles[MAX_LAYERS];
};

#pragma pack(pop)


struct BinAssets
{
	File file;

	BinAssetsHeader header;

	BinShader *shaders;
	BinImage *images;
	BinAudioClip *audioClips;
	BinMusicFile *musicFiles;
	BinMaterial *materials;
	BinSprite *sprites;
	BinEntity *entities;
	BinRoom *rooms;
};

// Functions

#if USE_DATA_BUILD
void BuildAssets(const AssetDescriptors &assetDescriptors, const char *filepath, Arena tempArena);
#endif // USE_DATA_BUILD

BinAssets OpenAssets(Arena &dataArena, const char *filepath);
void CloseAssets(BinAssets &assets);

#endif // DATA_H
