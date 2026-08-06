EntityH::operator bool() const { return IsValidHandle(engine->scene.entityHandles, *this); }
RoomH::operator bool()   const { return IsValidHandle(engine->scene.roomHandles, *this); }

Sprite &GetSprite(ID id)
{
	ASSERT( Valid(id) );
	Sprite &sprite = *((Sprite*)GetObject(id));
	return sprite;
}

// Sprites keep a dense index, unlike textures: the GPU sprite data buffer and the
// parallel animation state array are both addressed by it.
u16 GetSpriteIndex(const Scene &scene, ID id)
{
	const Sprite &sprite = GetSprite(id);
	const u16 index = (u16)(&sprite - scene.sprites);
	ASSERT( index < scene.spriteCount );
	return index;
}

ID CreateSprite(Engine &engine, ID existingId)
{
	Scene &scene = engine.scene;

	if ( scene.spriteCount == MAX_SPRITES )
	{
		LOG(Warning, "Could not create sprite, the sprite array is full.\n");
		return {};
	}

	ID id = existingId;
	if ( id.slot == 0 ) {
		id = NewID();
	} else {
		ReserveID(id);
	}

	const u16 index = (u16)scene.spriteCount++;
	Sprite &sprite = scene.sprites[index];
	sprite = {};
	sprite.desc.id = id;
	scene.spriteAnimStates[index] = {}; // The element held another sprite's animation before

	SetObject(id, &sprite);

	return id;
}

ID CreateSprite(Engine &engine, const SpriteDesc &desc)
{
	Graphics &gfx = engine.gfx;

	const ID id = CreateSprite(engine, desc.id);
	if ( !id ) {
		return id;
	}

	Sprite &sprite = GetSprite(id);
	sprite.desc = desc;
	sprite.desc.id = id;
	sprite.desc.frameCount = desc.frameCount > 0 ? desc.frameCount : 1;

	if ( !Valid(sprite.desc.textureId) )
	{
		LOG(Warning, "Sprite <%s> refers to texture ID %u, which does not exist.\n",
				desc.name, desc.textureId.slot);
		sprite.desc.textureId = gfx.defaultTexture;
	}

	const Texture &tex = GetTexture(sprite.desc.textureId);
	if ( desc.size.x == 0 && desc.size.y == 0 ) {
		sprite.desc.size = tex.size;
	}

	return id;
}

ID CreateSprite(Engine &engine, const BinSpriteDesc &desc)
{
	const SpriteDesc txtDesc = {
		.id          = desc.id,
		.name        = desc.name,
		.textureId   = desc.textureId,
		.pos         = desc.pos,
		.size        = desc.size,
		.frameCount  = desc.frameCount,
		.fps         = desc.fps,
		.loop        = desc.loop,
	};
	return CreateSprite(engine, txtDesc);
};

ID FindSprite(const Scene &scene, const char *name)
{
	if (!name) return {};
	for (u32 i = 0; i < scene.spriteCount; ++i)
	{
		const SpriteDesc &desc = scene.sprites[i].desc;
		if (StrEq(desc.name, name)) return desc.id;
	}
	return {};
}

ID FindSprite(const Scene &scene, ID textureId, uint2 pos, uint2 size)
{
	for (u32 i = 0; i < scene.spriteCount; ++i)
	{
		const SpriteDesc &desc = scene.sprites[i].desc;
		if (desc.textureId == textureId &&
			desc.pos.x == pos.x && desc.pos.y == pos.y &&
			desc.size.x == size.x && desc.size.y == size.y) return desc.id;
	}
	return {};
}

ID GetOrCreateSprite(Engine &engine, const SpriteDesc &desc)
{
	ID id = FindSprite(engine.scene, desc.name);
	if (!id)
		id = CreateSprite(engine, desc);
	return id;
}

void RemoveSprite(Scene &scene, ID id)
{
	if ( IsBuiltin(id) )
	{
		LOG(Warning, "Ignoring an attempt to remove builtin sprite <%s>.\n", GetSprite(id).desc.name);
		return;
	}

	if (id)
	{
		// Marks only. The sprite keeps its element until CompactSprites, so the tiles
		// and entities still drawing it this frame have something valid to read.
		GetSprite(id).desc.id = {};
		Invalidate(id);
	}
}

