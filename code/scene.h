#ifndef SCENE_H
#define SCENE_H

struct Engine;

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

// A named, instantiable template: a fixed set of EntityDescs spawned together and
// offset by a world position. No hierarchy links between them yet, matching Entity's.
struct Prefab
{
	ID id;
	const char *name;
	EntityDesc entities[MAX_PREFAB_ENTITIES];
	u32 entityCount;
};

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

#endif // SCENE_H
