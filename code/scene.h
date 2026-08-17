#ifndef SCENE_H
#define SCENE_H

struct Engine;

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
	float3 position;
	float scale;
	bool visible;
	bool culled;
	// 3D entity
	GeometryType geometryType;
	BufferChunk vertices;
	BufferChunk indices;
	ID materialId;
	// Sprite entity
	ID spriteId;
	bool flipX;

	ID layerId;

	// Hierarchy links. IDs rather than pointers: the element array compacts, so a
	// pointer into it would dangle at the next CompactEntities.
	ID next;  // Sibling
	ID child; // First child
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
};

// MAX_LAYERS is defined in data.h (RoomDesc needs it)

struct Room
{
	ID id;
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
	u32 roomCount;
	Room rooms[MAX_ROOMS];

	u32 entityCount;
	Entity entities[MAX_ENTITIES];

	u32 spriteCount;
	Sprite sprites[MAX_SPRITES];
	SpriteAnimState spriteAnimStates[MAX_SPRITES]; // Parallel to sprites
};


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
EntityDesc GetEntityDesc(ID entityId);
ID CreateEntity(Engine &engine, const EntityDesc &desc);
ID CreateEntity(Engine &engine, const BinEntityDesc &desc);
void RemoveEntity(Engine &engine, ID entityId);
ID DuplicateEntity(Engine &engine, ID entityId);
void CompactEntities(Scene &scene);

u32 EntityDrawId(const Scene &scene, ID entityId);
ID EntityFromDrawId(u32 drawId);


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