void CompactSprites(Scene &scene)
{
	u32 storeIndex = U32_MAX;
	for (u32 i = 0; i < scene.spriteCount; ++i)
	{
		if ( !scene.sprites[i].desc.id ) {
			storeIndex = i;
			break;
		}
	}

	if ( storeIndex == U32_MAX ) {
		return;
	}

	for (u32 readIndex = storeIndex; readIndex < scene.spriteCount; ++readIndex)
	{
		Sprite &readSprite = scene.sprites[readIndex];

		if ( !readSprite.desc.id )
		{
			readSprite = {};
			continue;
		}

		const u32 writeIndex = storeIndex++;
		Sprite &writeSprite = scene.sprites[writeIndex];
		writeSprite = readSprite;
		scene.spriteAnimStates[writeIndex] = scene.spriteAnimStates[readIndex];
		readSprite = {};

		SetObject(writeSprite.desc.id, &writeSprite);
	}

	scene.spriteCount = storeIndex;
}


////////////////////////////////////////////////////////////////////////
// Room management

Room &GetRoom(Scene &scene, RoomH handle)
{
	ASSERT( IsValidHandle(scene.roomHandles, handle) );
	Room &room = scene.rooms[GetHandleIndex(scene.roomHandles, handle)];
	return room;
}

u16 GetRoomIndex(const Scene &scene, RoomH handle)
{
	ASSERT( IsValidHandle(scene.roomHandles, handle) );
	return GetHandleIndex(scene.roomHandles, handle);
}

static MOVE_ELEMENT(MoveRoom)
{
	Scene &scene = *(Scene*)data;
	scene.rooms[dstIndex] = scene.rooms[srcIndex];
}

void CompactRooms(Scene &scene)
{
	CompactPool(scene.roomHandles, MoveRoom, nullptr, &scene);
}


////////////////////////////////////////////////////////////////////////
// Entity management

Entity &GetEntity(Scene &scene, EntityH handle)
{
	ASSERT( IsValidHandle(scene.entityHandles, handle) );
	Entity &entity = scene.entities[GetHandleIndex(scene.entityHandles, handle)];
	return entity;
}

u16 GetEntityIndex(const Scene &scene, EntityH handle)
{
	ASSERT( IsValidHandle(scene.entityHandles, handle) );
	return GetHandleIndex(scene.entityHandles, handle);
}

u32 EntityDrawId(const Scene &scene, EntityH handle)
{
	const u32 index = GetEntityIndex(scene, handle);
	const u32 drawId = (index << 16) | handle.gen;
	return drawId;
}

EntityH EntityHandleFromDrawId(const Scene &scene, u32 drawId)
{
	const u16 index = (u16)(drawId >> 16);
	const u16 gen = (u16)(drawId & 0xFFFF);

	EntityH handle = InvalidHandle;
	if ( index < HandleCount(scene.entityHandles) )
	{
		const EntityH candidate = GetHandleAt(scene.entityHandles, index);
		// The readback is a frame behind, so the element may belong to someone else now
		if ( candidate.gen == gen ) {
			handle = candidate;
		}
	}
	return handle;
}

static MOVE_ELEMENT(MoveEntity)
{
	Scene &scene = *(Scene*)data;
	scene.entities[dstIndex] = scene.entities[srcIndex];
}

void CompactEntities(Scene &scene)
{
	CompactPool(scene.entityHandles, MoveEntity, nullptr, &scene);
}

void EntitySetPosition(Entity &entity, float3 position)
{
	entity.position = position;
}

EntityDesc GetEntityDesc(Scene &scene, EntityH handle)
{
	ASSERT( IsValidHandle(scene.entityHandles, handle) );
	const Entity &entity = GetEntity(scene, handle);
	EntityDesc entityDesc = {
		.name  = entity.name,
		.layer = entity.layer,
		.pos   = entity.position,
		.scale = entity.scale,
	};
	if (entity.spriteId) {
		entityDesc.spriteId = entity.spriteId;
	} else {
		entityDesc.materialId = entity.materialId;
		entityDesc.geometryType = entity.geometryType;
	}
	return entityDesc;
}

