SpriteH CreateSprite(Engine &engine, const SpriteDesc &desc)
{
	Scene &scene = engine.scene;
	Graphics &gfx = engine.gfx;

	const TextureH textureH = FindTextureHandle(gfx, desc.textureName);

	Sprite sprite = {};
	sprite.name       = desc.name;
	sprite.textureH   = textureH;
	sprite.frameCount = desc.frameCount > 0 ? desc.frameCount : 1;
	sprite.fps        = desc.fps;
	sprite.loop       = desc.loop != 0;

	{
		const Texture &tex = GetTexture(gfx, textureH);
		const uint2 size = (desc.size.x > 0 || desc.size.y > 0) ? desc.size : tex.size;
		sprite.pos  = desc.pos;
		sprite.size = size;
	}

	SpriteH handle = NewHandle(scene.spriteHandles);
	scene.sprites[handle.idx] = sprite;
	return handle;
}

SpriteH CreateSprite(Engine &engine, const BinSpriteDesc &desc)
{
	const SpriteDesc txtDesc = {
		.name        = desc.name,
		.textureName = desc.textureName,
		.pos         = desc.pos,
		.size        = desc.size,
		.frameCount  = desc.frameCount,
		.fps         = desc.fps,
		.loop        = desc.loop,
	};
	return CreateSprite(engine, txtDesc);
};

Sprite &GetSprite(Scene &scene, SpriteH handle)
{
	ASSERT( IsValidHandle(scene.spriteHandles, handle) );
	return scene.sprites[handle.idx];
}

const SpriteDesc GetSpriteDesc(Scene &scene, SpriteH handle)
{
	const Sprite &sprite = GetSprite(scene, handle);
	const Texture &tex = GetTexture(engine->gfx, sprite.textureH);

	const SpriteDesc desc = {
		.name = sprite.name,
		.textureName = tex.name,
		.pos = sprite.pos,
		.size = sprite.size,
		.frameCount = sprite.frameCount,
		.fps = sprite.fps,
		.loop = sprite.loop,
	};
	return desc;
}

static SpriteH FindSpriteHandle(const Scene &scene, const char *name)
{
	if (!name) return InvalidHandle;
	for (HandleIter it = BeginIter(scene.spriteHandles); it; it++)
	{
		Handle handle = *it;
		if (StrEq(scene.sprites[handle.idx].name, name)) return handle;
	}
	return InvalidHandle;
}

SpriteH GetOrCreateSprite(Engine &engine, const SpriteDesc &desc)
{
	SpriteH handle = FindSpriteHandle(engine.scene, desc.name);
	if (handle == InvalidHandle)
		handle = CreateSprite(engine, desc);
	return handle;
}

void RemoveSprite(Scene &scene, SpriteH handle)
{
	scene.sprites[handle.idx] = {};
	FreeHandle(scene.spriteHandles, handle);
}


////////////////////////////////////////////////////////////////////////
// Room management

Room &GetRoom(Scene &scene, Handle handle)
{
	ASSERT( IsValidHandle(scene.roomHandles, handle) );
	Room &room = scene.rooms[handle.idx];
	return room;
}


////////////////////////////////////////////////////////////////////////
// Entity management

Entity &GetEntity(Scene &scene, Handle handle)
{
	ASSERT( IsValidHandle(scene.entityHandles, handle) );
	Entity &entity = scene.entities[handle.idx];
	return entity;
}

void EntitySetPosition(Entity &entity, float3 position)
{
	entity.position = position;
}

EntityDesc GetEntityDesc(Scene &scene, Handle handle)
{
	ASSERT( IsValidHandle(scene.entityHandles, handle) );
	const Entity &entity = GetEntity(scene, handle);
	EntityDesc entityDesc = {
		.name  = entity.name,
		.layer = entity.layer,
		.pos   = entity.position,
		.scale = entity.scale,
	};
	if (IsValidHandle(scene.spriteHandles, entity.spriteH)) {
		entityDesc.spriteName = GetSprite(scene, entity.spriteH).name;
	} else {
		const Material &material = GetMaterial(engine->gfx, entity.materialH);
		entityDesc.materialName = material.name;
		entityDesc.geometryType = entity.geometryType;
	}
	return entityDesc;
}

Handle CreateEntity(Engine &engine, const EntityDesc &desc)
{
	Scene &scene = engine.scene;

	BufferChunk vertices = GetVerticesForGeometryType(engine.gfx, desc.geometryType);
	BufferChunk indices = GetIndicesForGeometryType(engine.gfx, desc.geometryType);

	Handle handle = NewHandle(scene.entityHandles);
	Entity &entity = GetEntity(scene, handle);
	entity.name = desc.name;
	entity.visible = true;
	EntitySetPosition(entity, desc.pos);
	entity.scale = desc.scale;
	entity.layer = desc.layer;
	entity.geometryType = desc.geometryType;
	entity.vertices = vertices;
	entity.indices = indices;
	entity.materialH = FindMaterialHandle(engine.gfx, desc.materialName);
	entity.spriteH = FindSpriteHandle(scene, desc.spriteName);

	return handle;
}

Handle CreateEntity(Engine &engine, const BinEntityDesc &desc)
{
	const EntityDesc entityDesc = {
		.name = desc.name,
		.materialName = desc.materialName,
		.geometryType = desc.geometryType,
		.spriteName = desc.spriteName,
		.layer = desc.layer,
		.pos = desc.pos,
		.scale = desc.scale,
	};
	return CreateEntity(engine, entityDesc);
}

