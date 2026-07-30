#ifndef GAME_H
#define GAME_H

////////////////////////////////////////////////////////////////////////
// Engine Types
////////////////////////////////////////////////////////////////////////

struct Entity;

////////////////////////////////////////////////////////////////////////
// Game Types
////////////////////////////////////////////////////////////////////////

enum GameState
{
	GameStateStopped,
	GameStateStarting,
	GameStateRunning,
	GameStateStopping,
	GameStateCount,
};

union InputButton
{
	u8 press : 1;
	u8 pressed : 1;
	u8 release : 1;
};

struct GameInput
{
	float2 move;
	InputButton jump;
};

struct Box
{
	float2 pos;
	float2 size;
	float4 color;
};

struct Game
{
	GameState state;

	f32 deltaSeconds;
	f32 accumulatedSeconds;

	GameInput input;

	Box box1;
	float2 speed;

	Box box2;
	float2 speed2;
	float accel2;
	bool grounded;

	Entity *ent;

	Handle sndJump;
	Handle modEquinox;

	bool playingMusic;

	Camera camera;

	Room *room;
};

////////////////////////////////////////////////////////////////////////
// Engine -> Game interface
////////////////////////////////////////////////////////////////////////

void GameSetInput(Game &game, const Keyboard &, const Mouse &, const Gamepad &);
void GameStart(Game &game);
void GameSimulate(Game &game);
void GameUpdate(Game &game);
void GameStop(Game &game);

////////////////////////////////////////////////////////////////////////
// Game -> Engine interface
////////////////////////////////////////////////////////////////////////

Entity *GetEntity(const char *name);
void EntitySetPosition(Entity &entity, float3 position);
void DrawBox(float2 pos, float2 size, float4 color);
bool IsColliderAtWorldPos(float2 worldPos);
bool IsColliderInBox(float2 pos, float2 size);

#endif // GAME_H
