
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

GAME_API
void SetCamera(const Camera &camera)
{
	Engine &engine = GetEngine();
	engine.gfx.camera = camera;
}

GAME_API
ID GetDefaultRoomId()
{
	return GetEngine().scene.defaultRoomId;
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

