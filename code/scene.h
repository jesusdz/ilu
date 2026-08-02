#ifndef SCENE_H
#define SCENE_H

struct Engine;

struct Sprite
{
	const char *name;
	TextureH textureH;
	uint2 pos;
	uint2 size;
	u32 frameCount;
	u32 fps;
	bool loop;
};

typedef Handle SpriteH;

struct SpriteAnimState
{
	f32 elapsedTime;
	u32 currentFrame;
};

struct Entity
{
	const char *name;
	float3 position;
	float scale;
	bool visible;
	bool culled;
	// 3D entity
	GeometryType geometryType;
	BufferChunk vertices;
	BufferChunk indices;
	MaterialH materialH;
	// Sprite entity
	SpriteH spriteH;
	i32 layer;

	Entity *next; // Pointer to sibling in the hierarchy
	Entity *child; // Pointer to children in the hierarchy
};

#define PIXELS_PER_METER 16
#define TILE_GRID_SIZE_X 40
#define TILE_GRID_SIZE_Y 30
#define TILE_SIZE_PIXELS 16.0f // size of each grid cell, in pixels (at PIXELS_PER_METER scale)

union Cell
{
	Handle handle;
	u32 collider;
};

struct Layer
{
	bool initialized;
	const char *name;
	i32 order; // draw order within the room, lower values drawn first (further back)
	bool visible;
	bool isCollider;
	uint2 size;
	Cell cells[TILE_GRID_SIZE_X][TILE_GRID_SIZE_Y]; // sprite per cell, InvalidHandle if empty
};

// MAX_LAYERS is defined in data.h (RoomDesc needs it)

struct Room
{
	const char *name;
	int2 pos;
	Layer layers[MAX_LAYERS];
	u32 layerCount;
};

#define MAX_ENTITIES 4092
#define MAX_SPRITES 4092
#define MAX_ROOMS 256
#define MAX_TILES 16 * 16 * 8 * MAX_ROOMS

constexpr u32 SCENE_WIDTH = 320;
constexpr u32 SCENE_HEIGHT = 180;

struct Scene
{
	Room rooms[MAX_ROOMS];
	HandleManager roomHandles;

	Entity entities[MAX_ENTITIES];
	HandleManager entityHandles;

	Sprite sprites[MAX_SPRITES];
	HandleManager spriteHandles;

	SpriteAnimState spriteAnimStates[MAX_SPRITES];
};


////////////////////////////////////////////////////////////////////////
// Sprite management

SpriteH CreateSprite(Engine &engine, const SpriteDesc &desc);
SpriteH CreateSprite(Engine &engine, const BinSpriteDesc &desc);
Sprite &GetSprite(Scene &scene, SpriteH handle);
const SpriteDesc GetSpriteDesc(Scene &scene, SpriteH handle);
SpriteH GetOrCreateSprite(Engine &engine, const SpriteDesc &desc);
void RemoveSprite(Scene &scene, SpriteH handle);


////////////////////////////////////////////////////////////////////////
// Entity management

Entity &GetEntity(Scene &scene, Handle handle);
void EntitySetPosition(Entity &entity, float3 position);
EntityDesc GetEntityDesc(Scene &scene, Handle handle);
Handle CreateEntity(Engine &engine, const EntityDesc &desc);
Handle CreateEntity(Engine &engine, const BinEntityDesc &desc);
void RemoveEntity(Engine &engine, Handle handle);
Handle DuplicateEntity(Engine &engine, Handle entityHandle);


////////////////////////////////////////////////////////////////////////
// Tile grid

float2 GetWorld2DCoord(const Engine &engine, const Camera &camera, int2 pixelCoord);
int2 GetGridTileCoord(const Engine &engine, const Camera &camera, int2 pixelCoord);
void SetGridTileAtCoord(Engine &engine, Layer &layer, u32 collider, int2 coord);
void SetGridTileAtCoord(Engine &engine, Layer &layer, SpriteH spriteH, int2 coord);
u32 GetColliderAtWorldPos(float2 worldPos);
bool IsColliderInBox(float2 pos, float2 size, u32 collider);


////////////////////////////////////////////////////////////////////////
// Room and layer management

Room &GetRoom(Scene &scene, Handle handle);
u32 CreateLayer(Room &room, const LayerDesc &desc);
void RemoveLayer(Room &room, u32 index);
float2 LayerSize(const Layer &layer);
float2 RoomSize(const Room &room);
Handle CreateRoom(Engine &engine);
Handle CreateRoom(Engine &engine, const RoomDesc &desc, const SpriteH *spriteHandles, u32 spriteHandleCount);
Handle CreateRoom(Engine &engine, const BinRoom &binRoom, const SpriteH *spriteHandles, u32 spriteHandleCount);
void RemoveRoom(Engine &engine, Handle handle);


////////////////////////////////////////////////////////////////////////
// Scene lifetime
//
// The Clean* callbacks are passed to ForAllHandles by CleanScene.

void CleanTexture(Handle handle, void* data);
void CleanMaterial(Handle handle, void* data);
void CleanEntity(Handle handle, void* data);
void CleanSprite(Handle handle, void* data);
void CleanAudioClip(Handle handle, void* data);

void CreateScene(Engine &engine);
void CleanScene(Engine &engine);

#endif // SCENE_H
