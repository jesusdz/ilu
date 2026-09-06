#ifndef ENGINE_H
#define ENGINE_H

////////////////////////////////////////////////////////////////////////////////////////////////////
// TYPES: Reflected properties
////////////////////////////////////////////////////////////////////////////////////////////////////

#define ILU_STRUCT(...)
#define ILU_PROPERTY(...)
#define ILU_ENUM(...)

// List of custom types for reflex
#define REFLEX_ID_CUSTOM_TYPES \
	ReflexID_IDBegin, \
	ReflexID_IDEntity, \
	ReflexID_IDTexture, \
	ReflexID_IDSprite, \
	ReflexID_IDPrefab, \
	ReflexID_IDSoundClip, \
	ReflexID_IDMusicFile, \
	ReflexID_IDEnd, \
	ReflexID_u32,

#include "reflex\reflex.h"

typedef ReflexID PropertyType;

typedef ID IDEntity;
typedef ID IDTexture;
typedef ID IDSprite;
typedef ID IDPrefab;
typedef ID IDSoundClip;
typedef ID IDMusicFile;

struct PropertyValue
{
	PropertyType type;
	union
	{
		u32 uValue;
		ID idValue;
	};
};

inline bool IsIDProperty(PropertyType type)
{
	const bool res = type >= ReflexID_IDBegin && type <= ReflexID_IDEnd;
	return res;
}

// Whether a reflected member holds a value the engine knows how to read, write
// and serialize. Pointers, arrays and types outside the property set are not
// storable, and every reflected member was tagged on purpose, so callers report
// these instead of silently dropping them.
inline bool IsStorableProperty(const ReflexMember &member)
{
	if ( member.pointerCount > 0 || member.isArray ) {
		return false;
	}

	const bool res = member.reflexId == ReflexID_u32 || IsIDProperty(member.reflexId);
	return res;
}

inline const char *PropertyTypeToString(PropertyType type)
{
	const char *str = ReflexGetTypeName(type);
	return str;
}

inline PropertyType StringToPropertyType(const char *str)
{
	const PropertyType type = ReflexGetTypeFromName(str);
	return type;
}

inline PropertyType StringToPropertyType(String str)
{
	char buffer[128];
	StrCopy(buffer, str);
	const PropertyType type = ReflexGetTypeFromName(buffer);
	return type;
}

inline PropertyValue GetPropertyValue(const ReflexMember &member, const void *base)
{
	if ( !IsStorableProperty(member) ) {
		const PropertyValue none = { .type = ReflexID_Null };
		return none;
	}

	const byte *field = (const byte *)base + member.offset;

	PropertyValue value = { .type = member.reflexId };

	if (member.reflexId == ReflexID_u32) {
		value.uValue = *(const u32*)field;
	} else if (IsIDProperty(member.reflexId)) {
		value.idValue = *(const ID*)field;
	}

	return value;
}

