#ifndef DATA_H
#define DATA_H

////////////////////////////////////////////////////////////////////////
// Text data

// Types

enum AssetFlags
{
	// Not serialized and hidden from the editor's asset lists. Transient previews are ghosts, and so
	// are the builtins, which the engine recreates on its own.
	AssetFlag_Ghost = 1 << 0,
	// Owned by the engine, not by the scene, so CleanScene must leave it alone. These assets hold the
	// shared images bound in the global bind group, which nothing recreates after initialization.
	AssetFlag_Builtin = 1 << 1,
};

// The desc fields below are typed AssetFlags, but combining two enumerators yields an int that C++
// will not convert back to the enum on its own, so give the type the operator it is used as if it had.
inline AssetFlags operator|(AssetFlags a, AssetFlags b) { return (AssetFlags)((u32)a | (u32)b); }

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
	ID id;
	const char *name;
	const char *filename;
	u8 mipmap;
	AssetFlags flags;
};

struct MaterialDesc
{
	ID id;
	const char *name;
	ID textureId;
	const char *pipelineName;
	float uvScale;
	AssetFlags flags;
};

struct SpriteDesc
{
	ID id;
	const char *name;
	ID textureId;
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
	ID materialId;
	GeometryType geometryType;
	// Sprite entity
	ID spriteId;
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
	// Which member applies is decided by the owning LayerDesc::isCollider. Both are
	// four bytes wide, so the raw u32 doubles as the serialized view of either.
	union
	{
		ID spriteId;
		u32 collider;
	};
};

struct LayerDesc
{
	const char *name;
	bool isBase;
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
	ID id;
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
	ID id;
	const char *name;
	ID textureId;
	const char *pipelineName;
	float uvScale;
};

struct BinSpriteDesc
{
	ID id;
	const char *name;
	ID textureId;
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
	ID materialId;
	ID spriteId;
	float3 pos;
	float scale;
	i32 layer;
	GeometryType geometryType;
};

struct BinLayerDesc
{
	const char *name;
	u8 isBase;
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

constexpr u32 BinAssetsVersion = 6; // 6: sprites carry an ID; entities and tiles refer to one by ID

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