void RemoveEntity(Engine &engine, Handle handle)
{
	Entity &entity = GetEntity(engine.scene, handle);
	entity = {};

	FreeHandle(engine.scene.entityHandles, handle);
}

Handle DuplicateEntity(Engine &engine, Handle entityHandle)
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
	const bool coordValid = coord.x >= 0 && coord.x < layer.size.x &&
			coord.y >= 0 && coord.y < layer.size.y;
	if (coordValid)
	{
		layer.cells[coord.x][coord.y].collider = collider;
	}
}

void SetGridTileAtCoord(Engine &engine, Layer &layer, SpriteH spriteH, int2 coord)
{
	const bool coordValid = coord.x >= 0 && coord.x < layer.size.y &&
			coord.y >= 0 && coord.y < layer.size.y;
	if (coordValid)
	{
		layer.cells[coord.x][coord.y].handle = spriteH;
	}
}

static int2 WorldPosToGridCoord(float2 worldPos)
{
	const f32 cellWorldSize = TILE_SIZE_PIXELS / PIXELS_PER_METER;
	return int2{ (i32)Floor(worldPos.x / cellWorldSize), (i32)Floor(worldPos.y / cellWorldSize) };
}

static u32 GetColliderAtGridCoord(Scene &scene, int2 coord)
{
	for (HandleIter it = BeginIter(scene.roomHandles); it; it++)
	{
		const Room &room = GetRoom(scene, *it);
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


static void CleanRoom(Handle handle, void *data)
{
	Engine &engine = *(Engine*)data;
	RemoveRoom(engine, handle);
}


void CleanTexture(Handle handle, void* data)
{
	Engine &engine = *(Engine*)data;
	const TextureDesc &desc = GetTextureDesc( engine.gfx, handle);
	if ( !(desc.flags & AssetFlag_Builtin) ) {
		RemoveTexture(engine.gfx, handle);
	}
}

void CleanMaterial(Handle handle, void* data)
{
	Engine &engine = *(Engine*)data;
	const MaterialDesc &desc = GetMaterialDesc( engine.gfx, handle);
	if ( !(desc.flags & AssetFlag_Builtin) ) {
		RemoveMaterial(engine.gfx, handle);
	}
}

void CleanEntity(Handle handle, void* data)
{
	Engine &engine = *(Engine*)data;
	RemoveEntity(engine, handle);
}

void CleanSprite(Handle handle, void* data)
{
	Engine &engine = *(Engine*)data;
	RemoveSprite(engine.scene, handle);
}


void CleanAudioClip(Handle handle, void* data)
{
	Engine &engine = *(Engine*)data;
	RemoveAudioClip(engine, handle);
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
				layer.order = desc.order;
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
	if ( room.layerCount > 1 && index < ARRAY_COUNT(room.layers) )
	{
		Layer &layer = room.layers[index];
		if (layer.initialized)
		{
			layer = {};
			room.layerCount--;
		}
	}
}

float2 LayerSize(const Layer &layer)
{
	const float2 res = Float2(layer.size);
	return res;
}

float2 RoomSize(const Room &room)
{
	const float2 res = LayerSize(room.layers[0]);
	return res;
}

Handle CreateRoom(Engine &engine)
{
	Handle roomH = NewHandle(engine.scene.roomHandles);
	Room &room = engine.scene.rooms[roomH.idx];

	room.name = InternString("Room");
	room.pos = {};

	const LayerDesc desc1 = { .name = "Layer", .visible = true, .isCollider = false, .size {TILE_GRID_SIZE_X, TILE_GRID_SIZE_Y} };
	CreateLayer(room, desc1);
	const LayerDesc desc2 = { .name = "Colliders", .visible = true, .isCollider = true, .size {TILE_GRID_SIZE_X, TILE_GRID_SIZE_Y} };
	CreateLayer(room, desc2);

	return roomH;
}

Handle CreateRoom(Engine &engine, const RoomDesc &desc, const SpriteH *spriteHandles, u32 spriteHandleCount)
{
	Handle roomH = NewHandle(engine.scene.roomHandles);
	Room &room = engine.scene.rooms[roomH.idx];
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
					if (tile.spriteIndex < spriteHandleCount) {
						layer.cells[tile.x][tile.y].handle = spriteHandles[tile.spriteIndex];
					}
				}
			}
		}
	}

	return roomH;
}

Handle CreateRoom(Engine &engine, const BinRoom &binRoom, const SpriteH *spriteHandles, u32 spriteHandleCount)
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
			.order = ld.order,
			.visible = ld.visible != 0,
			.isCollider = ld.isCollider != 0,
			.size = ld.size,
			.tiles = binRoom.tiles[l],
			.tileCount = ld.tiles.size / sizeof(TileDesc),
		};
	}

	return CreateRoom(engine, desc, spriteHandles, spriteHandleCount);
}

void RemoveRoom(Engine &engine, Handle handle)
{
	Room &room = GetRoom(engine.scene, handle);
	room = {};
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

	ForAllHandles(engine.gfx.textureHandles, CleanTexture, &engine);
	ForAllHandles(engine.gfx.materialHandles, CleanMaterial, &engine);
	ForAllHandles(engine.scene.roomHandles, CleanRoom, &engine);
	ForAllHandles(engine.scene.entityHandles, CleanEntity, &engine);
	ForAllHandles(engine.scene.spriteHandles, CleanSprite, &engine);
	ForAllHandles(engine.audio.clipHandles, CleanAudioClip, &engine);
	CloseAssets(engine.assets);

	engine.gfx.shouldUpdateMaterials = true;
	engine.gfx.shouldUpdateMaterialBindGroups = true;
}
