
#define GAME_API static

GAME_API
Entity &GetSelf()
{
	Game &game = GetGame();
	Entity &entity = GetEntity(game.currentEntity);
	return entity;
}

// The entity entering or exiting the region, only valid inside OnTriggerEnter/OnTriggerExit
GAME_API
Entity &GetOther()
{
	Game &game = GetGame();
	Entity &entity = GetEntity(game.otherEntity);
	return entity;
}

GAME_API
const GameInput &GetInput()
{
	return GetGame().input;
}

GAME_API
ID GetEntitySprite(ID entityId)
{
	return EntitySpriteId(GetEngine().scene, entityId);
}

GAME_API
void SetEntitySprite(ID entityId, ID spriteId)
{
	Engine &engine = GetEngine();
	if ( !HasComponents(engine.scene, entityId, Component_Sprite) ) {
		AddComponent(engine, entityId, ComponentType_Sprite);
	}
	GetSprite(engine.scene, entityId).spriteId = spriteId;
}

GAME_API
SpriteComponent &GetSpriteComponent(ID entityId)
{
	return GetSprite(GetEngine().scene, entityId);
}

GAME_API
ColliderComponent &GetColliderComponent(ID entityId)
{
	return GetCollider(GetEngine().scene, entityId);
}

GAME_API
u32 GetCameraRegionCount()
{
	return GetEngine().scene.componentPools[ComponentType_CameraRegion].count;
}

GAME_API
const CameraRegionComponent &GetCameraRegion(u32 index)
{
	const Scene &scene = GetEngine().scene;
	ASSERT( index < scene.componentPools[ComponentType_CameraRegion].count );
	return scene.cameraRegionComponents[index];
}

GAME_API
f32 GetScenePixelsPerMeter()
{
	return GetEngine().gfx.renderTargets.scenePixelsPerMeter;
}

// Rendering blends the camera here from where it was on the previous step
GAME_API
void MoveCamera(const Camera &camera)
{
	Engine &engine = GetEngine();
	engine.gfx.camera = camera;
}

// Places the camera with no blend, like EntitySetPosition does for an entity
GAME_API
void SetCamera(const Camera &camera)
{
	Engine &engine = GetEngine();
	engine.gfx.camera = camera;
	engine.game.prevCameraPosition = camera.position;
}

GAME_API
ID GetDefaultRoomId()
{
	return GetEngine().scene.defaultRoomId;
}

// Game state: SetNextXXX takes effect next step

GAME_API
ID GetCurrentRoomId()
{
	return GetGame().curr.roomId;
}

GAME_API
void SetNextRoomId(ID roomId)
{
	GetGame().next.roomId = roomId;
}

// True on the step after EntityTeleport
GAME_API
bool CurrentlyTeleporting()
{
	return GetGame().curr.isTeleporting;
}

// True for one step, including the first
GAME_API
bool CurrentRoomChanged()
{
	const Game &game = GetGame();
	const bool changed = game.curr.roomId != game.prev.roomId;
	return changed;
}

GAME_API
float2 GetSpawnPosition()
{
	float2 spawnPosition = {};
	if ( GetGame().curr.isSpawning )
	{
		spawnPosition = GetGame().curr.spawnPosition;
	}
	else
	{
		ID roomId = GetCurrentRoomId();
		// TODO: Look for teleports and get the first one in this room
	}
	return spawnPosition;
}

GAME_API
void SetNextSpawnPosition(float2 pos)
{
	GetGame().next.isSpawning = true;
	GetGame().next.spawnPosition = pos;
}

// Nullable counterpart to GetRoom, which asserts. Game code holds an ID across frames
// and has to cope with the room going away.
GAME_API
Room *TryGetRoom(ID roomId)
{
	Room *roomPtr = nullptr;
	if ( roomId ) {
		roomPtr = &GetRoom(roomId);
	}
	return roomPtr;
}

// See TryGetRoom
GAME_API
Entity *TryGetEntity(ID entityId)
{
	Entity *ent = nullptr;
	if ( entityId ) {
		ent = &GetEntity(entityId);
	}
	return ent;
}

GAME_API
u32 PlayAudioClip(ID clipId)
{
	Engine &engine = GetEngine();
	u32 ret = PlayAudioClip(engine.audio, clipId);
	return ret;
}

GAME_API
void PlayMusic(ID musicId)
{
	Engine &engine = GetEngine();
	MusicPlay(engine, musicId);
}

