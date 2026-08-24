
SCRIPT_THUNK(ScriptPlayerController)


void RegisterScripts(Game &game)
{
	game.scriptCount = 0;
	ZeroArray(game.scripts);
	game.propertyCount = 0;
	ZeroArray(game.properties);

	{
		SCRIPT_BEGIN(ScriptPlayerController);
		PROPERTY(ID, sprPlayerIdle);
		PROPERTY(ID, sprPlayerRun);
		PROPERTY(ID, sprPlayerJump);
		PROPERTY(ID, sprPlayerFall);
		SCRIPT_END();
	}
}

void Start(ScriptPlayerController &script)
{
	script.playerState = OnAir;

	Entity &player = GetSelf();
	player.position.xy = float2{1, 1};
	player.colliderSize = float2{player.scale, player.scale};
	player.speed = {};
	player.accel = 50;

	//script.sprPlayerIdle = FindSprite("spr_playeridle");
	//script.sprPlayerRun = FindSprite("spr_playerrun");
	//script.sprPlayerJump = FindSprite("spr_playerjump");
	//script.sprPlayerFall = FindSprite("spr_playerfall");
	script.sndJump = GetAudioClip("snd_bell_wav");
	script.modEquinox = GetMusic("mod_equinox_mod");
	script.playingMusic = false;

	script.camera = {
		.projectionType = ProjectionOrthographic,
		.position = {0, 0, -1},
		.znear = -10.0f,
		.zfar = 10.0f,
		//.height = 180.0f / PIXELS_PER_METER,
		.height = 90.0f / PIXELS_PER_METER,
	};

	script.roomId = FindRoom("Room");
}

