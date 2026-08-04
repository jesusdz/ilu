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

enum PlayerState
{
	OnFloor,
	OnPlatform,
	OnAir,
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
	PlayerState playerState;

	EntityH entH;

	AudioClipH sndJump;
	MusicH modEquinox;

	bool playingMusic;

	Camera camera;

	RoomH roomH;
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

EntityH FindEntity(const char *name);
Entity *GetEntity(EntityH handle);
RoomH FindRoom(const char *name);
Room *GetRoom(RoomH handle);
void EntitySetPosition(Entity &entity, float3 position);
void DrawBox(float2 pos, float2 size, float4 color);
bool IsColliderAtWorldPos(float2 worldPos);
bool IsColliderInBox(float2 pos, float2 size);

#endif // GAME_H