EntityH CreateEntity(Engine &engine, const EntityDesc &desc)
{
	Scene &scene = engine.scene;

	BufferChunk vertices = GetVerticesForGeometryType(engine.gfx, desc.geometryType);
	BufferChunk indices = GetIndicesForGeometryType(engine.gfx, desc.geometryType);

	EntityH handle = NewHandle(scene.entityHandles);
	Entity &entity = GetEntity(scene, handle);
	entity = {}; // The element held another entity before
	entity.name = desc.name;
	entity.visible = true;
	EntitySetPosition(entity, desc.pos);
	entity.scale = desc.scale;
	entity.layer = desc.layer;
	entity.geometryType = desc.geometryType;
	entity.vertices = vertices;
	entity.indices = indices;
	entity.materialId = desc.materialId;
	entity.spriteId = desc.spriteId;

	// An entity carries either a material or a sprite, so only an ID that was set and
	// then failed to resolve is worth complaining about
	if ( desc.materialId.slot != 0 && !Valid(desc.materialId) )
	{
		LOG(Warning, "Entity <%s> refers to material ID %u, which does not exist.\n",
				desc.name, desc.materialId.slot);
		entity.materialId = engine.gfx.defaultMaterial;
	}
	if ( desc.spriteId.slot != 0 && !Valid(desc.spriteId) )
	{
		LOG(Warning, "Entity <%s> refers to sprite ID %u, which does not exist.\n",
				desc.name, desc.spriteId.slot);
		entity.spriteId = {};
	}

	return handle;
}

EntityH CreateEntity(Engine &engine, const BinEntityDesc &desc)
{
	const EntityDesc entityDesc = {
		.name = desc.name,
		.materialId = desc.materialId,
		.geometryType = desc.geometryType,
		.spriteId = desc.spriteId,
		.layer = desc.layer,
		.pos = desc.pos,
		.scale = desc.scale,
	};
	return CreateEntity(engine, entityDesc);
}

void RemoveEntity(Engine &engine, EntityH handle)
{
	FreeHandle(engine.scene.entityHandles, handle);
}

EntityH DuplicateEntity(Engine &engine, EntityH entityHandle)
{
	const EntityDesc &desc = GetEntityDesc(engine.scene, entityHandle);
	return CreateEntity(engine, desc);
}


////////////////////////////////////////////////////////////////////////
// Tile grid management

float2 GetWorld2DCoord(const Engine &engine, const Camera &camera, int2 pixelCoord)
{
	const Window &window = *sPlatform->window;
	const uint2 windowSize = { window.width, window.height };
	const float2 uvCoords = {(f32)pixelCoord.x/windowSize.x, 1.0f - (f32)pixelCoord.y/windowSize.y};
	const float2 ndcCoords = 2.0f * uvCoords - float2{1.0f, 1.0f};
	const f32 aspect = (f32)windowSize.x / (f32)windowSize.y;
	const float2 scale = {camera.height * aspect, camera.height};
	const float2 worldCoords = scale * ndcCoords + float2{camera.position.x, camera.position.y};
	return worldCoords;
}

int2 GetGridTileCoord(const Engine &engine, const Camera &camera, int2 pixelCoord)
{
	const float2 worldCoords = GetWorld2DCoord(engine, camera, pixelCoord);
	const f32 cellWorldSize = TILE_SIZE_PIXELS / PIXELS_PER_METER;
	const int2 res = {(i32)Floor(worldCoords.x / cellWorldSize), (i32)Floor(worldCoords.y / cellWorldSize)};
	//LOG(Debug, "Tile coord: (%f, %f)\n", uvCoords.x, uvCoords.y);
	//LOG(Debug, "Tile coord: (%d, %d)\n", res.x, res.y);
	return res;
}

void SetGridTileAtCoord(Engine &engine, Layer &layer, u32 collider, int2 coord)
{
	const bool coordValid = coord.x >= 0 && coord.x < layer.size.x && coord.y >= 0 && coord.y < layer.size.y;
	if (coordValid)
	{
		layer.cells[coord.x][coord.y].collider = collider;
	}
}

void SetGridTileAtCoord(Engine &engine, Layer &layer, ID spriteId, int2 coord)
{
	const bool coordValid = coord.x >= 0 && coord.x < layer.size.x && coord.y >= 0 && coord.y < layer.size.y;
	if (coordValid)
	{
		layer.cells[coord.x][coord.y].spriteId = spriteId;
	}
}

static int2 WorldPosToGridCoord(float2 worldPos)
{
	const f32 cellWorldSize = TILE_SIZE_PIXELS / PIXELS_PER_METER;
	return int2{ (i32)Floor(worldPos.x / cellWorldSize), (i32)Floor(worldPos.y / cellWorldSize) };
}

