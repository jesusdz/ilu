#ifndef EDITOR_H
#define EDITOR_H

enum EditorIcon
{
	EditorIcon_Play,
	EditorIcon_Open,
	EditorIcon_Save,
	EditorIcon_Reload,
	EditorIcon_Eye,
	EditorIcon_Up,
	EditorIcon_Down,
	EditorIcon_X,
	EditorIcon_Count,
};

constexpr const char *sEditorIconFilenames[] = {
	"editor/play_16.png",
	"editor/open.png",
	"editor/save.png",
	"editor/reload.png",
	"editor/eye16.png",
	"editor/up_16.png",
	"editor/down_16.png",
	"editor/x_16.png",
};

CT_ASSERT(ARRAY_COUNT(sEditorIconFilenames) == EditorIcon_Count);

enum EditorCommandType
{
	EditorCommandRemoveTexture,
	EditorCommandNew,
	EditorCommandLoadTxt,
	EditorCommandSaveTxt,
	EditorCommandLoadBin,
	EditorCommandBuildBin,
	EditorCommandCount,
};

struct EditorCommand
{
	EditorCommandType type;
	union
	{
		ID textureId;
		const char *filepath;
	};
};

enum EditorSelectedType
{
	EditorSelectedType_None,
	// Assets
	EditorSelectedType_Scene,
	EditorSelectedType_Room,
	EditorSelectedType_Layer,
	EditorSelectedType_Entity,
	EditorSelectedType_Material,
	EditorSelectedType_Texture,
	EditorSelectedType_Audio,
	EditorSelectedType_Music,
	EditorSelectedType_Sprite,
	EditorSelectedType_Prefab,
	EditorSelectedType_ParticleEffect,
	// File
	EditorSelectedType_FileImage,
	EditorSelectedType_FileAudio,
	EditorSelectedType_FileMusic,
	EditorSelectedType_FileUnknown,
	// Count
	EditorSelectedType_Count,
	// The types that name an asset, all of them an ID living in the same union
	EditorSelectedType_AssetBegin = EditorSelectedType_Entity,
	EditorSelectedType_AssetEnd = EditorSelectedType_ParticleEffect,
	EditorSelectedType_FileBegin = EditorSelectedType_FileImage,
	EditorSelectedType_FileEnd = EditorSelectedType_FileUnknown,
};

constexpr const char *EditorSelectedTypeName[] = {
	"None",
	"Scene",
	"Scene", // Room
	"Scene", // Layer
	"Entity",
	"Material",
	"Texture",
	"Audio",
	"Music",
	"Sprite",
	"Prefab",
	"Particle effect",
	"Image file",
	"Audio file",
	"Music file",
	"Unknown file type",
};
CT_ASSERT(ARRAY_COUNT(EditorSelectedTypeName) == EditorSelectedType_Count);

enum EditorTool
{
	EditorTool_Draw,
	EditorTool_ColliderSolid,
	EditorTool_ColliderPlatform,
	EditorTool_Erase,
};

enum FileNodeType
{
	FileNodeType_Image,
	FileNodeType_Music,
	FileNodeType_Sound,
	FileNodeType_COUNT,
};

struct FileNode
{
	FileNodeType type;
	const char *filename;

	FileNode *next; // Next file in same directory
	FileNode *prev; // Prev file in same directory
	FileNode *child; // For directy contents
};

struct SnapshotNode
{
	const char *filepath;
	ImageH imageH;
	SnapshotNode *next;
};

struct EditorSelection
{
	EditorSelectedType type;
	// Discriminated by `type`. Every asset is an ID now, so nothing but `type` says
	// which kind this one is: go through the matching EditorSelect* helper.
	union
	{
		ID id;
		FileNode *file;
		u64 value;
	};
};

struct EditorInspector
{
	EditorSelection selected;
	EditorSelection nextSelected;
	u32 audioSourceIndex; // Source the inspector plays through
	// Preview asset built from the inspected file, discriminated by selected.type
	// the same way EditorSelection is
	union
	{
		ID tmpTextureId;
		ID tmpAudioClipId;
		ID tmpMusicId;
	};
};

struct EditorSpriteSheet
{
	ID textureId;
	ID spriteId;
};

constexpr u32 EditorNoLayer = U32_MAX;

struct EditorContext
{
	FileNode *selectedFile;
	ID roomId;
	u32 layerIndex;
	ID spriteId; // brush
	EditorTool tool;
};

struct Editor
{
	bool showLoadScene;
	bool showSaveScene;
	bool showDebugUI;
	bool showOutliner;
	bool showAssets;
	bool showInspector;
	bool showSpriteSheet;
	bool showProfiler;
	bool showGrid;
	bool showAbout;
	bool showContextMenu;
	bool showSettings;
	bool showQuit;

	// World position the scene view was right-clicked at, captured when the menu opens
	// because lastMouseClickPos moves to the menu item as soon as it is clicked
	float2 contextMenuWorldPos;

	bool selectEntity;

	bool isTranslating;

	Camera camera[ProjectionTypeCount];
	ProjectionType cameraType;
	bool cameraOrbit;

	EditorCommand commands[128];
	u32 commandCount;

	ImageH iconAsset;
	ImageH iconWav;
	ImageH iconMod;
	ImageH iconImg;
	ImageH iluLogo;

	SnapshotNode *snapshots;

	EditorContext context;
	EditorInspector inspector;
	EditorSpriteSheet spriteSheet;

	FileNode *root;
	FileNode *freeNodes;
};

struct Engine;

void EditorInitialize(Engine &engine);
void EditorUpdate(Engine &engine);
void EditorRender(Engine &engine, CommandList &commandList);
void EditorPostRender(Engine &engine);

inline ID EditorGetSelectedEntity(const Editor &editor)
{
	ID entityId = {};
	if ( editor.inspector.selected.type == EditorSelectedType_Entity ) {
		entityId = editor.inspector.selected.id;
	}
	return entityId;
}

#endif // #ifndef EDITOR_H
