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


////////////////////////////////////////////////////////////////////////
// Asset descriptors and scene serialization

void LoadSceneFromBin(Engine &engine);

#if USE_DATA_BUILD
void LoadSceneFromTxt(Engine &engine, const char *filepath);
void SaveSceneToTxt(Engine &engine, const char *filepath);
void BuildShaders(Engine &engine, const char *outBinFilepath);
void BuildAssetsFromTxt(Engine &engine, const char *inTxtFilepath, const char *outBinFilepath);
#endif // USE_DATA_BUILD

#endif // ENGINE_H
