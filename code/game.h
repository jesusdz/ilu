#ifndef GAME_H
#define GAME_H

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

struct InputButton
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

constexpr f32 SIMULATE_SECONDS = (1.0f / 60.0f);

struct Game
{
	GameState state;
	GameInput input;

	f32 deltaSeconds;
	f32 accumulatedSeconds;

	ID currentEntity;
};

////////////////////////////////////////////////////////////////////////
// Game -> Engine interface
////////////////////////////////////////////////////////////////////////

Entity &GetSelf();
ID FindEntity(const char *name);
Entity *TryGetEntity(ID entityId); // Null once the entity is gone
ID FindRoom(const char *name);
Room *TryGetRoom(ID roomId);       // Null once the room is gone

#endif // GAME_H
