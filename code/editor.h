#ifndef EDITOR_H
#define EDITOR_H

enum EditorCommandType
{
	EditorCommandReloadGraphicsPipeline,
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
		u32 pipelineIndex;
		ID textureId;
		const char *filepath;
	};
};

enum EditorSelectedType
{
	EditorSelectedType_None,
	EditorSelectedType_Scene,
	EditorSelectedType_Room,
	EditorSelectedType_Layer,
	EditorSelectedType_Entity,
	EditorSelectedType_Material,
	EditorSelectedType_Texture,
	EditorSelectedType_Audio,
	EditorSelectedType_Music,
	EditorSelectedType_Sprite,
	EditorSelectedType_FileImage,
	EditorSelectedType_FileAudio,
	EditorSelectedType_FileMusic,
	EditorSelectedType_FileUnknown,
	EditorSelectedType_Count,
	EditorSelectedType_FileBegin = EditorSelectedType_FileImage,
	EditorSelectedType_FileEnd = EditorSelectedType_FileUnknown,
};

static const char *EditorSelectedTypeName[] = {
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
	// Discriminated by `type`. Reading through the wrong member is the one thing the
	// handle types cannot catch, so go through the matching EditorSelect* helper.
	union
	{
		Handle handle; // untyped view, only for clearing and comparing
		ID roomId;
		ID entityId;
		ID materialId;
		ID textureId;
		AudioClipH audioClipH;
		MusicH musicH;
		ID spriteId;
		FileNode *file;
		u64 value;
	};
};

struct EditorInspector
{
	EditorSelection selected;
	EditorSelection nextSelected;
	// Preview asset built from the inspected file, discriminated by selected.type
	// the same way EditorSelection is
	union
	{
		Handle tmpHandle; // untyped view, only for clearing
		ID tmpTextureH;
		AudioClipH tmpAudioClipH;
		MusicH tmpMusicH;
	};
};

struct EditorSpriteSheet
{
	ID textureId;
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
		entityId = editor.inspector.selected.entityId;
	}
	return entityId;
}

#endif // #ifndef EDITOR_H
