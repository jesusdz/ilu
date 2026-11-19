
constexpr f32 LOW_SCENE_WIDTH = 320;
constexpr f32 LOW_SCENE_HEIGHT = 180;

REFLEX(Script)
struct GameController
{
	REFLEX(Room)
	ID startRoom;

	REFLEX(Entity)
	ID playerId;

	ID nextRoomId; // Set by anyone, applied and cleared by Commit
	float2 nextSpawnPosition;
};

enum PlayerState
{
	OnFloor,
	OnPlatform,
	OnLadder,
	OnAir,
};

REFLEX(Script)
struct PlayerController
{
	PlayerState playerState;

	REFLEX(Sprite)
	ID sprPlayerIdle;

	REFLEX(Sprite)
	ID sprPlayerRun;

	REFLEX(Sprite)
	ID sprPlayerJump;

	REFLEX(Sprite)
	ID sprPlayerFall;

	REFLEX(AudioClip)
	ID sndJump;

	float2 speed;
	f32 accel;
};

REFLEX(Script)
struct CameraController
{
	REFLEX(Entity)
	ID playerId;

	Camera camera;
	f32 cameraTargetY;

	bool cutToTarget;
	float2 boundsMin;
	float2 boundsMax;
};

REFLEX(Script)
struct Teleport
{
	REFLEX(Entity)
	ID teleportId;
};

// Singletons. Also assigned every step, as a hot-reload resets them and may move script data.
static GameController *sGameController = nullptr;
#define gGameController (*sGameController)

static CameraController *sCameraController = nullptr;
#define gCameraController (*sCameraController)

REFLEX()
void Start(GameController &script)
{
	sGameController = &script;
	script.nextRoomId = script.startRoom ? script.startRoom : GetDefaultRoomId();
	script.nextSpawnPosition = FindSpawnPosition(script.nextRoomId);
}

REFLEX()
void Commit(GameController &script)
{
	sGameController = &script;

	if ( script.nextRoomId )
	{
		SetCurrentRoom(script.nextRoomId);
		if ( Entity *player = TryGetEntity(script.playerId) ) {
			EntityTeleport(*player, float3{script.nextSpawnPosition.x, script.nextSpawnPosition.y, player->position.z});
		}
		if ( sCameraController ) { // Null for the first step after a hot-reload
			gCameraController.cutToTarget = true;
		}
		script.nextRoomId = {};
	}
}

REFLEX()
void OnTriggerEnter(Teleport &script)
{
	const Entity *target = TryGetEntity(script.teleportId);
	if ( target && GetOther().id == gGameController.playerId ) {
		gGameController.nextRoomId = target->roomId;
		gGameController.nextSpawnPosition = target->position.xy;
	}
}

REFLEX()
void Start(PlayerController &script)
{
	script.playerState = OnAir;
	script.speed = {};
	script.accel = 50;
}

