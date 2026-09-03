#ifndef ENGINE_H
#define ENGINE_H

struct Settings
{
	bool hotReload;
};

struct Engine
{
	IDPool idPool;

	Graphics gfx;
	Audio audio;
	Scene scene;
	Game game;
	ScriptDataPool scriptData;
#if USE_UI
	UI ui;
#endif
#if USE_EDITOR
	Editor editor;
#endif
	Settings settings;

	BinAssets shaderAssets;
	BinAssets assets;

	Arena dataArenaStates[1];
	u32 dataArenaStateCount;
};

#endif // ENGINE_H