void Simulate(ScriptPlayerController &script)
{
	const Game &game = GetGame();

	if (!script.playingMusic)
	{
		PlayMusic(script.modEquinox);
		script.playingMusic = true;
	}


	const f32 deltaSeconds = game.deltaSeconds;
	constexpr f32 gravity = -15.8f;

	const Room *roomPtr = TryGetRoom(script.roomId);
	if ( !roomPtr ) {
		return;
	}
	const Room &room = *roomPtr;
	Entity &player = GetSelf();

	const f32 screenLeft = room.pos.x;
	const f32 screenRight = room.pos.x + RoomSize(room).x;
	const f32 screenBottom = room.pos.y;
	const f32 screenTop = room.pos.y + RoomSize(room).y;

	// Player entity
	{
		const float2 size = player.colliderSize;
		const f32 accel = player.accel;
		float2 &pos = player.position.xy;
		float2 &speed = player.speed;

		f32 direction = game.input.move.x;

		// Speed epsilon ///////////////////////////////////////////////

		constexpr f32 SPEED_EPSILON = 0.01;
		if ( Abs(speed.x) < SPEED_EPSILON ) { speed.x = 0.0f; }

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

		speed.x = Clamp(speed.x, -10.0f, 10.0f);

		const f32 prevX = pos.x;
		pos.x += speed.x * deltaSeconds;

		if (IsColliderInBox(pos, size, 1)) {
			pos.x = prevX;
			speed.x = 0.0f;
		}

		// Y ///////////////////////////////////////////////////////////

		constexpr f32 gravityRise = -30.0f; // Lighter gravity while ascending so holding the button controls jump height
		constexpr f32 gravityFall = -50.0f; // Stronger gravity while falling for a snappier landing
		constexpr f32 jumpSpeed = 14.0f;
		constexpr f32 jumpCutMultiplier = 0.35f; // Kills upward speed quickly if the button is released early

		// Grounded state comes from last frame's collision resolution, before this frame moves the player
		if (game.input.jump.press) {
			if (script.playerState == OnFloor || script.playerState == OnPlatform) {
				if (game.input.move.y < 0.0 && Abs(game.input.move.y) > Abs(2 * game.input.move.x) && script.playerState == OnPlatform) {
					pos.y -= 0.1;
				} else {
					speed.y = jumpSpeed;
				}
				script.playerState = OnAir;
				PlayAudioClip(script.sndJump);
			}
		}

		if (speed.y > 0 && !game.input.jump.pressed) {
			speed.y *= jumpCutMultiplier;
		}

		const f32 gravity2 = speed.y > 0 ? gravityRise : gravityFall;
		const f32 prevY = pos.y;
		pos.y += speed.y * deltaSeconds + 0.5 * gravity2 * deltaSeconds * deltaSeconds;
		speed.y = speed.y + gravity2 * deltaSeconds;

		// Only landing on a surface grounds the player, hitting a ceiling does not
		script.playerState = OnAir;

		if (IsColliderInBox(pos, size, 1)) {
			if (prevY < pos.y) {
				pos.y = Ceil(prevY);
			} else {
				pos.y = Floor(prevY);
				script.playerState = OnFloor;
			}
			speed.y = 0.0f;
		}

		if (speed.y < 0.0)
		{
			const float2 prevVertical = {pos.x, prevY};
			if (GetColliderAtWorldPos(prevVertical) == 0 &&
				GetColliderAtWorldPos(pos) == 2) {
				if (prevY > pos.y) {
					pos.y = Floor(prevY);
					speed.y = 0.0f;
					script.playerState = OnPlatform;
				}
			}
		}

		if (pos.y < 0) {
			pos.y = 0;
			speed.y = 0;
			script.playerState = OnFloor;
		}

		// Player bounds
		pos.x = Clamp(pos.x, screenLeft, screenRight - size.x);
		pos.y = Clamp(pos.y, screenBottom, screenTop - size.y);

		// Animation
		if ( script.playerState == OnFloor || script.playerState == OnPlatform )
		{
			if ( Abs(speed.x) < 0.2 ) {
				player.spriteId = script.sprPlayerIdle;
			} else {
				player.spriteId = script.sprPlayerRun;
			}
		}
		else
		{
			if ( speed.y >= 0.0f ) {
				player.spriteId = script.sprPlayerJump;
			} else {
				player.spriteId = script.sprPlayerFall;
			}
		}

		if ( speed.x > 0 ) {
			player.flipX = false;
		} else if ( speed.x < 0 ) {
			player.flipX = true;
		}
	}

	// Camera
	{
		const float2 playerPos = player.position.xy;

		const float2 halfSceneSize = 0.5f * float2{SCENE_WIDTH, SCENE_HEIGHT} / PIXELS_PER_METER;
		const f32 cameraLeft = screenLeft + halfSceneSize.x;
		const f32 cameraRight = screenRight - halfSceneSize.x;
		const f32 cameraBottom = screenBottom + halfSceneSize.y;
		const f32 cameraTop = screenTop - halfSceneSize.y;
		//const f32 cameraX = Lerp(script.camera.position.x, playerPos.x, 0.2f);
		//const f32 cameraY = Lerp(script.camera.position.y, playerPos.y, 0.2f);
		float2 cameraPos = script.camera.position.xy;
		cameraPos.x = cameraPos.x < playerPos.x - 1.0 ? playerPos.x - 1.0 : cameraPos.x;
		cameraPos.x = cameraPos.x > playerPos.x + 1.0 ? playerPos.x + 1.0 : cameraPos.x;
		cameraPos.y = cameraPos.y < playerPos.y - 1.0 ? playerPos.y - 1.0 : cameraPos.y;
		cameraPos.y = cameraPos.y > playerPos.y + 1.0 ? playerPos.y + 1.0 : cameraPos.y;
		if ( game.input.move.x == 0.0f ) { cameraPos.x = Lerp(cameraPos.x, playerPos.x, 0.2f); }
		if ( game.input.move.y == 0.0f ) { cameraPos.y = Lerp(cameraPos.y, playerPos.y, 0.2f); }
		script.camera.position.x = Clamp(cameraPos.x, cameraLeft, cameraRight);
		script.camera.position.y = Clamp(cameraPos.y, cameraBottom, cameraTop);

	}
}

void Update(ScriptPlayerController &script)
{
	SetCamera(script.camera);

	//const Room *roomPtr = TryGetRoom(script.roomId);
	//DrawBoxOutline(Float2(roomPtr->pos), LayerSize(roomPtr->layers[0]), ColorOrange);
	//DrawBox(script.box1.pos, script.box1.size, script.box1.color);
}

void Stop(ScriptPlayerController &script)
{
}