REFLEX()
void Simulate(PlayerController &script)
{
	const ID roomId = GetCurrentRoomId();
	const GameInput &input = GetInput();

	const f32 deltaSeconds = SIMULATE_SECONDS;

	Entity &player = GetSelf();
	SpriteComponent &sprite = GetSpriteComponent(player.id);
	const ColliderComponent &collider = GetColliderComponent(player.id);

	const Room &room = GetRoom(roomId);
	const float4 roomBounds = RoomOuterBounds(room);
	const f32 screenLeft = roomBounds.x;
	const f32 screenRight = roomBounds.z;
	const f32 screenBottom = roomBounds.y;
	const f32 screenTop = roomBounds.w;

	// Player entity
	{
		const float2 size = collider.size;
		const f32 accel = script.accel;
		float2 pos = player.position.xy + collider.offset;
		float2 &speed = script.speed;

		f32 direction = input.move.x;

		// Physics constants ///////////////////////////////////////////

		constexpr f32 runSpeed = 7.0f;
		constexpr f32 jumpSpeed = 12.0f;
		constexpr f32 gravityRise = -30.0f; // Lighter gravity while ascending so holding the button controls jump height
		constexpr f32 gravityFall = -50.0f; // ~1.8x rise: stronger gravity while falling for a snappier landing
		constexpr f32 terminalSpeed = -25.0f; // Keeps a long fall under one tile per step so collision can't tunnel
		constexpr f32 jumpCutMultiplier = 0.35f; // Kills upward speed quickly if the button is released early
		//constexpr f32 ADVANCE_EPSILON = 0.5f / ART_PIXELS_PER_METER;
		constexpr f32 SPEED_EPSILON = 0.2f;
		constexpr f32 climbSpeed = 4.0f;

		// Ladder //////////////////////////////////////////////////////

		const f32 ladderX = pos.x + 0.5f * size.x;
		const bool ladderAtBody = GetColliderAtWorldPos(float2{ladderX, pos.y + 0.5f * size.y}) == COLLIDER_LADDER;
		const bool ladderBelow = GetColliderAtWorldPos(float2{ladderX, pos.y - 0.1f}) == COLLIDER_LADDER;

		if (script.playerState != OnLadder)
		{
			const bool onGround = script.playerState == OnFloor || script.playerState == OnPlatform;
			const bool grabUp = input.action.press && input.move.y > 0.5f && ladderAtBody && speed.y <= 0.0f;
			const bool grabDown = input.action.press && input.move.y < -0.5f && ladderBelow && onGround;
			if (grabUp || grabDown)
			{
				// A box one cell wide touches both neighbouring columns when aligned to the grid,
				// so it sits a hair towards the free side or leaving the ladder would hit the wall.
				const f32 snapX = Floor(ladderX) + 0.5f - 0.5f * size.x;
				const bool leftBlocked = IsColliderInBox(float2{snapX - 0.01f, pos.y}, size, COLLIDER_SOLID);
				pos.x = leftBlocked ? snapX + 0.01f : snapX - 0.01f;
				speed = {};
				script.playerState = OnLadder;
			}
		}

		if (script.playerState == OnLadder)
		{
			if (input.jump.press)
			{
				speed.y = jumpSpeed;
				script.playerState = OnAir;
				PlayAudioClip(script.sndJump);
			}
			else
			{
				const float2 prevPos = pos;
				pos += climbSpeed * deltaSeconds * input.move;

				const bool leftColumn = false;// Floor(pos.x + 0.5f * size.x) != Floor(ladderX);
				if (leftColumn || IsColliderInBox(float2{pos.x, prevPos.y}, size, COLLIDER_SOLID)) {
					pos.x = prevPos.x;
				}

				const u32 atFeet = GetColliderAtWorldPos(float2{ladderX, pos.y});
				if (atFeet == COLLIDER_SOLID) {
					pos.y = Ceil(pos.y);
					script.playerState = OnFloor;
				} else if (GetColliderAtWorldPos(float2{ladderX, pos.y + size.y}) == COLLIDER_SOLID) {
					pos.y = prevPos.y;
				} else if (atFeet != COLLIDER_LADDER) {
					if (GetColliderAtWorldPos(float2{ladderX, pos.y - 0.1f}) == COLLIDER_LADDER) {
						pos.y = Floor(pos.y);
						script.playerState = OnPlatform;
					} else if (GetColliderAtWorldPos(float2{ladderX, pos.y + 0.5f * size.y}) != COLLIDER_LADDER) {
						script.playerState = OnAir;
					}
				}

				player.position.xy = pos - collider.offset;
				sprite.spriteId = script.sprPlayerIdle;
				return;
			}
		}

		// Speed epsilon ///////////////////////////////////////////////

		// X ///////////////////////////////////////////////////////////

		if ((direction < 0.0f && speed.x > 0.0f) ||
				(direction > 0.0f && speed.x < 0.0f) ||
				!direction )
		{
			// Retune of the old per-frame 0.8 factor, kept identical at 60Hz but framerate independent
			constexpr f32 frictionAt60Hz = 0.8f;
			speed.x *= Pow(frictionAt60Hz, deltaSeconds * 60.0f);
		}

		speed.x = speed.x + direction * accel * deltaSeconds;

		speed.x = Clamp(speed.x, -runSpeed, runSpeed);

		// Only without input: the first accelerating step advances less than the epsilon,
		// so the player could never start moving.
		if ( !direction && Abs(speed.x) < SPEED_EPSILON ) { speed.x = 0.0f; }

		const f32 prevX = pos.x;
		pos.x += speed.x * deltaSeconds;

		if (IsColliderInBox(pos, size, COLLIDER_SOLID)) {
			pos.x = prevX;
			speed.x = 0.0f;
		}

		// Y ///////////////////////////////////////////////////////////

		static f32 secondsSinceJump = 99.9f;
		static f32 secondsSinceJumpPress = 99.9f;
		if (input.jump.press) {
			secondsSinceJumpPress = 0.0f;
		} else {
			secondsSinceJumpPress += deltaSeconds;
		}

		// Grounded state comes from last frame's collision resolution, before this frame moves the player
		if (script.playerState == OnFloor || script.playerState == OnPlatform) {
			if (secondsSinceJump > 0.2f && secondsSinceJumpPress < 0.1f) {
				secondsSinceJump = 0.0f;
				if (input.move.y < 0.0 && Abs(input.move.y) > Abs(2 * input.move.x) && script.playerState == OnPlatform) {
					pos.y -= 0.1;
				} else {
					speed.y = jumpSpeed;
				}
				script.playerState = OnAir;
				PlayAudioClip(script.sndJump);
			}
		}

		secondsSinceJump += deltaSeconds;

		if (speed.y > 0 && !input.jump.pressed) {
			speed.y *= jumpCutMultiplier;
		}

		const f32 gravity2 = speed.y > 0 ? gravityRise : gravityFall;
		const f32 prevY = pos.y;
		pos.y += speed.y * deltaSeconds + 0.5 * gravity2 * deltaSeconds * deltaSeconds;
		speed.y = Max(speed.y + gravity2 * deltaSeconds, terminalSpeed);

		// Only landing on a surface grounds the player, hitting a ceiling does not
		script.playerState = OnAir;

		if (IsColliderInBox(pos, size, COLLIDER_SOLID)) {
			if (prevY < pos.y) {
				pos.y = prevY;
			} else {
				pos.y = Floor(prevY);
				script.playerState = OnFloor;
			}
			speed.y = 0.0f;
		}

		if (speed.y < 0.0)
		{
			const f32 centerX = pos.x + 0.5f * size.x;
			const float2 prevVertical = {centerX, prevY};
			const u32 colliderBelow = GetColliderAtWorldPos(float2{centerX, pos.y});
			if (GetColliderAtWorldPos(prevVertical) == 0 &&
				(colliderBelow == COLLIDER_PLATFORM || colliderBelow == COLLIDER_LADDER)) {
				if (prevY > pos.y) {
					pos.y = Floor(prevY);
					speed.y = 0.0f;
					script.playerState = OnPlatform;
				}
			}
		}

		if (pos.y < screenBottom) {
			pos.y = screenBottom;
			speed.y = 0;
			script.playerState = OnFloor;
		}

		// Player bounds
		pos.x = Clamp(pos.x, screenLeft, screenRight - size.x);
		pos.y = Clamp(pos.y, screenBottom, screenTop - size.y);

		player.position.xy = pos - collider.offset;

		// Animation
		if ( script.playerState == OnFloor || script.playerState == OnPlatform )
		{
			if ( Abs(speed.x) < SPEED_EPSILON ) {
				sprite.spriteId = script.sprPlayerIdle;
			} else {
				sprite.spriteId = script.sprPlayerRun;
			}
		}
		else
		{
			if ( speed.y >= 0.0f ) {
				sprite.spriteId = script.sprPlayerJump;
			} else {
				sprite.spriteId = script.sprPlayerFall;
			}
		}

		if ( speed.x > 0 ) {
			sprite.flipX = false;
		} else if ( speed.x < 0 ) {
			sprite.flipX = true;
		}
	}

}