static u32 GetColliderAtGridCoord(Scene &scene, int2 coord)
{
	for (u16 roomIndex = 0; roomIndex < HandleCount(scene.roomHandles); ++roomIndex)
	{
		const Room &room = scene.rooms[roomIndex];
		const int2 localCoord = coord - room.pos;

		for (u32 i = 0; i < ARRAY_COUNT(room.layers); ++i)
		{
			const Layer &layer = room.layers[i];
			if (!layer.initialized || !layer.isCollider) continue;

			if ( localCoord.x >= 0 && localCoord.x < layer.size.x &&
				localCoord.y >= 0 && localCoord.y < layer.size.y )
			{
				if (layer.cells[localCoord.x][localCoord.y].collider != 0)
				{
					return layer.cells[localCoord.x][localCoord.y].collider;
				}
			}
		}
	}
	return 0;
}

u32 GetColliderAtWorldPos(float2 worldPos)
{
	return GetColliderAtGridCoord(engine->scene, WorldPosToGridCoord(worldPos));
}

// pos is the box's bottom-left corner, size its width/height (same convention as DrawBox).
// Tiles the box only touches at an edge count as colliding.
bool IsColliderInBox(float2 pos, float2 size, u32 collider)
{
	const int2 minCoord = WorldPosToGridCoord(pos);
	const int2 maxCoord = WorldPosToGridCoord(pos + size);

	for (i32 y = minCoord.y; y <= maxCoord.y; ++y)
	{
		for (i32 x = minCoord.x; x <= maxCoord.x; ++x)
		{
			if (GetColliderAtGridCoord(engine->scene, int2{x, y}) == collider)
				return true;
		}
	}
	return false;
}



u32 CreateLayer(Room &room, const LayerDesc &desc)
{
	u32 index = U32_MAX;

	if ( room.layerCount < ARRAY_COUNT(room.layers) )
	{
		for (u32 i = 0; i < ARRAY_COUNT(room.layers); ++i)
		{
			Layer &layer = room.layers[i];
			if (!layer.initialized)
			{
				room.layerCount++;
				layer.initialized = true;
				layer.name = desc.name;
				layer.isBase = desc.isBase;
				layer.visible = desc.visible;
				layer.isCollider = desc.isCollider;
				layer.size = desc.size;
				index = i;
				break;
			}
		}
	}

	return index;
}

void RemoveLayer(Room &room, u32 index)
{
	if ( index < ARRAY_COUNT(room.layers) )
	{
		Layer &layer = room.layers[index];
		// The base layer sets the room size and the parallax reference, so it always stays.
		if (layer.initialized && !layer.isBase)
		{
			layer = {};
			room.layerCount--;
		}
	}
}

u32 MoveLayer(Room &room, u32 index, i32 delta)
{
	if ( delta == 0 || index >= ARRAY_COUNT(room.layers) ) return index;
	if ( !room.layers[index].initialized ) return index;

	const i32 slotCount = (i32)ARRAY_COUNT(room.layers);
	for (i32 i = (i32)index + delta; i >= 0 && i < slotCount; i += delta)
	{
		Layer &neighbour = room.layers[i];
		if (!neighbour.initialized) continue;

		const Layer moved = room.layers[index];
		room.layers[index] = neighbour;
		neighbour = moved;
		return (u32)i;
	}

	return index; // Already at the end it was asked to move towards
}

const Layer *GetBaseLayer(const Room &room)
{
	for (u32 i = 0; i < ARRAY_COUNT(room.layers); ++i)
	{
		const Layer &layer = room.layers[i];
		if (layer.initialized && layer.isBase) return &layer;
	}
	return nullptr;
}

float2 LayerSize(const Layer &layer)
{
	const float2 res = Float2(layer.size);
	return res;
}

float2 RoomSize(const Room &room)
{
	const Layer *baseLayer = GetBaseLayer(room);
	const float2 res = baseLayer ? LayerSize(*baseLayer) : float2{0.0f, 0.0f};
	return res;
}

RoomH CreateRoom(Engine &engine)
{
	RoomH roomH = NewHandle(engine.scene.roomHandles);
	Room &room = GetRoom(engine.scene, roomH);
	room = {}; // The element held another room before

	room.name = InternString("Room");
	room.pos = {};

	const LayerDesc desc1 = { .name = "Layer", .isBase = true, .visible = true, .isCollider = false, .size {TILE_GRID_SIZE_X, TILE_GRID_SIZE_Y} };
	CreateLayer(room, desc1);
	const LayerDesc desc2 = { .name = "Colliders", .visible = true, .isCollider = true, .size {TILE_GRID_SIZE_X, TILE_GRID_SIZE_Y} };
	CreateLayer(room, desc2);

	return roomH;
}

