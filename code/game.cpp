
void GameStart(Game &game)
{
	LOG(Info, "- GameStart!\n");

	game.box1 = {
		.pos = { 0, 0 },
		.size = { 1, 1 },
		.color = { 0.0, 0.5, 1.0, 1.0 },
	};
	game.box2 = {
		.pos = { 0, 0 },
		.size = { 1, 1 },
		.color = { 1.0, 0.5, 0.0, 1.0 },
	};

	game.speed = {2.0f, 10.0f};
	game.speed2 = {};
	game.accel2 = 50;

	game.ent = GetEntity("player");
	game.ent->position.xy = float2{1, 1};
	game.sndJump = GetAudioClip("snd_bell_wav");
	game.modEquinox = GetMusic("mod_equinox_mod");
	game.playingMusic = false;

	game.camera = {
		.projectionType = ProjectionOrthographic,
		.position = {0, 0, -1},
		.znear = -10.0f,
		.zfar = 10.0f,
		//.height = 180.0f / PIXELS_PER_METER,
		.height = 90.0f / PIXELS_PER_METER,
	};

	game.room = GetRoom("Room");
}

// Translate platform input to game input controls
void GameSetInput(Game &game, const Keyboard &keyboard, const Mouse &mouse, const Gamepad &gamepad)
{
	game.input = {};

	// Keyboard

	game.input.move.x += KeyPressed(keyboard, K_D) ? 1.0f : 0.0f;
	game.input.move.x -= KeyPressed(keyboard, K_A) ? 1.0f : 0.0f;
	game.input.jump.press = KeyPress(keyboard, K_SPACE);
	game.input.jump.pressed = KeyPressed(keyboard, K_SPACE);

	// Gamepad

	game.input.move += gamepad.leftAxis;
	game.input.jump.press |= ButtonPress(gamepad.a);
	game.input.jump.pressed |= ButtonPressed(gamepad.a);
}

void GameSimulate(Game &game)
{
	LOG(Debug, "- GameUpdate!\n");

	if (!game.playingMusic)
	{
		PlayMusic(game.modEquinox);
	}


	const f32 deltaSeconds = game.deltaSeconds;
	constexpr f32 gravity = -15.8f;

	const Room &room = *game.room;

	{
		float2 &pos = game.box1.pos;

		pos.x += game.speed.x * deltaSeconds;
		if (pos.x > 10) pos.x = -10;

		pos.y = pos.y + game.speed.y * deltaSeconds + 0.5 * gravity * deltaSeconds * deltaSeconds;
		game.speed.y = game.speed.y + gravity * deltaSeconds;

		if (pos.y < 0.0f ) {
			pos.y = 0.0f;
			game.speed.y = 10.0f;
		}
	}

	{
		float2 playerPos = game.ent->position.xy;
		const float2 playerSize = { game.ent->scale, game.ent->scale };

		f32 direction = game.input.move.x;

		// X ///////////////////////////////////////////////////////////

		if (direction < 0.0f && game.speed2.x > 0.0f ||
				direction > 0.0f && game.speed2.x < 0.0f ||
				!direction )
		{
			game.speed2.x *= 0.8;
		}

		if (direction != 0) {
			game.speed2.x = game.speed2.x + direction * game.accel2 * deltaSeconds;
		}

		game.speed2.x = Clamp(game.speed2.x, -10.0f, 10.0f);

		const f32 prevX = playerPos.x;
		playerPos.x += game.speed2.x * deltaSeconds;
		if (IsColliderInBox(playerPos, playerSize, 1)) {
			playerPos.x = prevX;
			game.speed2.x = 0.0f;
		}

		// Y ///////////////////////////////////////////////////////////

		constexpr f32 gravityRise = -30.0f; // Lighter gravity while ascending so holding the button controls jump height
		constexpr f32 gravityFall = -50.0f; // Stronger gravity while falling for a snappier landing
		constexpr f32 jumpSpeed = 14.0f;
		constexpr f32 jumpCutMultiplier = 0.35f; // Kills upward speed quickly if the button is released early

		if (game.input.jump.press) {
			if (game.speed2.y == 0) {
				game.speed2.y = jumpSpeed;
				PlayAudioClip(game.sndJump);
			}
		}

		if (game.speed2.y > 0 && !game.input.jump.pressed) {
			game.speed2.y *= jumpCutMultiplier;
		}

		const f32 gravity2 = game.speed2.y > 0 ? gravityRise : gravityFall;
		const f32 prevY = playerPos.y;
		playerPos.y += game.speed2.y * deltaSeconds + 0.5 * gravity2 * deltaSeconds * deltaSeconds;
		game.speed2.y = game.speed2.y + gravity2 * deltaSeconds;
		if (IsColliderInBox(playerPos, playerSize, 1)) {
			if (prevY < playerPos.y)
				playerPos.y = Ceil(prevY);
			else
				playerPos.y = Floor(prevY);
			game.speed2.y = 0.0f;
		}

		if (game.speed2.y < 0.0)
		{
			const float2 prevVertical = {playerPos.x, prevY};
			if (GetColliderAtWorldPos(prevVertical) == 0 &&
				GetColliderAtWorldPos(playerPos) == 2) {
				if (prevY > playerPos.y) {
					playerPos.y = Floor(prevY);
					game.speed2.y = 0.0f;
				}
			}
		}

		if (playerPos.y < 0) {
			playerPos.y = 0;
			game.speed2.y = 0;
		}

		if (playerPos.y < 0.0f ) {
			playerPos.y = 0.0f;
		}

		// Player bounds
		const f32 screenLeft = room.pos.x;
		const f32 screenRight = room.pos.x + RoomSize(room).x;
		const f32 screenBottom = room.pos.y;
		const f32 screenTop = room.pos.y + RoomSize(room).y;
		playerPos.x = Clamp(playerPos.x, screenLeft, screenRight - playerSize.x);
		playerPos.y = Clamp(playerPos.y, screenBottom, screenTop - playerSize.y);

		// Camera bounds
		const float2 halfSceneSize = 0.5f * float2{SCENE_WIDTH, SCENE_HEIGHT} / PIXELS_PER_METER;
		const f32 cameraLeft = screenLeft + halfSceneSize.x;
		const f32 cameraRight = screenRight - halfSceneSize.x;
		const f32 cameraBottom = screenBottom + halfSceneSize.y;
		const f32 cameraTop = screenTop - halfSceneSize.y;
		const f32 cameraX = Lerp(game.camera.position.x, playerPos.x, 0.2f);
		const f32 cameraY = Lerp(game.camera.position.y, playerPos.y, 0.2f);
		game.camera.position.x = Clamp(cameraX, cameraLeft, cameraRight);
		game.camera.position.y = Clamp(cameraY, cameraBottom, cameraTop);

		EntitySetPosition(*game.ent, Float3(playerPos, game.ent->position.z));
	}
}

void GameUpdate(Game &game)
{
	SetCamera(game.camera);

	//DrawBoxOutline(Float2(game.room->pos), LayerSize(game.room->layers[0]), ColorOrange);
	DrawBox(game.box1.pos, game.box1.size, game.box1.color);
}

void GameStop(Game &game)
{
	LOG(Info, "- GameStop!\n");
}