inline void SetPropertyValue(const ReflexMember &member, void *base, PropertyValue value)
{
	if ( !IsStorableProperty(member) || value.type != member.reflexId ) {
		return;
	}

	byte *field = (byte *)base + member.offset;

	if (member.reflexId == ReflexID_u32) {
		*(u32*)field = value.uValue;
	} else if (IsIDProperty(member.reflexId)) {
		*(ID*)field = value.idValue;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// TYPES: Assets
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Asset flags

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

////////////////////////////////////////////////////////////////////////
// Binary data

#pragma pack(push, 1)

struct BinLocation
{
	u32 offset;
	u32 size;
};

#pragma pack(pop)

////////////////////////////////////////////////////////////////////////////////////////////////////
// TYPES: Builtin IDs
////////////////////////////////////////////////////////////////////////////////////////////////////

// Saved data refers to these by value, so never renumber one that already exists in a
// scene file: append instead.
enum BuiltinID
{
	BuiltinID_DefaultTexture = 1, // 0 is reserved for invalid ID
	BuiltinID_NoiseTexture,
	BuiltinID_DefaultMaterial,
	BuiltinID_FountainParticleEffect,
	BuiltinID_FireParticleEffect,
	BuiltinID_Count,
};
CT_ASSERT(BuiltinID_Count <= ILU_ID_FIRST_DYNAMIC_SLOT);


////////////////////////////////////////////////////////////////////////////////////////////////////
// TYPES: Scripts
////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr u32 MAX_SCRIPTS = 64;
constexpr u32 MAX_SCRIPT_PROPERTIES = 16;

// Structs tagged ILU_STRUCT(Script) are the ones that can be registered as scripts
#define SCRIPT_HINT "Script"
constexpr u32 SCRIPT_DATA_ALIGN = 16;

constexpr u32 SCRIPT_SIZE_CLASS_COUNT = 64;
constexpr u32 MAX_POOLED_SCRIPT_DATA_SIZE = SCRIPT_SIZE_CLASS_COUNT * SCRIPT_DATA_ALIGN;
constexpr u32 SCRIPT_DATA_MEMORY = MB(1);

enum ScriptHookType
{
	ScriptHook_Start,
	ScriptHook_Simulate,
	ScriptHook_Update,
	ScriptHook_Stop,
	ScriptHook_Count,
};

typedef ReflexFunctor ScriptHook;

struct ScriptPropertyDesc
{
	const char *name;
	PropertyValue value;
};

struct ScriptDesc
{
	const char *name;
	u32 propertyCount;
	ScriptPropertyDesc properties[MAX_SCRIPT_PROPERTIES];
};

struct Script
{
	const ReflexStruct *type; // Layout and properties, generated by reflex
	ScriptHook hooks[ScriptHook_Count];
};

inline const char *ScriptName(const Script &script)
{
	return script.type->name;
}

inline u32 ScriptDataSize(const Script &script)
{
	return AlignUp((u32)script.type->size, SCRIPT_DATA_ALIGN);
}

struct ScriptComponent
{
	const char *name; // Interned, and what a reload re-resolves structIndex from
	u16 structIndex;  // U16_MAX until a script is assigned, and again if a reload drops it
	u32 dataSize;     // Which bucket data returns to, still known once the script is gone
	byte *data;
};

struct ScriptDataBlock
{
	ScriptDataBlock *next;
};

// a block is never smaller than the link it has to store while it waits in a bucket.
CT_ASSERT(SCRIPT_DATA_ALIGN >= sizeof(void*));

struct ScriptDataPool
{
	Arena arena;
	ScriptDataBlock *freeLists[SCRIPT_SIZE_CLASS_COUNT];
};

////////////////////////////////////////////////////////////////////////
// Binary data

#pragma pack(push, 1)

struct BinScriptPropertyDesc
{
	const char *name;
	ReflexID type;
	u32 value; // Raw view of PropertyValue, whichever member the type selects
};

struct BinScriptDesc
{
	const char *name;
	u32 propertyCount;
	BinScriptPropertyDesc properties[MAX_SCRIPT_PROPERTIES];
};

#pragma pack(pop)

////////////////////////////////////////////////////////////////////////////////////////////////////
// TYPES: Audio
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "libs/ibxm/ibxm.h"

#define MAX_AUDIO_CLIPS 16
#define MAX_AUDIO_SOURCES 16
#define AUDIO_CHUNK_SAMPLE_COUNT (48000u/4u)

#define MAX_MUSIC_FILES 16

enum AudioClipLoadSource
{
	AUDIO_CLIP_LOAD_SOURCE_WAV,
	//AUDIO_CLIP_LOAD_SOURCE_MOD,
	AUDIO_CLIP_LOAD_SOURCE_ASSETS,
};

struct AudioClipDesc
{
	ID id;
	const char *name;
	const char *filename;
	AssetFlags flags;
};

struct AudioClip
{
	AudioClipDesc desc;
	u32 sampleCount;
	u32 samplingRate;
	u16 sampleSize;
	u16 channelCount;
	AudioClipLoadSource loadSource;
	union
	{
		BinLocation location;
		const char *filename;
	};
};

enum AudioState
{
	AUDIO_STATE_IDLE,
	AUDIO_STATE_PLAYING,
	AUDIO_STATE_PAUSED,
};

struct AudioSource
{
	ID clip;
	u32 lastWriteSampleIndex = 0;
	AudioState state;
};

struct AudioChunk
{
	ID clipId;
	u32 index;
	i16 samples[AUDIO_CHUNK_SAMPLE_COUNT];
	AudioChunk *prev;
	AudioChunk *next;
};

enum LoadSource
{
	LOAD_SOURCE_MOD_FILE,
	LOAD_SOURCE_ASSET_FILE,
};

struct MusicFileDesc
{
	ID id;
	const char *name;
	const char *filename;
	AssetFlags flags;
};

struct MusicFile
{
	MusicFileDesc desc;
	LoadSource loadSource;
	union
	{
		BinLocation location;
		const char *filename;
	};
};

struct Audio
{
	// Compact, no holes, like the rest of the pools. Unlike the rest, these are read
	// by the mixing thread, so CompactAudio is what closes the gaps and it runs from
	// PreRenderAudio rather than from the frame loop. See CompactAudio.
	u32 clipCount;
	AudioClip clips[MAX_AUDIO_CLIPS] = {};

	AudioSource sources[MAX_AUDIO_SOURCES] = {};

	// Circular list of audio chunks
	AudioChunk audioChunkSentinel;

	// Music ring buffer
	i16 *musicBuffer;
	u32 musicBufferSampleCount; // Mono samples count

	// Music play state
	AudioState musicState;
	u32 musicBufferReadSampleIndex;
	u32 musicBufferWriteSampleIndex;

	u32 musicFileCount;
	MusicFile musicFiles[MAX_MUSIC_FILES] = {};

	ID musicFile; // Music file being played

	// MOD tracks
	Arena moduleArena;
	struct module *module;
	u32 moduleSampleCount;
	struct replay *moduleReplay;

	bool initialized;
};


#pragma pack(push, 1)

struct BinAudioClipDesc
{
	ID id;
	u32 sampleCount;
	u32 samplingRate;
	u16 sampleSize;
	u16 channelCount;
	BinLocation location;
};

struct BinMusicFileDesc
{
	ID id;
	const char *name;
	BinLocation location;
};

struct BinAudioClip
{
	BinAudioClipDesc *desc;
};

struct BinMusicFile
{
	BinMusicFileDesc *desc;
};

#pragma pack(pop)

////////////////////////////////////////////////////////////////////////////////////////////////////
// TYPES: Graphics
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Geometry and vertices

typedef u16 Index;

enum GeometryType
{
	GeometryTypeCube,
	GeometryTypePlane,
	GeometryTypeScreen,
	GeometryTypeQuad,
	GeometryTypeSprite,
};

enum ShaderType
{
	ShaderTypeVertex,
	ShaderTypeFragment,
	ShaderTypeCompute
};

struct Vertex
{
	float3 pos;
	float3 normal;
	float2 texCoord;
};

struct DebugDrawVertex
{
	float2 pos;
	float2 texCoord;
	rgba color;
};

struct DebugDrawBatch
{
	ImageH imageH;
	u32 vertexIndex;
	u32 vertexCount;
};

////////////////////////////////////////////////////////////////////////
// Textures

struct TextureDesc
{
	ID id;
	const char *name;
	const char *filename;
	u8 mipmap;
	AssetFlags flags;
};

struct Texture
{
	TextureDesc desc;
	ImageH image;
	bool ownsImage;
	uint2 size;
	u64 ts;
};

////////////////////////////////////////////////////////////////////////
// Materials

struct MaterialDesc
{
	ID id;
	const char *name;
	ID textureId;
	const char *pipelineName;
	float uvScale;
	AssetFlags flags;
};

struct Material
{
	MaterialDesc desc;
	u16 pipelineIndex;   // Resolved from desc.pipelineName
	u32 bufferOffset;    // Derived from the element index, so it moves with compaction
};

////////////////////////////////////////////////////////////////////////
// Render targets

struct RenderTargets
{
	uint2 sceneSize;
	ImageH depthImage;
	ImageH sceneImage;
	Framebuffer sceneFramebuffer;

	Framebuffer displayFramebuffers[MAX_SWAPCHAIN_IMAGE_COUNT];

	ImageH shadowmapImage;
	Framebuffer shadowmapFramebuffer;

	ImageH idImage;
	Framebuffer idFramebuffer;

	bool initialized;
};

////////////////////////////////////////////////////////////////////////
// Camera

enum ProjectionType
{
	ProjectionPerspective,
	ProjectionOrthographic,
	ProjectionTypeCount,
};
constexpr const char *ProjectionTypeStr[] = {
	"ProjectionPerspective",
	"ProjectionOrthographic",
};
CT_ASSERT(ARRAY_COUNT(ProjectionTypeStr) == ProjectionTypeCount);

// The light culling shader branches on globals.projectionType
CT_ASSERT(ProjectionPerspective == PROJECTION_PERSPECTIVE);
CT_ASSERT(ProjectionOrthographic == PROJECTION_ORTHOGRAPHIC);

inline const char *ProjectionTypeToStr(ProjectionType type)
{
	if ( type < ProjectionTypeCount ) {
		return ProjectionTypeStr[type];
	} else {
		return "<unknown>";
	}
}

inline ProjectionType StrToProjectionType(const char *str)
{
	ProjectionType type = ProjectionTypeCount;
	for (u32 i = 0; i < ProjectionTypeCount; ++i) {
		if ( StrEq(ProjectionTypeStr[i], str) ) {
			return (ProjectionType)i;
		}
	}
	LOG(Warning, "StrToProjectionType could not find projection type for: %s\n", str);
	return type;
}

struct Camera
{
	ProjectionType projectionType;
	float3 position;
	float2 orientation; // yaw and pitch
	f32 znear;
	f32 zfar;
	f32 height; // orthographic only: half the vertical size of the view volume
	f32 fovy;   // perspective only: vertical field of view, in degrees
};

////////////////////////////////////////////////////////////////////////
// Pipelines and shaders

// Code-only: never written to a file, so entries can be reordered or inserted freely.
// Asset files name a pipeline instead. Declared unconditionally, editor-only ones
// included, so the array shape does not depend on USE_EDITOR.
enum PipelineIndex
{
	Pipeline_Shading,
	Pipeline_Shading2D,
	Pipeline_Shading2DTile,
	Pipeline_Shadowmap,
	Pipeline_Sky,
	Pipeline_Grid2D,
	Pipeline_Grid3D,
	Pipeline_Blit,
	Pipeline_UI,
	Pipeline_ModelId,
	Pipeline_SpriteId,
	Pipeline_DebugDraw,
	Pipeline_Fog,
	Pipeline_ComputeSelect,
	Pipeline_LightBinning,
	Pipeline_Count,
};

struct ShaderAndPipelineDesc
{
	const char *vsName;
	const char *fsName;
	const char *renderPass;
	PipelineIndex index;
	PipelineDesc desc;
};

struct ShaderAndComputeDesc
{
	const char *csName;
	PipelineIndex index;
	ComputeDesc desc;
};

struct ShaderSourceDesc
{
	ShaderType type;
	const char *filename;
	const char *entryPoint;
	const char *name;
	const char *defines;
};

////////////////////////////////////////////////////////////////////////
// Graphics state

#define MAX_TEXTURES 4092
#define MAX_MATERIALS 4092
#define MAX_DYNAMIC_BIND_GROUPS 4092
#define MAX_DEBUG_DRAW_BATCHES 64

struct Graphics
{
	GraphicsDevice device;

	RenderTargets renderTargets;

	BufferH stagingBuffer;
	u32 stagingBufferOffset;
	bool inUploadContext;

	BufferArena globalVertexArena;
	BufferArena globalIndexArena;

	BufferChunk cubeVertices;
	BufferChunk cubeIndices;
	BufferChunk planeVertices;
	BufferChunk planeIndices;
	BufferChunk quadVertices;
	BufferChunk quadIndices;
	BufferChunk spriteVertices;
	BufferChunk spriteIndices;
	BufferChunk screenTriangleVertices;
	BufferChunk screenTriangleIndices;

	BufferH globalsBuffer[MAX_FRAMES_IN_FLIGHT];
	BufferH entityBuffer[MAX_FRAMES_IN_FLIGHT];
	BufferH materialBuffer;
	BufferH computeBufferH;
	BufferViewH computeBufferViewH;
#if USE_EDITOR
	BufferH selectionBufferH;
	BufferViewH selectionBufferViewH;
#endif

	BufferH debugDrawVertexBuffer[MAX_FRAMES_IN_FLIGHT];
	DebugDrawVertex *debugDrawVertices[MAX_FRAMES_IN_FLIGHT];
	DebugDrawVertex *debugDrawVerticesCPU;
	u32 debugDrawVertexCount;
	DebugDrawBatch debugDrawBatches[MAX_DEBUG_DRAW_BATCHES];
	u32 debugDrawBatchCount;

	BufferH spriteDataBuffer[MAX_FRAMES_IN_FLIGHT];
	BufferH tileDataBuffer[MAX_FRAMES_IN_FLIGHT];
	BufferH lightBuffer[MAX_FRAMES_IN_FLIGHT];
	BufferH lightGridBuffer[MAX_FRAMES_IN_FLIGHT];

	SamplerH pointSamplerH;
	SamplerH linearSamplerH;
	SamplerH shadowmapSamplerH;
	SamplerH skySamplerH;
	SamplerH screenSamplerH;

	RenderPassH litRenderPassH;
	RenderPassH shadowmapRenderPassH;
	RenderPassH idRenderPassH;
	RenderPassH displayRenderPassH;

	u32 textureCount;
	Texture textures[MAX_TEXTURES];

	u32 materialCount;
	Material materials[MAX_MATERIALS];
	bool shouldUpdateMaterials;

	BindGroupAllocator globalBindGroupAllocator;
	BindGroupAllocator materialBindGroupAllocator;
	BindGroupAllocator dynamicBindGroupAllocator[MAX_FRAMES_IN_FLIGHT];

	BindGroupLayout globalBindGroupLayout;

	// Updated each frame so we need MAX_FRAMES_IN_FLIGHT elements
	BindGroup globalBindGroups[MAX_FRAMES_IN_FLIGHT];
	bool shouldUpdateGlobalBindGroups;

	BindGroup materialBindGroups[MAX_MATERIALS]; // Parallel to materials
	bool shouldUpdateMaterialBindGroups;

	BindGroupDesc dynamicBindGroupDescs[MAX_DYNAMIC_BIND_GROUPS];
	BindGroup dynamicBindGroups[MAX_DYNAMIC_BIND_GROUPS];
	u32 dynamicBindGroupCount;

	ImageH whiteImageH;
	ImageH pinkImageH;
	ImageH grayImageH;
	ImageH blackImageH;
	ImageH noiseImageH;

	ID skyTexture;
	ID defaultTexture;
	ID noiseTexture;

	ID defaultMaterial;

	PipelineH pipelines[Pipeline_Count];

	bool deviceInitialized;

	f32 deltaSeconds;

	Camera camera;
};

////////////////////////////////////////////////////////////////////////
// Binary data

#pragma pack(push, 1)

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

struct BinMaterialDesc
{
	ID id;
	const char *name;
	ID textureId;
	const char *pipelineName;
	float uvScale;
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

struct BinMaterial
{
	BinMaterialDesc *desc;
};

#pragma pack(pop)

////////////////////////////////////////////////////////////////////////////////////////////////////
// TYPES: Scene
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Scene descriptor

struct SceneDesc
{
	ProjectionType projectionType;
	float3 ambientLight;
};

////////////////////////////////////////////////////////////////////////
// Components

enum ComponentTypes
{
	ComponentType_Light,
	ComponentType_Particles,
	ComponentType_Script,
	ComponentType_Count,
};

enum ComponentBits
{
	Component_Light = 1<<ComponentType_Light,
	Component_Particles = 1<<ComponentType_Particles,
	Component_Script = 1<<ComponentType_Script,
};

typedef u32 ComponentFlags;

constexpr const char *ComponentNames[] = { "Light", "Particles", "Script" };

CT_ASSERT(ARRAY_COUNT(ComponentNames) == ComponentType_Count);

enum LightType
{
	LightType_Point,
};

struct LightComponent
{
	LightType type;
	float3 color;
	f32 intensity;
	f32 radius;
};

struct ParticleEffectDesc
{
	ID id;
	const char *name;

	// Look
	ID spriteID;
	float4_range color;
	f32_range size;

	// Emission
	f32 rate;
	u32 burstCount;
	f32 duration;
	u8 loop;

	// Per-particle spawn ranges
	f32_range lifetime;
	f32_range speed;
	f32_range angle;

	// Shape
	float2 spawnOffset;
	float2 spawnExtent;

	// Simulation
	float2 gravity;
	f32 drag;
	u8 worldSpace;
};

struct ParticleEffect
{
	ParticleEffectDesc desc;
};

struct ParticlesComponent
{
	ID effectId;
	u8 playOnStart;
	u8 playing;
	f32 emitAccum; // ???
	f32 elapsedTime; // from 0 to duration
};

struct Particle
{
	float2 pos;
	float2 vel;
	f32 age;
	f32 lifetime;
	ID effectId;
	ID entityId;
};

////////////////////////////////////////////////////////////////////////
// Descriptors

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

struct EntityDesc
{
	ID id;
	const char *name;
	// Transform
	float3 pos;
	float scale;
	// 3D entity
	ID materialId;
	GeometryType geometryType;
	// Sprite entity
	ID spriteId;
	ID layerId;
	// Collider
	float2 colliderSize;
	// Physics
	float2 speed;
	f32 accel;
	// Scripts
	ScriptDesc script;
	// Components
	ComponentFlags components;
	LightComponent light;
};

#define MAX_PREFAB_ENTITIES 16

struct PrefabDesc
{
	ID id;
	const char *name;
	EntityDesc entities[MAX_PREFAB_ENTITIES];
	u32 entityCount;
};

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
	ID id;
	const char *name;
	bool isBase;
	bool visible;
	bool isCollider;
	uint2 size;
	TileDesc *tiles; // non-empty grid cells only
	u32 tileCount;
};

#define MAX_LAYERS 4


struct RoomDesc
{
	ID id;
	const char *name;
	int2 pos;
	LayerDesc layers[MAX_LAYERS];
	u32 layerCount;
};

////////////////////////////////////////////////////////////////////////
// Sprites and entities

// Resolved form of the descriptor: size filled in from the texture when the desc left
// it at zero, frameCount forced to at least one, textureId guaranteed to resolve.
struct Sprite
{
	SpriteDesc desc;
};

struct SpriteAnimState
{
	f32 elapsedTime;
	u32 currentFrame;
};

struct Entity
{
	ID id;
	const char *name;
	// Transform
	float3 position;
	float scale;
	// 3D entity
	GeometryType geometryType;
	BufferChunk vertices;
	BufferChunk indices;
	ID materialId;
	// Sprite entity
	ID spriteId;
	bool flipX;
	// Collider
	float2 colliderSize;
	// Physics
	float2 speed;
	f32 accel;
	// Script

	ID layerId;

	bool visible;
	bool culled;

	// Hierarchy links. IDs rather than pointers: the element array compacts, so a
	// pointer into it would dangle at the next CompactEntities.
	//ID next;  // Sibling
	//ID child; // First child
};

////////////////////////////////////////////////////////////////////////
// Tile grid, rooms and layers

#define PIXELS_PER_METER 16
#define TILE_GRID_SIZE_X 40
#define TILE_GRID_SIZE_Y 30
#define TILE_SIZE_PIXELS 16.0f // size of each grid cell, in pixels (at PIXELS_PER_METER scale)

union Cell
{
	ID spriteId;
	u32 collider;
};

struct Layer
{
	bool initialized;
	ID id;
	const char *name;
	bool isBase; // Room's reference layer
	bool visible;
	bool isCollider;
	uint2 size;
	Cell cells[TILE_GRID_SIZE_X][TILE_GRID_SIZE_Y]; // sprite per cell, an invalid ID if empty
	f32 depth; // depth in world units
};

struct Room
{
	ID id;
	const char *name;
	int2 pos;
	Layer layers[MAX_LAYERS];
	u32 layerCount;
};

////////////////////////////////////////////////////////////////////////
// Prefabs

// A named, instantiable template: a fixed set of EntityDescs spawned together and
// offset by a world position. No hierarchy links between them yet, matching Entity's.
struct Prefab
{
	ID id;
	const char *name;
	EntityDesc entities[MAX_PREFAB_ENTITIES];
	u32 entityCount;
};

////////////////////////////////////////////////////////////////////////
// Scene state

#define MAX_ENTITIES 4092
#define MAX_SPRITES 4092
#define MAX_ROOMS 256
#define MAX_PREFABS 256
#define MAX_TILES 16 * 16 * 8 * MAX_ROOMS

constexpr u32 SCENE_WIDTH = 320;
constexpr u32 SCENE_HEIGHT = 180;

constexpr u32 MAX_PARTICLES = 1024;
constexpr u32 MAX_PARTICLE_EFFECTS = 8;

struct Scene
{
	ProjectionType projectionType;
	float3 ambientLight;

	u32 roomCount;
	Room rooms[MAX_ROOMS];

	u32 entityCount;
	Entity entities[MAX_ENTITIES];
	ComponentFlags entityComponents[MAX_ENTITIES];
	LightComponent entityLights[MAX_ENTITIES];
	ParticlesComponent entityParticles[MAX_ENTITIES];
	ScriptComponent entityScripts[MAX_ENTITIES];

	u32 particleEffectCount;
	ParticleEffect particleEffects[MAX_PARTICLE_EFFECTS];

	RandomSeries particleRandom;

	u32 particleCount;
	Particle particles[MAX_PARTICLES];

	u32 spriteCount;
	Sprite sprites[MAX_SPRITES];
	SpriteAnimState spriteAnimStates[MAX_SPRITES]; // Parallel to sprites

	u32 prefabCount;
	Prefab prefabs[MAX_PREFABS];
};

////////////////////////////////////////////////////////////////////////
// Binary data

#pragma pack(push, 1)

struct BinSceneDesc
{
	ProjectionType projectionType;
	float3 ambientLight;
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
	ID id;
	const char *name;
	ID materialId;
	ID spriteId;
	ID layerId;
	float3 pos;
	float scale;
	GeometryType geometryType;
	BinScriptDesc script;
	ComponentFlags components;
	LightComponent light;
};

struct BinLayerDesc
{
	ID id;
	const char *name;
	u8 isBase;
	u8 visible;
	u8 isCollider;
	uint2 size;
	BinLocation tiles; // payload of TileDesc entries; count == tiles.size / sizeof(TileDesc)
};

struct BinRoomDesc
{
	ID id;
	const char *name;
	int2 pos;
	u32 layerCount;
	BinLayerDesc layers[MAX_LAYERS];
};

struct BinPrefabDesc
{
	ID id;
	const char *name;
	u32 entityCount;
	BinEntityDesc entities[MAX_PREFAB_ENTITIES];
};

struct BinSprite
{
	BinSpriteDesc *desc;
};

struct BinEntity
{
	BinEntityDesc *desc;
};

struct BinPrefab
{
	BinPrefabDesc *desc;
};

struct BinRoom
{
	BinRoomDesc *desc;
	TileDesc *tiles[MAX_LAYERS];
};

#pragma pack(pop)

////////////////////////////////////////////////////////////////////////////////////////////////////
// TYPES: Game
////////////////////////////////////////////////////////////////////////////////////////////////////

enum GameState
{
	GameStateStopped,
	GameStateStarting,
	GameStateRunning,
	GameStateStopping,
	GameStateCount,
};

struct InputButton
{
	u8 press : 1;
	u8 pressed : 1;
	u8 release : 1;
};

struct GameInput
{
	float2 move;
	InputButton jump;
};

constexpr f32 SIMULATE_SECONDS = (1.0f / 60.0f);

struct Game
{
	GameState state;

	InputAccumulator accumulatedInput;
	GameInput input;

	f32 deltaSeconds;
	f32 accumulatedSeconds;

	ID currentEntity;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// TYPES: Data
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Text data

struct AssetDescriptors
{
	SceneDesc sceneDesc;

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

	PrefabDesc *prefabDescs;
	u32 prefabDescCount;

	RoomDesc *roomDescs;
	u32 roomDescCount;

	AudioClipDesc *audioClipDescs;
	u32 audioClipDescCount;

	MusicFileDesc *musicFileDescs;
	u32 musicFileDescCount;
};

////////////////////////////////////////////////////////////////////////
// Binary data

constexpr u32 BinAssetsVersion = 14; // 14: scripts nested in their entity

#pragma pack(push, 1)

struct BinAssetsHeader
{
	u32 magicNumber;
	u32 version;
	u32 sceneOffset;
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
	u32 prefabsOffset;
	u32 prefabCount;
	u32 roomsOffset;
	u32 roomCount;
	u32 stringPoolOffset;
	u32 stringPoolSize;
};

#pragma pack(pop)


struct BinAssets
{
	File file;

	BinAssetsHeader header;

	BinSceneDesc scene;
	BinShader *shaders;
	BinImage *images;
	BinAudioClip *audioClips;
	BinMusicFile *musicFiles;
	BinMaterial *materials;
	BinSprite *sprites;
	BinEntity *entities;
	BinPrefab *prefabs;
	BinRoom *rooms;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// TYPES: Engine
////////////////////////////////////////////////////////////////////////////////////////////////////

struct Settings
{
	bool hotReload;
};

struct Engine
{
	IDPool idPool;

	Graphics gfx;
	Audio audio;
	Scene scene;
	Game game;
	ScriptDataPool scriptData;
#if USE_UI
	UI ui;
#endif
	Settings settings;

	BinAssets shaderAssets;
	BinAssets assets;

	Arena dataArenaStates[1];
	u32 dataArenaStateCount;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS: Scripts
////////////////////////////////////////////////////////////////////////////////////////////////////

// The registry is rebuilt by RegisterScripts on every reload, so an index is only
// valid until the next one. Never hold one across a reload, hold the name instead.
void RegisterScripts();
u32 ScriptCount();
const Script &GetScriptAt(u32 index);

ScriptComponent *AddScript(Engine &engine, ID entityId);
ScriptComponent *SetScript(Engine &engine, ID entityId, const char *scriptName);
void SetScript(Engine &engine, ID entityId, const ScriptDesc &desc);
void RemoveScript(Engine &engine, ID entityId);
bool GatherEntityScriptDesc(const Scene &scene, ID entityId, ScriptDesc &outScript);
void RunScriptHooks(Engine &engine, ScriptHookType hook);
void RebindScripts(Engine &engine);

////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS: Audio
////////////////////////////////////////////////////////////////////////////////////////////////////

// Each function takes the narrowest thing it touches, so a signature says how far the
// call can reach:
// - ID only        Resolved through the ID pool, no subsystem state read (see ilu_id.h).
// - nothing        Only queues an AudioCmd, which the mixing thread applies later.
// - Audio &        Reads or writes the audio pools.
// - Engine &       Streams from engine.assets, so it needs more than the audio state.

bool InitializeAudio(Audio &audio, Arena &globalArena);

bool LoadAudioClipFromWAVFile(const char *filename, Arena &arena, AudioClip &audioClip, void **outSamples);
bool LoadSamplesFromWAVFile(const char *filename, void *samples, u32 firstSampleIndex, u32 sampleCount);

AudioClip &GetAudioClip(ID clipId);
ID CreateAudioClip(Audio &audio, const BinAudioClip &binAudioClip);
ID CreateAudioClip(Audio &audio, const AudioClipDesc &audioClipDesc);
ID GetOrCreateAudioClip(Audio &audio, const AudioClipDesc &audioClipDesc);
void RemoveAudioClip(ID clipId); // Deferred, takes effect on the next CompactAudio
void CompactAudio(Audio &audio);
u32 PlayAudioClip(Audio &audio, ID clipId);
bool IsActiveAudioSource(const Audio &audio, u32 audioSourceIndex);
bool IsPausedAudioSource(const Audio &audio, u32 audioSourceIndex);
void PauseAudioSource(u32 audioSourceIndex);
void ResumeAudioSource(u32 audioSourceIndex);
void StopAudioSource(u32 audioSourceIndex);

void PreRenderAudio(Audio &audio);
void RenderAudio(Engine &engine, SoundBuffer &soundBuffer); // Streams clips from engine.assets

MusicFile &GetMusicFile(ID musicId);
ID CreateMusicFile(Audio &audio, const BinMusicFile &binMusicFile);
ID CreateMusicFile(Audio &audio, const MusicFileDesc &musicFileDesc);
ID GetOrCreateMusicFile(Audio &audio, const MusicFileDesc &musicFileDesc);
void DestroyMusicFile(ID musicId);
void MusicPlay(Engine &engine, ID musicId); // Streams the module from engine.assets
void MusicPause();
void MusicStop(Audio &audio);
bool MusicIsPlaying(const Audio &audio);

void AudioStopAll(Audio &audio);

////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS: Graphics
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Image loading

bool ReadImagePixels(Arena &arena, const char *filepath, ImagePixels &image);
ImagePixels ResizeImagePixels(Arena &arena, ImagePixels inputImagePixels, i32 w, i32 h);


////////////////////////////////////////////////////////////////////////
// Shader source and pipeline descriptor tables

ShaderSourceDesc *GetShaderSourceDescs();
u32 GetShaderSourceDescCount();

const u32 FindShaderSourceDescIndex(const char *name);
RenderPassH FindRenderPassHandle(const Graphics &gfx, const char *name);

// Pipelines are named in asset files, so that adding or reordering one never invalidates
// a file. Compute pipelines are deliberately not searched: only a graphics pipeline can
// back a material.
PipelineIndex FindPipelineIndex(const char *name);
const char *GetPipelineName(u16 index);


////////////////////////////////////////////////////////////////////////
// Buffers and data upload

BufferH CreateStagingBuffer(Graphics &gfx);
BufferH CreateVertexBuffer(Graphics &gfx, u32 size);
BufferH CreateIndexBuffer(Graphics &gfx, u32 size);
BufferArena MakeBufferArena(Graphics &gfx, BufferH bufferHandle);
void UploadData(Graphics &gfx, const CommandList &commandList, const void *data, u32 size, BufferH destBuffer, u32 destOffset, u32 alignment = 0);
BufferChunk PushData(Graphics &gfx, const CommandList &commandList, BufferArena &arena, const void *data, u32 size, u32 alignment = 0);


////////////////////////////////////////////////////////////////////////
// Image management

void GenerateMipmaps(const GraphicsDevice &device, const CommandList &commandList, ImageH imageH);
ImageH GfxCreateImage(Graphics &gfx, const char *name, int width, int height, int channels, bool mipmap, const byte *pixels);
ImageH GfxCreateImage(Graphics &gfx, const ImagePixels &img, const char *name, bool createMipmaps);


////////////////////////////////////////////////////////////////////////
// Texture management

Texture &GetTexture(ID id);
Texture &GetTextureAt(Graphics &gfx, u32 index);
ID CreateTexture(Graphics &gfx, const TextureDesc &desc, ImageH imageH);
ID CreateTexture(Graphics &gfx, const TextureDesc &desc);
ID GetOrCreateTexture(Graphics &gfx, const TextureDesc &desc);
ID CreateTexture(Graphics &gfx, const BinImage &binImage);
ImageH GetTextureImage(Graphics &gfx, ID textureId, ImageH imageH);
void RemoveTexture(Graphics &gfx, ID textureId);
void CompactTextures(Graphics &gfx);
void RecreateModifiedTextures(Engine &engine);


////////////////////////////////////////////////////////////////////////
// Material management

Material &GetMaterial(ID id);
u16 GetMaterialIndex(const Graphics &gfx, ID materialId);
ID CreateMaterial(Graphics &gfx, const MaterialDesc &desc);
ID GetOrCreateMaterial(Graphics &gfx, const MaterialDesc &desc);
ID CreateMaterial(Graphics &gfx, const BinMaterialDesc &desc);
void RemoveMaterial(Graphics &gfx, ID materialId);
void CompactMaterials(Graphics &gfx);


////////////////////////////////////////////////////////////////////////
// Builtin geometry

BufferChunk GetVerticesForGeometryType(Graphics &gfx, GeometryType geometryType);
BufferChunk GetIndicesForGeometryType(Graphics &gfx, GeometryType geometryType);


////////////////////////////////////////////////////////////////////////
// Pipeline compilation

void CompileGraphicsPipeline(Engine &engine, Arena scratch, const ShaderAndPipelineDesc &shaderPipeDesc);
void CompileComputePipeline(Engine &engine, Arena scratch, const ShaderAndComputeDesc &shaderComputeDesc);
void RecompilePipelines(Engine &engine, Arena scratch);


////////////////////////////////////////////////////////////////////////
// Dynamic bind groups

void ResetDynamicBindGroups(Graphics &gfx);
const BindGroup &GetOrCreateDynamicBindGroup(Graphics &gfx, const BindGroupDesc &bindGroupDesc);


////////////////////////////////////////////////////////////////////////
// Render targets

void CreateRenderTargets(Graphics &gfx, u32 sceneWidth = 0, u32 sceneHeight = 0);
void DestroyRenderTargets(Graphics &gfx, RenderTargets &renderTargets);


////////////////////////////////////////////////////////////////////////
// Device lifetime and bind groups

bool InitializeGraphics(Engine &engine, Arena &globalArena);
BindGroupDesc GlobalBindGroupDesc(const Graphics &gfx, u32 frameIndex);
void UpdateGlobalBindGroups(Graphics &gfx);
BindGroupDesc MaterialBindGroupDesc(Graphics &gfx, const Material &material);
void UpdateMaterialBindGroups(Graphics &gfx);
void UploadMaterialData(Graphics &gfx);
void CreateMaterialBindGroup(Graphics &gfx, ID materialId);
void CreateMaterialBindGroups(Graphics &gfx);
void GfxWaitDeviceIdle(Graphics &gfx);
void CleanupGraphics(Graphics &gfx);


////////////////////////////////////////////////////////////////////////
// Framebuffer and swapchain queries

uint2 GetFramebufferSize(const Framebuffer &framebuffer);
const ImageH GetDisplayImageH(const Graphics &gfx);
const Image &GetDisplayImage(const Graphics &gfx);
Framebuffer GetDisplayFramebuffer(const Graphics &gfx);
Framebuffer GetShadowmapFramebuffer(const Graphics &gfx);

////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS: Scene
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Scene initialization

void InitializeScene(Engine &engine);


////////////////////////////////////////////////////////////////////////
// Particle effect management

ParticleEffect &GetParticleEffect(ID particleEffectId);
ID CreateParticleEffect(Engine &engine, const ParticleEffectDesc &desc);
ID FindParticleEffect(const Scene &scene, const char *name);
void RemoveParticleEffect(Scene &scene, ID particleEffectId);
void CompactParticleEffects(Scene &scene);

void SimulateParticles(Scene &scene, f32 deltaSeconds);
void PlayParticles(Scene &scene, ID entityId);
void StopParticles(Scene &scene, ID entityId);
void StartParticles(Scene &scene);
void ClearParticles(Scene &scene);


////////////////////////////////////////////////////////////////////////
// Sprite management

Sprite &GetSprite(ID spriteId);
u16 GetSpriteIndex(const Scene &scene, ID spriteId);
ID CreateSprite(Engine &engine, const SpriteDesc &desc);
ID CreateSprite(Engine &engine, const BinSpriteDesc &desc);
ID FindSprite(const Scene &scene, const char *name);
ID FindSprite(const Scene &scene, ID textureId, uint2 pos, uint2 size);
ID GetOrCreateSprite(Engine &engine, const SpriteDesc &desc);
void RemoveSprite(Scene &scene, ID spriteId);
void CompactSprites(Scene &scene);


////////////////////////////////////////////////////////////////////////
// Entity management

Entity &GetEntity(ID entityId);
u16 GetEntityIndex(const Scene &scene, ID entityId);
void EntitySetPosition(Entity &entity, float3 position);
EntityDesc GetEntityDesc(Engine &engine, ID entityId);
ID CreateEntity(Engine &engine, const EntityDesc &desc);
ID CreateEntity(Engine &engine, const BinEntityDesc &desc);
void RemoveEntity(Engine &engine, ID entityId);
ID DuplicateEntity(Engine &engine, ID entityId);
void CompactEntities(Scene &scene);

u32 EntityDrawId(const Scene &scene, ID entityId);
ID EntityFromDrawId(u32 drawId);


////////////////////////////////////////////////////////////////////////
// Entity components

bool HasComponents(const Scene &scene, ID entityId, ComponentFlags components);

LightComponent &AddLight(Scene &scene, ID entityId);
void RemoveLight(Scene &scene, ID entityId);
LightComponent &GetLight(Scene &scene, ID entityId);
const LightComponent &GetLight(const Scene &scene, ID entityId);

ParticlesComponent &AddParticles(Scene &scene, ID entityId);
void RemoveParticles(Scene &scene, ID entityId);
ParticlesComponent &GetParticles(Scene &scene, ID entityId);
const ParticlesComponent &GetParticles(const Scene &scene, ID entityId);

ScriptComponent &GetScript(Scene &scene, ID entityId);
const ScriptComponent &GetScript(const Scene &scene, ID entityId);


////////////////////////////////////////////////////////////////////////
// Prefab management

Prefab &GetPrefab(ID prefabId);
u16 GetPrefabIndex(const Scene &scene, ID prefabId);
ID FindPrefab(const Scene &scene, const char *name);
ID CreatePrefab(Engine &engine, const PrefabDesc &desc);
ID CreatePrefab(Engine &engine, const BinPrefabDesc &desc);
void RemovePrefab(Scene &scene, ID prefabId);
void CompactPrefabs(Scene &scene);
ID InstantiatePrefab(Engine &engine, ID prefabId, float3 atPosition);


////////////////////////////////////////////////////////////////////////
// Tile grid

float2 GetWorld2DCoord(const Engine &engine, const Camera &camera, int2 pixelCoord);
int2 GetGridTileCoord(const Engine &engine, const Camera &camera, int2 pixelCoord);
void SetGridTileAtCoord(Engine &engine, Layer &layer, u32 collider, int2 coord);
void SetGridTileAtCoord(Engine &engine, Layer &layer, ID spriteId, int2 coord);
u32 GetColliderAtWorldPos(float2 worldPos);
bool IsColliderInBox(float2 pos, float2 size, u32 collider);


////////////////////////////////////////////////////////////////////////
// Room and layer management

Room &GetRoom(ID roomId);
u16 GetRoomIndex(const Scene &scene, ID roomId);
void CompactRooms(Scene &scene);
Layer &GetLayer(ID layerId);
u32 CreateLayer(Room &room, const LayerDesc &desc);
void RemoveLayer(Room &room, u32 index);
u32 MoveLayer(Room &room, u32 index, i32 delta); // delta -1 moves towards the front, +1 towards the back
const Layer *GetBaseLayer(const Room &room);
float2 LayerSize(const Layer &layer);
float2 RoomSize(const Room &room);
ID CreateRoom(Engine &engine);
ID CreateRoom(Engine &engine, const RoomDesc &desc);
ID CreateRoom(Engine &engine, const BinRoom &binRoom);
void RemoveRoom(Engine &engine, ID roomId);


////////////////////////////////////////////////////////////////////////
// Scene lifetime

void CreateScene(Engine &engine);
void CleanScene(Engine &engine);

////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS: Render
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Immediate draw

void DrawSprite(ID spriteId, float2 worldPos, float4 pcolor);
void DrawBox(float2 pos, float2 size, float4 color);
void DrawBoxOutline(float2 pos, float2 size, float4 color);
void DrawParticles(const Scene &scene);


////////////////////////////////////////////////////////////////////////
// Camera math

float3 UpDirectionFromAngles(const float2 &angles);
float3 ForwardDirectionFromAngles(const float2 &angles);
float3 RightDirectionFromAngles(const float2 &angles);
float4x4 ViewMatrixFromCamera(const Camera &camera);


////////////////////////////////////////////////////////////////////////
// Frame rendering

bool RenderGraphics(Engine &engine);

////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS: Game
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Game -> Engine interface

Entity &GetSelf();
ID FindEntity(const char *name);
Entity *TryGetEntity(ID entityId); // Null once the entity is gone
ID FindRoom(const char *name);
Room *TryGetRoom(ID roomId);       // Null once the room is gone

////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS: Data
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Descriptor parsing

#if USE_DATA_BUILD
void CompileShaders();
bool CompileModifiedShaders();
void SaveAssetDescriptors(const char *path, const AssetDescriptors &assetDescriptors);
AssetDescriptors ParseDescriptors(const char *filepath, Arena &arena);
#endif // USE_DATA_BUILD

////////////////////////////////////////////////////////////////////////
// Asset files

#if USE_DATA_BUILD
void BuildAssets(const AssetDescriptors &assetDescriptors, const char *filepath, Arena tempArena);
#endif // USE_DATA_BUILD

BinAssets OpenAssets(Arena &dataArena, const char *filepath);
void CloseAssets(BinAssets &assets);

////////////////////////////////////////////////////////////////////////
// Data arena state

bool PushDataArenaState(Engine &engine);
bool PopDataArenaState(Engine &engine);


////////////////////////////////////////////////////////////////////////
// Scene serialization

void LoadShadersFromBin(Engine &engine);
void LoadSceneFromBin(Engine &engine);

#if USE_DATA_BUILD
void LoadSceneFromTxt(Engine &engine, const char *filepath);
void SaveSceneToTxt(Engine &engine, const char *filepath);
void BuildShaders(Engine &engine, const char *outBinFilepath);
void BuildAssetsFromTxt(Engine &engine, const char *inTxtFilepath, const char *outBinFilepath);
#endif // USE_DATA_BUILD

#endif // ENGINE_H