RoomH CreateRoom(Engine &engine, const RoomDesc &desc)
{
	RoomH roomH = NewHandle(engine.scene.roomHandles);
	Room &room = GetRoom(engine.scene, roomH);
	room = {};

	room.name = InternString(desc.name);
	room.pos = desc.pos;

	for (u32 l = 0; l < desc.layerCount; ++l)
	{
		LayerDesc layerDesc = desc.layers[l];
		layerDesc.name = InternString(layerDesc.name);

		const u32 index = CreateLayer(room, layerDesc);
		if (index == U32_MAX) {
			continue;
		}

		Layer &layer = room.layers[index];
		if (layerDesc.isCollider) {
			for (u32 t = 0; t < layerDesc.tileCount; ++t) {
				const TileDesc &tile = layerDesc.tiles[t];
				if (tile.x < layer.size.x && tile.y < layer.size.y) {
					layer.cells[tile.x][tile.y].collider = tile.collider;
				}
			}
		}
		else
		{
			for (u32 t = 0; t < layerDesc.tileCount; ++t) {
				const TileDesc &tile = layerDesc.tiles[t];
				if (tile.x < layer.size.x && tile.y < layer.size.y) {
					if ( tile.spriteId.slot != 0 && !Valid(tile.spriteId) ) {
						LOG(Warning, "Layer <%s> has a tile at (%u, %u) referring to sprite ID %u, which does not exist.\n",
								layerDesc.name, tile.x, tile.y, tile.spriteId.slot);
						continue;
					}
					layer.cells[tile.x][tile.y].spriteId = tile.spriteId;
				}
			}
		}
	}

	return roomH;
}

RoomH CreateRoom(Engine &engine, const BinRoom &binRoom)
{
	const BinRoomDesc &bin = *binRoom.desc;

	RoomDesc desc = {};
	desc.name = bin.name;
	desc.pos = bin.pos;

	for (u32 l = 0; l < bin.layerCount && l < ARRAY_COUNT(desc.layers); ++l)
	{
		const BinLayerDesc &ld = bin.layers[l];
		desc.layers[desc.layerCount++] = {
			.name = ld.name,
			.isBase = ld.isBase != 0,
			.visible = ld.visible != 0,
			.isCollider = ld.isCollider != 0,
			.size = ld.size,
			.tiles = binRoom.tiles[l],
			.tileCount = ld.tiles.size / sizeof(TileDesc),
		};
	}

	return CreateRoom(engine, desc);
}

void RemoveRoom(Engine &engine, RoomH handle)
{
	// Marks only, see RemoveEntity
	FreeHandle(engine.scene.roomHandles, handle);
}

void CreateScene(Engine &engine)
{
	CreateRoom(engine);
}

void CleanScene(Engine &engine)
{
	WaitDeviceIdle(engine.gfx.device);

	if (PopDataArenaState(engine))
	{
	}

	Graphics &gfx = engine.gfx;
	Scene &scene = engine.scene;
	Audio &audio = engine.audio;

	// Mark everything the scene owns
	for (u16 i = 0; i < gfx.textureCount; ++i) {
		if ( !(gfx.textures[i].desc.flags & AssetFlag_Builtin) ) {
			RemoveTexture(gfx, gfx.textures[i].desc.id);
		}
	}
	for (u16 i = 0; i < gfx.materialCount; ++i) {
		if ( !(gfx.materials[i].desc.flags & AssetFlag_Builtin) ) {
			RemoveMaterial(gfx, gfx.materials[i].desc.id);
		}
	}
	for (u16 i = 0; i < HandleCount(scene.roomHandles); ++i) {
		RemoveRoom(engine, GetHandleAt(scene.roomHandles, i));
	}
	for (u16 i = 0; i < HandleCount(scene.entityHandles); ++i) {
		RemoveEntity(engine, GetHandleAt(scene.entityHandles, i));
	}
	for (u16 i = 0; i < scene.spriteCount; ++i) {
		RemoveSprite(scene, scene.sprites[i].desc.id);
	}
	for (u16 i = 0; i < HandleCount(audio.clipHandles); ++i) {
		RemoveAudioClip(engine, GetHandleAt(audio.clipHandles, i));
	}

	// Compaction after removal
	CompactRooms(engine.scene);
	CompactEntities(engine.scene);
	CompactSprites(engine.scene);
	CompactMaterials(engine.gfx);
	CompactTextures(engine.gfx);
	// The audio pools are not compacted here: only the mixing thread may move that

	CloseAssets(engine.assets);

	engine.gfx.shouldUpdateMaterials = true;
	engine.gfx.shouldUpdateMaterialBindGroups = true;
}