REFLEX()
void Start(CameraController &script)
{
	sCameraController = &script;
	const f32 fovyDeg = GAME_CAMERA_FOVY;
	const f32 zcam = GameCameraDistance();

	script.camera = {
		.projectionType = ProjectionPerspective,
		.position = {0, 0, zcam},
		.orientation = { 0, 0 },
		.znear = 1.0f,
		.zfar = 1000.0f,
		.fovy = fovyDeg,
	};

	SetCamera(script.camera);

	script.cutToTarget = true;
}

REFLEX()
void Follow(CameraController &script)
{
	sCameraController = &script;

	const ID roomId = GetCurrentRoomId();

	Entity &player = GetEntity(script.playerId);
	const float2 playerPos = player.position.xy;

	const Room &room = GetRoom(roomId);

	const f32 visHalfWidth = 0.5f * LOW_SCENE_WIDTH / ART_PIXELS_PER_METER;
	const f32 visHalfHeight = 0.5f * LOW_SCENE_HEIGHT / ART_PIXELS_PER_METER;
	const float2 visCenter = script.cutToTarget ? playerPos : script.camera.position.xy;
	const float2 visRegionMin = float2{visCenter.x - visHalfWidth, visCenter.y - visHalfHeight};
	const float2 visRegionMax = float2{visCenter.x + visHalfWidth, visCenter.y + visHalfHeight};

	constexpr f32 lookAheadSeconds = 0.0;
	constexpr f32 verticalWindow = 1.5f;
	constexpr f32 followAt60Hz = 0.1f;
	constexpr f32 boundsAt60Hz = 0.05f;
	constexpr f32 snapEpsilon = 1.0f / ART_PIXELS_PER_METER;

	const float4 roomBounds = RoomInnerBounds(room);
	const float2 roomMin = roomBounds.xy;
	const float2 roomMax = roomBounds.zw;

	float2 targetMin = roomMin;
	float2 targetMax = roomMax;

	const u32 regionCount = GetCameraRegionCount();
	for (u32 i = 0; i < regionCount; ++i)
	{
		const CameraRegionComponent &region = GetCameraRegion(i);
		const float2 regionMin = GetEntity(region.entityId).position.xy + region.offset;
		const float2 regionMax = regionMin + region.size;

		if (playerPos.x >= regionMin.x && playerPos.x < regionMax.x &&
			playerPos.y >= regionMin.y && playerPos.y < regionMax.y) {
		}

		const bool outsideX = playerPos.x < regionMin.x || playerPos.x >= regionMax.x;
		const bool outsideY = playerPos.y < regionMin.y || playerPos.y >= regionMax.y;
		const bool rowsVisible = visRegionMax.y > regionMin.y && visRegionMin.y < regionMax.y;
		const bool columnsVisible = visRegionMax.x > regionMin.x && visRegionMin.x < regionMax.x;

		if (outsideX && (!outsideY || rowsVisible))
		{
			if (playerPos.x < regionMin.x) {
				targetMax.x = Min(targetMax.x, regionMin.x);
			}
			if (playerPos.x >= regionMax.x) {
				targetMin.x = Max(targetMin.x, regionMax.x);
			}
		}
		else if (!outsideX || columnsVisible)
		{
			if (playerPos.y < regionMin.y) {
				targetMax.y = Min(targetMax.y, regionMin.y);
			}
			if (playerPos.y >= regionMax.y) {
				targetMin.y = Max(targetMin.y, regionMax.y);
			}
		}
	}

	targetMin = Max(targetMin, roomMin);
	targetMax = Min(targetMax, roomMax);

	if (script.cutToTarget) {
		script.boundsMin = targetMin;
		script.boundsMax = targetMax;
	} else {
		const f32 boundsT = 1.0f - Pow(1.0f - boundsAt60Hz, SIMULATE_SECONDS * 60.0f);
		script.boundsMin = Lerp(script.boundsMin, targetMin, boundsT);
		script.boundsMax = Lerp(script.boundsMax, targetMax, boundsT);
		if (Length(targetMin - script.boundsMin) < snapEpsilon) {
			script.boundsMin = targetMin;
		}
		if (Length(targetMax - script.boundsMax) < snapEpsilon) {
			script.boundsMax = targetMax;
		}
	}

	const float2 halfSceneSize = 0.5f * float2{LOW_SCENE_WIDTH, LOW_SCENE_HEIGHT} / ART_PIXELS_PER_METER;
	f32 cameraLeft = script.boundsMin.x + halfSceneSize.x;
	f32 cameraRight = script.boundsMax.x - halfSceneSize.x;
	f32 cameraBottom = script.boundsMin.y + halfSceneSize.y;
	f32 cameraTop = script.boundsMax.y - halfSceneSize.y;

	if (cameraLeft > cameraRight) {
		cameraLeft = cameraRight = 0.5f * (script.boundsMin.x + script.boundsMax.x);
	}
	if (cameraBottom > cameraTop) {
		cameraBottom = cameraTop = 0.5f * (script.boundsMin.y + script.boundsMax.y);
	}

	//if (script.playerState == OnFloor || script.playerState == OnPlatform) {
	//	script.cameraTargetY = playerPos.y;
	//}
	script.cameraTargetY = Clamp(script.cameraTargetY, playerPos.y - verticalWindow, playerPos.y + verticalWindow);

	// Camera easing / interpolation
	const float2 target = { playerPos.x, script.cameraTargetY };
	const f32 t = 1.0f - Pow(1.0f - followAt60Hz, SIMULATE_SECONDS * 60.0f);
	const float2 prevCameraPos = script.camera.position.xy;
	float2 cameraPos = Lerp(prevCameraPos, target, t);
	if (Length(target - cameraPos) < snapEpsilon) {
		cameraPos = target;
	}

	// Cancel easing
	if ( script.cutToTarget ) {
		cameraPos = target;
	}

	script.camera.position.x = Clamp(cameraPos.x, cameraLeft, cameraRight);
	script.camera.position.y = Clamp(cameraPos.y, cameraBottom, cameraTop);

	// The renderer rounds the camera and the player to pixels independently, so a
	// fractional distance between them shows as a 1px wobble of whichever is being
	// watched. Per axis: while the camera travels with the player, the submitted camera
	// sits a whole number of pixels from it, so the player holds its screen pixel;
	// while the player moves across a slower camera it is left alone, or the whole
	// scene would shake instead. script.camera stays unsnapped so the lerp never stalls.
	Camera camera = script.camera;
	const f32 pixelSize = 1.0f / GetScenePixelsPerMeter();
	const float2 cameraStep = script.camera.position.xy - prevCameraPos;
	const float2 offsetStep = cameraStep - (playerPos - player.prevPosition.xy);
	const float2 offset = script.camera.position.xy - playerPos;
	if (Abs(offsetStep.x) < Abs(cameraStep.x)) {
		camera.position.x = playerPos.x + Round(offset.x / pixelSize) * pixelSize;
	}
	if (Abs(offsetStep.y) < Abs(cameraStep.y)) {
		camera.position.y = playerPos.y + Round(offset.y / pixelSize) * pixelSize;
	}

	SetCamera(camera);

	script.cutToTarget = false;
}
