
static const char * NameFromFilename(const char *name)
{
	FilePath path = {};
	StrCopy(path.str, name);
	char *ptr = path.str;
	while (*ptr) {
		if (*ptr == '.') {
			*ptr = '_';
		}
		ptr++;
	}
	const char *outName = InternString(path.str);
	return outName;
}

static const char *MakeName(const char *format, ...)
{
	FilePath path = {};
	va_list vaList;
	va_start(vaList, format);
	VSPrintf(path.str, format, vaList);
	va_end(vaList);
	const char *outName = InternString(path.str);
	return outName;
}

// Both null once the selection is gone, and only good for this frame
static Room *EditorGetContextRoom()
{
	Engine &engine = GetEngine();
	const ID roomId = engine.editor.context.roomId;
	Room *room = nullptr;
	if ( roomId ) {
		room = &GetRoom(roomId);
	}
	return room;
}

static Layer *EditorGetContextLayer()
{
	Engine &engine = GetEngine();
	const u32 layerIndex = engine.editor.context.layerIndex;
	Room *room = EditorGetContextRoom();
	Layer *layer = nullptr;
	if ( room && layerIndex < ARRAY_COUNT(room->layers) && room->layers[layerIndex].initialized ) {
		layer = &room->layers[layerIndex];
	}
	return layer;
}

static bool EditorMode3D()
{
	Editor &editor = GetEditor();
	return editor.cameraType == ProjectionPerspective;
}

static bool EditorMode2D()
{
	return !EditorMode3D();
}

static void EditorSetCamera()
{
	Editor &editor = GetEditor();
	SetCamera(editor.camera[editor.cameraType]);
}

static void EditorSetMode3D()
{
	Editor &editor = GetEditor();
	editor.cameraType = ProjectionPerspective;
	EditorSetCamera();
}

static void EditorSetMode2D()
{
	Editor &editor = GetEditor();
	editor.cameraType = ProjectionOrthographic;
	EditorSetCamera();
}

static void AddEditorCommand(const EditorCommand &command)
{
	Editor &editor = GetEditor();
	ASSERT(editor.commandCount < ARRAY_COUNT(editor.commands));
	editor.commands[editor.commandCount++] = command;
}

static ImageH EditorLoadIcon(const char *filename, const char *name)
{
	Engine &engine = GetEngine();
	const FilePath path = MakePath(ProjectDir, filename);
	ImagePixels imagePixels;
	Scratch scratch;
	ReadImagePixels(scratch.arena, path.str, imagePixels);
	const ImageH handle = GfxCreateImage(engine.gfx, imagePixels, name, false);
	return handle;
}

static ImageH EditorLoadSnapshot(const char *filepath, const char *name)
{
	Engine &engine = GetEngine();
	ImagePixels imagePixels;
	Scratch scratch;
	ReadImagePixels(scratch.arena, filepath, imagePixels);
	imagePixels = ResizeImagePixels(scratch.arena, imagePixels, 32, 32);
	const ImageH handle = GfxCreateImage(engine.gfx, imagePixels, name, false);
	return handle;
}

static SnapshotNode *EditorGetOrCreateSnapshotNode(const char *filepath)
{
	Engine &engine = GetEngine();
	// First we try to find an existing snapshot for this path
	SnapshotNode *snapshot = engine.editor.snapshots;
	while (snapshot)
	{
		if ( StrEq( snapshot->filepath, filepath ) )
		{
			break;
		}
		snapshot = snapshot->next;
	}

	// If it didn't exist, create a new one
	if ( !snapshot )
	{
		snapshot = PushZeroStruct( GlobalArena, SnapshotNode );
		if (engine.editor.snapshots) {
			snapshot->next = engine.editor.snapshots;
		}
		engine.editor.snapshots = snapshot;
		snapshot->filepath = InternString(filepath);
		snapshot->imageH = EditorLoadSnapshot(filepath, "editor_snapshot");
	}

	return snapshot;
}

static void EditorUpdateUI_MenuBar()
{
	Engine &engine = GetEngine();
	UI &ui = engine.ui;
	Graphics &gfx = engine.gfx;
	Scene &scene = engine.scene;
	Editor &editor = engine.editor;

	if ( UI_BeginMenuBar(ui) )
	{
		if (UI_BeginMenu(ui, "File"))
		{
			if ( UI_MenuItem(ui, "New scene") )
			{
				const EditorCommand command = { .type = EditorCommandNew };
				AddEditorCommand(command);
			}

			if ( UI_MenuItem(ui, "Load scene") )
			{
				editor.showLoadScene = true;
			}
			if ( UI_MenuItem(ui, "Save scene") )
			{
				editor.showSaveScene = true;
			}

			UI_Separator(ui);

			if ( UI_MenuItem(ui, "Load scene (BIN)") )
			{
				const EditorCommand command = { .type = EditorCommandLoadBin };
				AddEditorCommand(command);
			}

			if ( UI_MenuItem(ui, "Build scene (BIN)") )
			{
				const EditorCommand command = { .type = EditorCommandBuildBin };
				AddEditorCommand(command);
			}

			UI_Separator(ui);

			if ( UI_MenuItem(ui, "Quit") )
			{
				editor.showQuit = true;
			}
			UI_EndMenu(ui);
		}
		if (UI_BeginMenu(ui, "View"))
		{
			if ( UI_MenuItem(ui, "Outliner", editor.showOutliner) )
			{
				editor.showOutliner = !editor.showOutliner;
			}
			if ( UI_MenuItem(ui, "Assets", editor.showAssets) )
			{
				editor.showAssets = !editor.showAssets;
			}
			if ( UI_MenuItem(ui, "Inspector", editor.showInspector) )
			{
				editor.showInspector = !editor.showInspector;
			}
			if ( UI_MenuItem(ui, "Sprite Sheet", editor.showSpriteSheet) )
			{
				editor.showSpriteSheet = !editor.showSpriteSheet;
			}
			#if USE_PROFILE
			if ( UI_MenuItem(ui, "Profiler", editor.showProfiler) )
			{
				editor.showProfiler = !editor.showProfiler;
			}
			#endif // USE_PROFILE
			//if ( UI_MenuItem(ui, "Debug UI", editor.showDebugUI) )
			//{
			//	editor.showDebugUI = !editor.showDebugUI;
			//}

			UI_Separator(ui);

			if ( UI_MenuItem(ui, "Grid", editor.showGrid) )
			{
				editor.showGrid = !editor.showGrid;
			}

			UI_Separator(ui);

			if ( UI_MenuItem(ui, "Settings", editor.showSettings) )
			{
				editor.showSettings = !editor.showSettings;
			}

			UI_EndMenu(ui);
		}
		if (UI_BeginMenu(ui, "Help"))
		{
			if ( UI_MenuItem(ui, "About") )
			{
				editor.showAbout = true;
			}
			UI_EndMenu(ui);
		}

		UI_EndMenuBar(ui);
	}
}

static void EditorUpdateUI_ToolBar()
{
	Engine &engine = GetEngine();
	UI &ui = engine.ui;
	Scene &scene = engine.scene;
	EditorContext &context = engine.editor.context;

	if ( UI_BeginToolBar(ui) )
	{
		if ( UI_ButtonIcon(ui, 0) )
		{
			engine.game.state = GameStateStarting;
		}

		UI_Separator(ui);

		UI_Label(ui, "Camera");

		if ( UI_Radio(ui, "2D", EditorMode2D()) ) {
			EditorSetMode2D();
		}
		if ( UI_Radio(ui, "3D", EditorMode3D()) ) {
			EditorSetMode3D();
		}

		UI_Separator(ui);

		const Room *contextRoom = EditorGetContextRoom();
		const Layer *contextLayer = EditorGetContextLayer();
		if (contextRoom)
		{
			UI_Label(ui, "%s", contextRoom->name);

			if (contextLayer)
			{
				UI_Label(ui, "> %s", contextLayer->name);

				UI_Separator(ui);

				if (contextLayer->isCollider)
				{
					static EditorTool tool = EditorTool_Draw;
					if (UI_Radio(ui, "Solid", context.tool == EditorTool_ColliderSolid)) {
						context.tool = EditorTool_ColliderSolid;
					}
					if (UI_Radio(ui, "Platform", context.tool == EditorTool_ColliderPlatform)) {
						context.tool = EditorTool_ColliderPlatform;
					}
					if (UI_Radio(ui, "Erase", context.tool == EditorTool_Erase)) {
						context.tool = EditorTool_Erase;
					}
				}
				else
				{
					static EditorTool tool = EditorTool_Draw;
					if (UI_Radio(ui, "Draw", context.tool == EditorTool_Draw)) {
						context.tool = EditorTool_Draw;
					}
					if (UI_Radio(ui, "Erase", context.tool == EditorTool_Erase)) {
						context.tool = EditorTool_Erase;
					}
				}
			}
		}

		UI_EndToolBar(ui);
	}
}

static void EditorSelectScene()
{
	Editor &editor = GetEditor();
	editor.inspector.nextSelected.type = EditorSelectedType_Scene;
}

static void EditorSelectRoom(ID roomId)
{
	Editor &editor = GetEditor();
	editor.context.roomId = roomId;
	editor.context.layerIndex = EditorNoLayer;
	editor.inspector.nextSelected.type = EditorSelectedType_Room;
}

static void EditorUnselectRoom(ID roomId)
{
	Editor &editor = GetEditor();
	if (editor.context.roomId == roomId) {
		editor.context.roomId = {};
		editor.context.layerIndex = EditorNoLayer;
		editor.inspector.nextSelected.type = EditorSelectedType_None;
	}
}

static void EditorSelectLayer(ID roomId, u32 layerIndex)
{
	Editor &editor = GetEditor();
	editor.context.roomId = roomId;
	editor.context.layerIndex = layerIndex;
	editor.inspector.nextSelected.type = EditorSelectedType_Layer;
}

static void EditorUnselectLayer(ID roomId, u32 layerIndex)
{
	Editor &editor = GetEditor();
	if (editor.context.roomId == roomId && editor.context.layerIndex == layerIndex) {
		editor.context.layerIndex = EditorNoLayer;
		editor.inspector.nextSelected.type = EditorSelectedType_None;
	}
}

static void EditorSelectEntity(ID entityId)
{
	Editor &editor = GetEditor();
	editor.inspector.nextSelected.id = entityId;
	editor.inspector.nextSelected.type = EditorSelectedType_Entity;
}

static void EditorSelectMaterial(ID materialId)
{
	Editor &editor = GetEditor();
	editor.inspector.nextSelected.id = materialId;
	editor.inspector.nextSelected.type = EditorSelectedType_Material;
}

static void EditorSelectTexture(ID textureId)
{
	Editor &editor = GetEditor();
	editor.inspector.nextSelected.id = textureId;
	editor.inspector.nextSelected.type = EditorSelectedType_Texture;
}

static void EditorSelectAudioClip(ID clipId)
{
	Editor &editor = GetEditor();
	editor.inspector.nextSelected.id = clipId;
	editor.inspector.nextSelected.type = EditorSelectedType_Audio;
}

static void EditorSelectMusic(ID musicId)
{
	Editor &editor = GetEditor();
	editor.inspector.nextSelected.id = musicId;
	editor.inspector.nextSelected.type = EditorSelectedType_Music;
}

static void EditorSelectSprite(ID spriteId)
{
	Editor &editor = GetEditor();
	editor.context.spriteId = spriteId;
	editor.inspector.nextSelected.id = spriteId;
	editor.inspector.nextSelected.type = EditorSelectedType_Sprite;
}

static void EditorUnselectSprite(ID spriteId)
{
	Editor &editor = GetEditor();
	if (editor.context.spriteId == spriteId) {
		editor.context.spriteId = {};
		editor.inspector.nextSelected.type = EditorSelectedType_None;
	}
}


static void EditorSelectFileImage(FileNode *node)
{
	Editor &editor = GetEditor();
	editor.context.selectedFile = node;
	editor.inspector.nextSelected.file = node;
	editor.inspector.nextSelected.type = EditorSelectedType_FileImage;
}

static void EditorSelectFileAudio(FileNode *node)
{
	Editor &editor = GetEditor();
	editor.context.selectedFile = node;
	editor.inspector.nextSelected.file = node;
	editor.inspector.nextSelected.type = EditorSelectedType_FileAudio;
}

static void EditorSelectFileMusic(FileNode *node)
{
	Editor &editor = GetEditor();
	editor.context.selectedFile = node;
	editor.inspector.nextSelected.file = node;
	editor.inspector.nextSelected.type = EditorSelectedType_FileMusic;
}

static void EditorSelectFileUnknown(FileNode *node)
{
	Editor &editor = GetEditor();
	editor.context.selectedFile = node;
	editor.inspector.nextSelected.file = node;
	editor.inspector.nextSelected.type = EditorSelectedType_FileUnknown;
}

static void EditorUpdateInspectedAsset()
{
	Engine &engine = GetEngine();
	EditorInspector &inspector = engine.editor.inspector;

	const bool selectionMoved =
		inspector.selected.type != inspector.nextSelected.type ||
		inspector.selected.value != inspector.nextSelected.value;

	if ( !selectionMoved ) {
		return;
	}

	AudioStopAll(engine.audio);
	inspector.audioSourceIndex = U32_MAX;

	// All three share a union, only the one `selected` names is live
	if (inspector.selected.type == EditorSelectedType_FileImage) {
		WaitDeviceIdle(engine.gfx.device);
		RemoveTexture(engine.gfx, inspector.tmpTextureId);
	}
	if (inspector.selected.type == EditorSelectedType_FileAudio) {
		RemoveAudioClip(inspector.tmpAudioClipId);
	}
	if (inspector.selected.type == EditorSelectedType_FileMusic) {
		DestroyMusicFile(inspector.tmpMusicId);
	}
	inspector.tmpTextureId = {};

	inspector.selected.type = inspector.nextSelected.type;
	inspector.selected.value = inspector.nextSelected.value;

	const bool selectedFile =
		inspector.selected.type >= EditorSelectedType_FileBegin &&
		inspector.selected.type <= EditorSelectedType_FileEnd &&
		inspector.selected.file != nullptr;

	if ( !selectedFile ) {
		return;
	}

	const char *filename = inspector.selected.file->filename;

	if (inspector.selected.type == EditorSelectedType_FileImage) {
		const TextureDesc desc = {
			.name = InternString("inspected_image"),
			.filename = filename,
			.mipmap = true,
			.flags = AssetFlag_Ghost,
		};
		inspector.tmpTextureId = CreateTexture(engine.gfx, desc);
	}
	else if (inspector.selected.type == EditorSelectedType_FileAudio) {
		const AudioClipDesc desc = {
			.name = InternString("inspected_audio_clip"),
			.filename = filename,
			.flags = AssetFlag_Ghost,
		};
		inspector.tmpAudioClipId = CreateAudioClip(engine.audio, desc);
	}
	else if (inspector.selected.type == EditorSelectedType_FileMusic) {
		const MusicFileDesc desc = {
			.name = InternString("inspected_music_file"),
			.filename = filename,
			//.flags = AssetFlag_Ghost,
		};
		inspector.tmpMusicId = CreateMusicFile(engine.audio, desc);
	}
}

static void EditorUnselectAll()
{
	Editor &editor = GetEditor();

	editor.context.selectedFile = nullptr;
	editor.context.roomId = {};
	editor.context.layerIndex = EditorNoLayer;
	editor.context.spriteId = {};
	editor.spriteSheet.textureId = {};
	editor.inspector.nextSelected.value = 0;
	editor.inspector.nextSelected.type = EditorSelectedType_None;
	editor.inspector.audioSourceIndex = U32_MAX;
}

static void EditorRepointSpritesToDefaultTexture(Engine &engine, ID textureId)
{
	Scene &scene = engine.scene;
	for (u32 i = 0; i < scene.spriteCount; ++i)
	{
		SpriteDesc &sprite = scene.sprites[i].desc;
		if ( !Valid(sprite.textureId) )// == textureId )
		{
			LOG(Warning, "Sprite <%s> lost the texture it refers to, falling back to the default one.\n", sprite.name);
			sprite.textureId = engine.gfx.defaultTexture;
		}
	}
}

static void EditorRemoveSelection()
{
	Engine &engine = GetEngine();
	Editor &editor = engine.editor;

	const EditorSelection selection = editor.inspector.nextSelected;

	if ( !( selection.type >= EditorSelectedType_AssetBegin && selection.type <= EditorSelectedType_AssetEnd ) ) {
		return;
	}

	const ID assetId = selection.id;
	if ( IsBuiltin(assetId) ) {
		return;
	}

	switch ( selection.type )
	{
		case EditorSelectedType_Entity:
			editor.isTranslating = false;
			RemoveEntity(engine, assetId);
			break;

		case EditorSelectedType_Material:
			RemoveMaterial(engine.gfx, assetId);
			break;

		case EditorSelectedType_Texture:
			RemoveTexture(engine.gfx, assetId);
			EditorRepointSpritesToDefaultTexture(engine, assetId);
			break;

		case EditorSelectedType_Audio:
			for (u32 i = 0; i < ARRAY_COUNT(engine.audio.sources); ++i) {
				if ( engine.audio.sources[i].clip == assetId ) {
					StopAudioSource(i);
				}
			}
			RemoveAudioClip(assetId);
			break;

		case EditorSelectedType_Music:
			if ( engine.audio.musicFile == assetId ) {
				MusicStop(engine.audio);
				engine.audio.musicFile = {}; // So the next play loads its module afresh
			}
			DestroyMusicFile(assetId);
			break;

		case EditorSelectedType_Sprite:
			RemoveSprite(engine.scene, assetId);
			break;

		default:
			break;
	}

	EditorUnselectAll();
}

static void EditorRemoveAsset(EditorSelectedType type, ID assetId)
{
	Editor &editor = GetEditor();
	const EditorSelection previous = editor.inspector.nextSelected;

	editor.inspector.nextSelected.type = type;
	editor.inspector.nextSelected.id = assetId;
	EditorRemoveSelection();

	if ( !(previous.type == type && previous.id == assetId) ) {
		editor.inspector.nextSelected = previous;
	}
}

static void EditorAssetContextMenu(const char *name, EditorSelectedType type, ID assetId)
{
	Engine &engine = GetEngine();
	UI &ui = engine.ui;

	if ( IsBuiltin(assetId) ) { return; }

	UI_PushID(ui, assetId.slot);
	if (UI_BeginContextMenu(ui, name))
	{
		if (UI_MenuItem(ui, "Delete"))
		{
			EditorRemoveAsset(type, assetId);
		}
		UI_EndContextMenu(ui);
	}
	UI_PopID(ui);
}

static void EditorEntityDropTarget(ID layerId)
{
	Engine &engine = GetEngine();
	UI &ui = engine.ui;

	if ( UI_DragAndDropTarget(ui, "Entity") )
	{
		const ID droppedId = { UI_DragAndDropPayload(ui).uvalue };
		if ( droppedId ) {
			GetEntity(droppedId).layerId = layerId;
		}
	}
}

static void EditorUpdateUI_DebugUI()
{
	Engine &engine = GetEngine();
	UI &ui = engine.ui;
	Graphics &gfx = engine.gfx;
	Scene &scene = engine.scene;
	Editor &editor = engine.editor;

	UI_BeginWindow(ui, "Debug UI", &editor.showDebugUI);


	UI_EndWindow(ui);
}

static void EditorUpdateUI_Outliner()
{
	Engine &engine = GetEngine();
	UI &ui = engine.ui;
	Editor &editor = engine.editor;
	Scene &scene = engine.scene;
	Graphics &gfx = engine.gfx;
	Audio &audio = engine.audio;
	Game &game = engine.game;

	constexpr uint2 size = {200, 500};
	UI_SetNextWindowDefaultSize(ui, size);
	UI_SetNextWindowAnchor(ui, {0,0});
	UI_SetNextWindowDefaultDisplacement(ui, {10, 50});

	UI_BeginWindow(ui, "Outliner", &editor.showOutliner);

	if ( UI_Section(ui, "Hierarchy") )
	{
		Scratch scratch;
		Entity **entities = PushArray(scratch.arena, Entity*, MAX_ENTITIES);

		UI_BeginLayout(ui, UILayout_Horizontal);
		bool sceneIsOpen;
		if (UI_TreeNode(ui, "Scene", &scene, &sceneIsOpen))
		{
			EditorSelectScene();
		}
		EditorEntityDropTarget({}); // The way back out of a layer
		if (UI_BeginContextMenu(ui, "SceneContext"))
		{
			if (UI_MenuItem(ui, "Create room"))
			{
				CreateRoom(engine);
			}
			UI_EndContextMenu(ui);
		}

		UI_EndLayout(ui);

		if (sceneIsOpen)
		{
			UI_Indent(ui);

			UIID roomIndex = 0;
			for (u32 roomIdx = 0; roomIdx < scene.roomCount; ++roomIdx)
			{
				Room &room = scene.rooms[roomIdx];

				UI_BeginLayout(ui, UILayout_Horizontal);
				bool roomIsOpen;
				if (UI_TreeNode(ui, room.name, &room, &roomIsOpen))
				{
					EditorSelectRoom(scene.rooms[roomIdx].id);
				}
				// An entity belongs to a layer and not to a room, so the drop lands on the
				// one the room is measured by
				if ( const Layer *baseLayer = GetBaseLayer(room) ) {
					EditorEntityDropTarget(baseLayer->id);
				}
				UI_PushID(ui, roomIndex++);
				if (UI_BeginContextMenu(ui, "RoomContext"))
				{
					if (UI_MenuItem(ui, "Create layer"))
					{
						const LayerDesc desc = {
							.name = "Layer",
							.visible = true,
							.isCollider = false,
							.size = {TILE_GRID_SIZE_X, TILE_GRID_SIZE_Y},
						};
						CreateLayer(room, desc);
					}
					if (UI_MenuItem(ui, "Delete room"))
					{
						RemoveRoom(engine, scene.rooms[roomIdx].id);
					}
					UI_EndContextMenu(ui);
				}
				UI_PopID(ui);
				UI_EndLayout(ui);

				if (roomIsOpen)
				{
					UI_Indent(ui);

					for (u32 layerIndex = 0; layerIndex < ARRAY_COUNT(room.layers); ++layerIndex)
					{
						Layer &layer = room.layers[layerIndex];
						if (!layer.initialized) { continue; }

						UI_BeginLayout(ui, UILayout_Horizontal);

						bool layerIsOpen;
						// Keyed on the name within the room rather than on &layer: the slot address
						// changes when layers are reordered, which would leave the expanded state
						// behind with the slot instead of following the layer. The seed is the layer
						// array and not &room, so a layer named after its room does not collide with
						// the room's own node.
						if  (UI_TreeNode(ui, layer.name, &room.layers, &layerIsOpen))
						{
							EditorSelectLayer(scene.rooms[roomIdx].id, layerIndex);
						}
						EditorEntityDropTarget(layer.id);

						UI_SetCursorPosXFromRight(ui, 110);
						UI_ToggleIcon(ui, EditorIcon_Eye, &layer.visible);
						const bool moveDown = UI_ButtonIcon(ui, EditorIcon_Down);
						const bool moveUp = UI_ButtonIcon(ui, EditorIcon_Up);
						const bool removeLayer = !layer.isBase && UI_ButtonIcon(ui, EditorIcon_X);

						UI_EndLayout(ui);

						if ( layerIsOpen )
						{
							for (u32 i = 0; i < scene.entityCount; ++i)
							{
								const Entity &entity = scene.entities[i];

								if ( entity.layerId == layer.id )
								{
									if ( UI_Button(ui, entity.name) ) {
										EditorSelectEntity(entity.id);
									}
									UI_DragAndDropSource(ui, "Entity", UI_Payload(entity.id.slot), editor.iconAsset );
									EditorAssetContextMenu("EntityContext", EditorSelectedType_Entity, scene.entities[i].id);
								}
							}
						}

						const ID roomId = scene.rooms[roomIdx].id;

						if (moveDown || moveUp)
						{
							// The layer changes slot, so the selection, which holds its index in
							// the array, has to follow it to where it landed.
							const bool wasSelected = editor.context.roomId == roomId &&
								editor.context.layerIndex == layerIndex;
							const u32 movedIndex = MoveLayer(room, layerIndex, moveDown ? 1 : -1);
							if (wasSelected) {
								EditorSelectLayer(roomId, movedIndex);
							}
							break;
						}

						if (removeLayer)
						{
							EditorUnselectLayer(roomId, layerIndex);
							RemoveLayer(room, layerIndex);
							break;
						}
					}

					UI_Unindent(ui);
				}
			}

			for (u32 i = 0; i < scene.entityCount; ++i)
			{
				const Entity &entity = scene.entities[i];

				if ( !Valid(entity.layerId) )
				{
					if ( UI_Button(ui, entity.name) ) {
						EditorSelectEntity(scene.entities[i].id);
					}
					UI_DragAndDropSource(ui, "Entity", UI_Payload(entity.id.slot), editor.iconAsset );
					EditorAssetContextMenu("EntityContext", EditorSelectedType_Entity, scene.entities[i].id);
				}
			}

			UI_Unindent(ui);
		}
	}

	if ( UI_Section(ui, "Sprites") )
	{
		static ID selectedSprite = {};

		UI_BeginLayout(ui, UILayout_ItemBrowser);

		for (u32 i = 0; i < scene.spriteCount; ++i)
		{
			const SpriteDesc &sprite = scene.sprites[i].desc;
			const ID spriteId = sprite.id;
			const Texture &texture = GetTexture(sprite.textureId);
			const UIWidgetFlags flags = selectedSprite == spriteId ? UIWidgetFlag_Outline : UIWidgetFlag_None;
			const float2 uvPos = Float2(sprite.pos)/Float2(texture.size);
			const float2 uvSize = Float2(sprite.size)/Float2(texture.size);
			const float4 uvRect = Float4(uvPos, uvSize);

			if (UI_Image(ui, texture.image, float2{32, 32}, flags, uvRect))
			{
				EditorSelectSprite(spriteId);
				selectedSprite = spriteId;
			}

			UI_DragAndDropSource(ui, "Sprite", UI_Payload(spriteId.slot), texture.image, uvRect );

			EditorAssetContextMenu("SpriteContext", EditorSelectedType_Sprite, spriteId);
		}

		UI_EndLayout(ui);
	}

	if ( UI_Section(ui, "Materials") )
	{
		for (u32 i = 0; i < gfx.materialCount; ++i)
		{
			const MaterialDesc &desc = gfx.materials[i].desc;

			if ( UI_Button(ui, desc.name) ) {
				EditorSelectMaterial(desc.id);
			}
			EditorAssetContextMenu("MaterialContext", EditorSelectedType_Material, desc.id);
		}
	}

	if ( UI_Section(ui, "Textures") )
	{
		static ID selectedHandle = {};
		UI_BeginLayout(ui, UILayout_ItemBrowser);
		for (u16 i = 0; i < gfx.textureCount; ++i)
		{
			const Texture &texture = gfx.textures[i];
			const TextureDesc &desc = texture.desc;

			if ( (desc.flags & AssetFlag_Ghost) && !(desc.flags & AssetFlag_Builtin) ) { continue; }

			const ID textureId = desc.id;
			const UIWidgetFlags flags = selectedHandle == textureId ? UIWidgetFlag_Outline : UIWidgetFlag_None;

			if (UI_Image(ui, texture.image, float2{32, 32}, flags))
			{
				EditorSelectTexture(textureId);
				selectedHandle = textureId;
			}

			UI_DragAndDropSource(ui, "Texture", UI_Payload(textureId.slot), texture.image );

			EditorAssetContextMenu("TextureContext", EditorSelectedType_Texture, textureId);
		}
		UI_EndLayout(ui);
	}

	if ( UI_Section(ui, "AudioClips") )
	{
		for (u32 i = 0; i < audio.clipCount; ++i)
		{
			const AudioClipDesc &desc = audio.clips[i].desc;

			if ( UI_Button(ui, desc.name) ) {
				EditorSelectAudioClip(desc.id);
			}
			EditorAssetContextMenu("AudioClipContext", EditorSelectedType_Audio, desc.id);
		}
	}

	if ( UI_Section(ui, "MusicFiles") )
	{
		for (u32 i = 0; i < audio.musicFileCount; ++i)
		{
			const MusicFileDesc &desc = audio.musicFiles[i].desc;

			if ( UI_Button(ui, desc.name) ) {
				EditorSelectMusic(desc.id);
			}
			EditorAssetContextMenu("MusicFileContext", EditorSelectedType_Music, desc.id);
		}
	}

	if ( UI_Section(ui, "Scripts") )
	{
		for (u32 i = 0; i < game.scriptCount; ++i)
		{
			const Script &script = game.scripts[i];
			UI_TreeNode(ui, script.name, nullptr, nullptr, UITreeNodeFlag_Leaf);
			UI_DragAndDropSource(ui, "Script", UI_Payload((void*)&script), editor.iconAsset );
		}
	}

	UI_EndWindow(ui);
}


static bool IsWavFile(const char *filename)
{
	const bool isWav = HasFileExtension(filename, "wav");
	return isWav;
}

static bool IsMusicFile(const char *filename)
{
	const bool isMusic =
		HasFileExtension(filename, "mod") ||
		HasFileExtension(filename, "s3m") ||
		HasFileExtension(filename, "xm");
	return isMusic;
}

static bool IsImgFile(const char *filename)
{
	const bool isImg =
		HasFileExtension(filename, "png") ||
		HasFileExtension(filename, "bmp") ||
		HasFileExtension(filename, "jpg");
	return isImg;
}

static void EditorUpdateUI_Assets()
{
	Engine &engine = GetEngine();
	UI &ui = engine.ui;
	Graphics &gfx = engine.gfx;
	Scene &scene = engine.scene;
	Editor &editor = engine.editor;
	EditorContext &context = editor.context;
	EditorInspector &inspector = editor.inspector;

	constexpr uint2 size = {500, 200};
	constexpr float2 displacement = {10, -10};
	UI_SetNextWindowDefaultSize(ui, size);
	UI_SetNextWindowAnchor(ui, {0, 1});
	UI_SetNextWindowDefaultDisplacement(ui, displacement);

	UI_BeginWindow(ui, "Assets", &editor.showAssets);

	UI_BeginLayout(ui, UILayout_ItemBrowser);

	static char selectedName[MAX_PATH_LENGTH] = {};
	FilePath path;

	FileNode *node = editor.root;
	while (node)
	{
		const bool isImg = node->type == FileNodeType_Image;
		const bool isMusic = node->type == FileNodeType_Music;
		const bool isWav = node->type == FileNodeType_Sound;

		path = MakePath(AssetDir, node->filename);

		// For images, we get a snapshot
		ImageH iconImg = editor.iconImg;
		if ( isImg ) {
			SnapshotNode *snapshot = EditorGetOrCreateSnapshotNode(path.str);
			if ( snapshot ) {
				iconImg = snapshot->imageH;
			}
		}

		const ImageH icon =
			isWav ? editor.iconWav :
			isMusic ? editor.iconMod :
			isImg ? iconImg :
			editor.iconAsset;

		const bool isSelected = context.selectedFile ? StrEq(path.str, context.selectedFile->filename) : false;
		const UIWidgetFlags flags = isSelected ? UIWidgetFlag_Outline : UIWidgetFlag_None;

		if ( UI_Image(ui, icon, float2{32,32}, flags) )
		{
			if ( isWav )
			{
				EditorSelectFileAudio(node);
			}
			else if ( isMusic )
			{
				EditorSelectFileMusic(node);
			}
			else if ( isImg )
			{
				EditorSelectFileImage(node);
			}
			else
			{
				EditorSelectFileUnknown(node);
			}
		}

		UI_DragAndDropSource(ui, "FileNode", UI_Payload(node), icon );

		node = node->next;
	}

	UI_EndLayout(ui);

	UI_EndWindow(ui);
}

static const char *EditorMakeSpriteName(const Scene &scene, const char *textureName)
{
	const char *base = StrEqN(textureName, "tex_", 4) ? textureName + 4 : textureName;

	const char *name = MakeName("spr_%s", base);
	for (u32 attempt = 1; FindSprite(scene, name); ++attempt) {
		name = MakeName("spr_%s_%u", base, attempt);
	}
	return name;
}

static void EditorUpdateUI_SpriteSheet()
{
	Engine &engine = GetEngine();
	UI &ui = engine.ui;
	Graphics &gfx = engine.gfx;
	Scene &scene = engine.scene;
	Editor &editor = engine.editor;
	EditorSpriteSheet &sheet = editor.spriteSheet;

	constexpr uint2 size = {320, 560};
	UI_SetNextWindowDefaultSize(ui, size);
	UI_SetNextWindowAnchor(ui, {1, 0});
	UI_SetNextWindowDefaultDisplacement(ui, {-220, 50});

	UI_BeginWindow(ui, "Sprite Sheet", &editor.showSpriteSheet);

	UI_SeparatorLabel(ui, "Texture");
	{
		UI_BeginLayout(ui, UILayout_ItemBrowser);
		for (u16 i = 0; i < gfx.textureCount; ++i)
		{
			const Texture &texture = gfx.textures[i];
			const TextureDesc &desc = texture.desc;
			const ID textureId = desc.id;
			if ( desc.flags & AssetFlag_Ghost ) {
				continue;
			}

			const UIWidgetFlags flags = textureId == sheet.textureId ? UIWidgetFlag_Outline : UIWidgetFlag_None;

			if ( UI_Image(ui, texture.image, float2{32, 32}, flags) )
			{
				sheet.textureId = textureId;
			}
		}
		UI_EndLayout(ui);
	}

	if ( sheet.textureId )
	{
		const Texture &texture = GetTexture(sheet.textureId);

		UI_SeparatorLabel(ui, "Texture");

		UI_Text(ui, "Name", "%s", texture.desc.name);
		UI_Text(ui, "Size", "%u x %u", texture.size.x, texture.size.y);

		UI_SeparatorLabel(ui, "Sprite sheet");
		{
			u32 sheetSpriteCount = 0;
			for (u32 i = 0; i < scene.spriteCount; ++i) {
				if ( scene.sprites[i].desc.textureId == sheet.textureId ) {
					sheetSpriteCount++;
				}
			}
			UI_Text(ui, "Sprite count", "%u", sheetSpriteCount);

			// One cell is the smallest region that can be picked: sprite offsets snap to
			// it and sprite sizes are whole multiples of it
			static const char *gridSizes[] = { "16", "32", "64" };

			static u32 gridEnum = 0;
			UI_Combo(ui, "Grid size", gridSizes, ARRAY_COUNT(gridSizes), &gridEnum);
			const u32 grid =
				gridEnum == 0 ? 16 :
				gridEnum == 1 ? 32 :
				64;

			constexpr f32 maxPreviewHeight = 320.0f;

			const UIWindow &window = UI_GetCurrentWindow(ui);
			const float2 textureSize = Float2(texture.size);
			const float2 available = { UI_GetContainerSize(window).x, maxPreviewHeight };

			const float2 imagePos = UI_GetCursorPos(ui);
			const float2 imageSize = textureSize;

			UI_Image(ui, texture.image, imageSize);

			// Cell under the cursor, clamped so a whole cell always fits inside the
			// texture and the sprite never samples past the edge of its sheet
			const float2 maxLocal = Max(float2{0, 0}, textureSize - float2{(f32)grid, (f32)grid});
			const float2 local = Min(Max(Float2(UI_MousePos(ui)) - imagePos, float2{0, 0}), maxLocal);
			const uint2 cell = { AlignDown((u32)local.x, grid), AlignDown((u32)local.y, grid) };

			static bool spritesheetDrag = false;
			static uint2 spriteRelPos = {};
			static uint2 spriteSize = {};
			if (UI_LastWidgetPressed(ui)) {
				// The offset is decided here and stays put for the rest of the drag
				spritesheetDrag = true;
				spriteRelPos = cell;
			}
			if (spritesheetDrag)
			{
				// Dragging only resizes, growing from the anchor cell towards the cursor
				spriteSize = {
					cell.x > spriteRelPos.x ? cell.x - spriteRelPos.x + grid : grid,
					cell.y > spriteRelPos.y ? cell.y - spriteRelPos.y + grid : grid,
				};

				if (UI_IsMouseRelease(ui)) {
					spritesheetDrag = false;
				}
			}

			// The outline goes into a draw list of its own, pushed after UI_Image popped
			// the one binding the texture. Sibling draw lists are drawn in push order,
			// so this one lands on top of the image instead of under it.
			const float2 spritePos = imagePos + Float2(spriteRelPos);
			UI_PushDrawList(ui, ui.fontAtlasH);
			UI_PushColor(ui, ui.style.accentColor);
			UI_AddBorder(ui, spritePos, Float2(spriteSize), 1);
			UI_PopColor(ui);
			UI_PopDrawList(ui);

			if ( spriteSize.x > 0 && spriteSize.y > 0 )
			{
				UI_Text(ui, "Region", "%u, %u (%u x %u)",
						spriteRelPos.x, spriteRelPos.y, spriteSize.x, spriteSize.y);

				if ( UI_Button(ui, "Create sprite") )
				{
					if ( FindSprite(scene, sheet.textureId, spriteRelPos, spriteSize) )
					{
						LOG(Warning, "A sprite already covers that region of %s.\n", texture.desc.name);
					}
					else if ( scene.spriteCount >= MAX_SPRITES )
					{
						LOG(Warning, "Sprite limit reached (%u).\n", MAX_SPRITES);
					}
					else
					{
						const SpriteDesc desc = {
							.name = EditorMakeSpriteName(scene, texture.desc.name),
							.textureId = texture.desc.id,
							.pos = spriteRelPos,
							.size = spriteSize,
						};
						CreateSprite(engine, desc);
					}
				}
			}
		}
	}
	else
	{
		UI_Label(ui, "Select a texture.");
	}

	UI_EndWindow(ui);
}

static void EditorUpdateUI_Property(const Property &property, void *data)
{
	UI &ui = GetEngine().ui;

	PropertyValue value = GetPropertyValue(property, data);

	switch (property.type)
	{
		case Property_U32:
		{
			if ( UI_InputUInt(ui, property.name, &value.uValue) ) {
				SetPropertyValue(property, data, value);
			}
			break;
		}
		case Property_ID:
		{
			Entity *entity = nullptr;
			if ( value.idValue )
			{
				entity = &GetEntity(value.idValue);
			}
			UI_Text(ui, property.name, "%s", entity ? entity->name : "<none>");
			if ( UI_DragAndDropTarget(ui, "Entity") )
			{
				value.idValue = { UI_DragAndDropPayload(ui).uvalue };
				SetPropertyValue(property, data, value);
			}
			break;
		}
	}
}

static void EditorUpdateUI_InspectorProperties(const char *scriptName, const Property *properties, u32 propertyCount, void *base)
{
	UI &ui = GetEngine().ui;

	UI_SeparatorLabel(ui, scriptName);

	for (u32 i = 0; i < propertyCount; ++i)
	{
		const Property &property = properties[i];

		EditorUpdateUI_Property(property, base);
	}
}

static void EditorUpdateUI_InspectorScene(Scene &scene)
{
	Engine &engine = GetEngine();
	UI &ui = engine.ui;
	UI_Text(ui, "Rooms", "%u", scene.roomCount);
	UI_Text(ui, "Entities", "%u", scene.entityCount);
	UI_Text(ui, "Sprites", "%u", scene.spriteCount);

	UI_Combo(ui, "Projection", (const char **)ProjectionTypeStr, ProjectionTypeCount, (u32*)&scene.projectionType);
}

static void EditorUpdateUI_InspectorRoom(Room &room)
{
	Engine &engine = GetEngine();
	UI &ui = engine.ui;

	UI_SeparatorLabel(ui, "Room");

	static char name[64];
	StrCopy(name, room.name);
	UI_InputText(ui, "room#Name", name, ARRAY_COUNT(name));
	room.name = InternString(name);

	UI_InputInt2(ui, "Pos", &room.pos);
	UI_Text(ui, "Layers", "%u", room.layerCount);
}

static void EditorUpdateUI_InspectorLayer(Layer &layer)
{
	Engine &engine = GetEngine();
	UI &ui = engine.ui;
	EditorInspector &inspector = engine.editor.inspector;

	UI_SeparatorLabel(ui, "Layer");

	static char name[64];
	StrCopy(name, layer.name);
	UI_InputText(ui, "layer#Name", name, ARRAY_COUNT(name));
	layer.name = InternString(name);

	UI_Text(ui, "ID", "%u", layer.id.slot);

	UI_InputUInt2(ui, "Size", &layer.size);
	UI_Text(ui, "Base", "%s", layer.isBase ? "yes" : "no"); // Set on creation, not editable here
	UI_Checkbox(ui, "Visible", &layer.visible);
	UI_Checkbox(ui, "Collider", &layer.isCollider);
}

static void EditorUpdateUI_Inspector()
{
	Engine &engine = GetEngine();
	UI &ui = engine.ui;
	Editor &editor = engine.editor;
	EditorInspector &inspector = editor.inspector;
	EditorContext &context = editor.context;

	constexpr uint2 size = {200, 500};
	constexpr float2 displacement = {-10, 50};
	UI_SetNextWindowDefaultSize(ui, size);
	UI_SetNextWindowDefaultDisplacement(ui, displacement);
	UI_SetNextWindowAnchor(ui, {1, 0});

	UI_BeginWindow(ui, "Inspector", &editor.showInspector);

	UI_SeparatorLabel(ui, "%s", EditorSelectedTypeName[inspector.selected.type]);

	if (inspector.selected.file &&
		inspector.selected.type >= EditorSelectedType_FileBegin &&
		inspector.selected.type <= EditorSelectedType_FileEnd)
	{
		const char *filename = inspector.selected.file->filename;
		FilePath filepath = MakePath(AssetDir, filename);

		u64 fileSize = 0;
		GetFileSize(filepath.str, fileSize, false);

		UI_Text(ui, "File", "%s", filename);
		UI_Text(ui, "Size", "%llu KB", fileSize / 1024);

		if (inspector.selected.type == EditorSelectedType_FileImage)
		{
			UI_SeparatorLabel(ui, "Image");

			const ImageH imageH = GetTextureImage(engine.gfx, inspector.tmpTextureId, engine.gfx.grayImageH);
			UI_Image(ui, imageH, float2{0,0}, UIWidgetFlag_Expand);

			if ( UI_Button(ui, "Import texture") )
			{
				const char *basename = NameFromFilename(filename);
				const char *texname = MakeName("tex_%s", basename);

				const TextureDesc textureDesc = {
					.name = texname,
					.filename = filename,
					.mipmap = true,
				};
				ID textureId = GetOrCreateTexture(engine.gfx, textureDesc);

				EditorSelectTexture(textureId);
			}
		}
		else if (inspector.selected.type == EditorSelectedType_FileAudio)
		{
			UI_SeparatorLabel(ui, "Audio");

			if (inspector.tmpAudioClipId)
			{
				if (UI_Button(ui, "Play")) {
					inspector.audioSourceIndex = PlayAudioClip(engine.audio, inspector.tmpAudioClipId);
				}
				if (UI_Button(ui, "Stop")) {
					StopAudioSource(inspector.audioSourceIndex);
					inspector.audioSourceIndex = U32_MAX;
				}
			}
		}
		else if (inspector.selected.type == EditorSelectedType_FileMusic)
		{
			UI_SeparatorLabel(ui, "Music");

			if (inspector.tmpMusicId)
			{
				if (UI_Button(ui, "Play")) {
					MusicPlay(engine, inspector.tmpMusicId);
				}
				if (UI_Button(ui, "Stop")) {
					MusicStop(engine.audio);
				}
			}
		}
		else if (inspector.selected.type == EditorSelectedType_FileUnknown)
		{
			// Nothing to do
		}
	}
	else
	{
		if (inspector.selected.type == EditorSelectedType_Scene)
		{
			EditorUpdateUI_InspectorScene(engine.scene);
		}
		else if (inspector.selected.type == EditorSelectedType_Room)
		{
			EditorUpdateUI_InspectorScene(engine.scene);
			if ( Room *room = EditorGetContextRoom() ) {
				EditorUpdateUI_InspectorRoom(*room);
			}
		}
		else if (inspector.selected.type == EditorSelectedType_Layer)
		{
			EditorUpdateUI_InspectorScene(engine.scene);
			Room *room = EditorGetContextRoom();
			Layer *layer = EditorGetContextLayer();
			if ( room && layer ) {
				EditorUpdateUI_InspectorRoom(*room);
				EditorUpdateUI_InspectorLayer(*layer);
			}
		}
		else if(inspector.selected.type == EditorSelectedType_Entity)
		{
			// The ID goes invalid the moment the entity is removed, so this can be a
			// stale selection even though the element survives until CompactEntities
			if (inspector.selected.id)
			{
				Entity &entity = GetEntity(inspector.selected.id);

				static char name[64];
				StrCopy(name, entity.name);
				UI_InputText(ui, "Name", name, ARRAY_COUNT(name));
				entity.name = InternString(name);

				float3 entityPos = entity.position;
				if (UI_InputFloat3(ui, "Pos", &entityPos)) {
					EntitySetPosition(entity, entityPos);
				}
				UI_InputFloat(ui, "Scale", &entity.scale);
				UI_Checkbox(ui, "Visible", &entity.visible);

				UI_SeparatorLabel(ui, "Sprite");

				const SpriteDesc *sprite = entity.spriteId ? &GetSprite(entity.spriteId).desc : nullptr;
				UI_Text(ui, "Name", "%s", sprite ? sprite->name : "<none>");
				if ( UI_DragAndDropTarget(ui, "Sprite") )
				{
					const ID droppedId = { UI_DragAndDropPayload(ui).uvalue };
					if ( droppedId ) {
						entity.spriteId = droppedId;
					}
				}

				if (sprite && UI_Button(ui, "Go to sprite"))
				{
					EditorSelectSprite(entity.spriteId);
				}

				for (u32 i = 0; i < engine.game.scriptInstanceCount; ++i) 
				{
					const ScriptInstance &instance = engine.game.scriptInstances[i];
					if ( instance.scriptIndex >= engine.game.scriptCount ) {
						continue;
					}

					if ( instance.entity == inspector.selected.id )
					{
						const Script &script = engine.game.scripts[instance.scriptIndex];
						const Property *properties = engine.game.properties + script.propertyFirst;
						const u32 propertyCount = script.propertyCount;
						void * base = engine.game.scriptInstanceData + instance.offset;
						EditorUpdateUI_InspectorProperties(instance.scriptName, properties, propertyCount, base);
						if (UI_Button(ui, "Remove"))
						{
							RemoveScript(engine.game, i);
						}
					}
				}

				UI_SeparatorLabel(ui, "New script");

				UI_Text(ui, "Script", "<drag here>");
				if ( UI_DragAndDropTarget(ui, "Script") )
				{
					const Script &script = *(Script*)UI_DragAndDropPayload(ui).ptr;
					AddScript(engine.game, inspector.selected.id, script.name);
				}
			}
		}
		else if (inspector.selected.type == EditorSelectedType_Material)
		{
			if (inspector.selected.id)
			{
				MaterialDesc &desc = GetMaterial(inspector.selected.id).desc;

				static char name[64];
				StrCopy(name, desc.name);
				UI_InputText(ui, "Name", name, ARRAY_COUNT(name));
				desc.name = InternString(name);

				// Resolved into a pipeline index on creation, so it is not editable here
				UI_Text(ui, "Pipeline", "%s", desc.pipelineName ? desc.pipelineName : "");

				if (UI_InputFloat(ui, "UV scale", &desc.uvScale)) {
					engine.gfx.shouldUpdateMaterials = true; // uvScale is what the buffer holds
				}

				UI_SeparatorLabel(ui, "Texture");

				// A material outlives the texture it names, and drawing it pink is the point,
				// so the row reads as empty rather than resolving one
				const Texture *texture = desc.textureId ? &GetTexture(desc.textureId) : nullptr;
				UI_Text(ui, "Name", "%s", texture ? texture->desc.name : "<none>");
				if ( UI_DragAndDropTarget(ui, "Texture") )
				{
					const ID droppedId = { UI_DragAndDropPayload(ui).uvalue };
					if ( droppedId ) {
						desc.textureId = droppedId;
						engine.gfx.shouldUpdateMaterialBindGroups = true; // albedo is bound there
					}
				}

				const ImageH imageH = GetTextureImage(engine.gfx, desc.textureId, engine.gfx.grayImageH);
				UI_Image(ui, imageH, float2{0, 0}, UIWidgetFlag_Expand);

				if (texture && UI_Button(ui, "Go to texture"))
				{
					EditorSelectTexture(desc.textureId);
				}
			}
		}
		else if (inspector.selected.type == EditorSelectedType_Texture)
		{
			if (inspector.selected.id)
			{
				const Texture &texture = GetTexture(inspector.selected.id);
				UI_Text(ui, "Name", "%s", texture.desc.name);
				UI_Text(ui, "Size", "%u x %u", texture.size.x, texture.size.y);

				const ImageH imageH = GetTextureImage(engine.gfx, inspector.selected.id, engine.gfx.grayImageH);
				UI_Image(ui, imageH, float2{0,0}, UIWidgetFlag_Expand);

				UI_SeparatorLabel(ui, "Sprite");
				UI_BeginLayout(ui, UILayout_Horizontal);
				if (UI_Button(ui, "Sprite sheet..."))
				{
					editor.spriteSheet.textureId = inspector.selected.id;
					editor.showSpriteSheet = true;
				}
				UI_EndLayout(ui);
			}
		}
		else if (inspector.selected.type == EditorSelectedType_Audio)
		{
			if (inspector.selected.id)
			{
				if (UI_Button(ui, "Play")) {
					inspector.audioSourceIndex = PlayAudioClip(engine.audio, inspector.selected.id);
				}
				if (UI_Button(ui, "Stop")) {
					StopAudioSource(inspector.audioSourceIndex);
					inspector.audioSourceIndex = U32_MAX;
				}
			}
		}
		else if (inspector.selected.type == EditorSelectedType_Music)
		{
			if (inspector.selected.id)
			{
				if (UI_Button(ui, "Play")) {
					MusicPlay(engine, inspector.selected.id);
				}
				if (UI_Button(ui, "Stop")) {
					MusicStop(engine.audio);
				}
			}
		}
		else if (inspector.selected.type == EditorSelectedType_Sprite)
		{
			if (inspector.selected.id)
			{
				SpriteDesc &sprite = GetSprite(inspector.selected.id).desc;

				static char name[64];
				StrCopy(name, sprite.name);
				UI_InputText(ui, "Name", name, ARRAY_COUNT(name));
				sprite.name = InternString(name);

				if (sprite.textureId)
				{
					const Texture &texture = GetTexture(sprite.textureId);
					const float4 uvRect = {
						.xy = Float2(sprite.pos)/Float2(texture.size),
						.zw = Float2(sprite.size)/Float2(texture.size),
					};
					UI_Text(ui, "Texture", texture.desc.name);
					if ( UI_DragAndDropTarget(ui, "Texture") )
					{
						ID droppedId = { UI_DragAndDropPayload(ui).uvalue };
						if ( droppedId ) {
							sprite.textureId = droppedId;
						}
					}
					UI_Image(ui, texture.image, float2{0, 0}, UIWidgetFlag_Expand, uvRect);
				}

				int2 pos  = { (i32)sprite.pos.x,  (i32)sprite.pos.y  };
				int2 size = { (i32)sprite.size.x, (i32)sprite.size.y };
				i32 frameCount = (i32)sprite.frameCount;
				i32 fps        = (i32)sprite.fps;
				bool loop      = sprite.loop != 0;

				const uint2 oldPos  = sprite.pos;
				const uint2 oldSize = sprite.size;
				const u32   oldFrameCount     = sprite.frameCount;
				const u32   oldFps            = sprite.fps;

				UI_InputInt2(ui, "Pos",  &pos);
				UI_InputInt2(ui, "Size", &size);
				UI_InputInt(ui, "Frame Count", &frameCount);
				UI_InputInt(ui, "FPS",         &fps);
				UI_Checkbox(ui, "Loop",        &loop);

				sprite.pos  = { (u32)Max(0, pos.x),  (u32)Max(0, pos.y)  };
				sprite.size = { (u32)Max(0, size.x), (u32)Max(0, size.y) };
				sprite.frameCount     = (u32)Max(0, frameCount);
				sprite.fps            = (u32)Max(0, fps);
				sprite.loop           = loop ? 1 : 0;
			}
		}
	}

	UI_EndWindow(ui);
}

#if USE_PROFILE
// Meter fill by weight of the node in the frame, so the expensive rows stand out
constexpr float4 UiColorProfileCold = { 0.10, 0.30, 0.60, 1.0 };
constexpr float4 UiColorProfileWarm = { 0.70, 0.50, 0.10, 1.0 };
constexpr float4 UiColorProfileHot  = { 0.70, 0.20, 0.15, 1.0 };

// Position of a timestamp within the frame, from 0 (frame begin) to 1 (frame end)
static f32 ProfileNormalizedTime(ProfileTime time, ProfileTime frameBegin, ProfileTime frameTicks)
{
	const f32 res = ( frameTicks > 0 && time > frameBegin ) ? (f32)( time - frameBegin ) / (f32)frameTicks : 0.0f;
	return res;
}

// One row of the profiler tree: the node name on the left, and on the right a bar
// spanning the slice of the frame the node was running, so the column reads as a
// flame graph from top to bottom.
static bool UI_ProfilerTreeNode(UI &ui, bool *isOpen, u32 depth, bool hasChildren, const char *nodeName, f32 start, f32 end, f32 millis, f32 pct)
{
	void *ctx = (void*)isOpen;
	u32 flags = hasChildren ? UITreeNodeFlag_None : UITreeNodeFlag_Leaf;
	UI_TableNextRow(ui);
	UI_TableNextColumn(ui);
	UI_MoveCursorRight(ui, (f32)depth * UI_GetIndentWidth(ui, UI_StyleSpacing));
	UI_TreeNode(ui, nodeName, ctx, isOpen, flags);
	UI_TableNextColumn(ui);

	const float4 track = UI_GetElemColor(ui, UIElementMeter).base;
	const float4 heat = pct > 50.0f ? UiColorProfileHot : ( pct > 20.0f ? UiColorProfileWarm : UiColorProfileCold );
	UI_PushElemColor(ui, UIElementMeter, { track, heat, heat, track });
	UI_MeterRange(ui, start, end, "%.3f ms (%.2f %%)", millis, pct);
	UI_PopElemColor(ui, UIElementMeter);

	return *isOpen;
}

// Draws the node tree of a profiled frame (either a CPU thread or the GPU) as a table of rows
static void UI_ProfilerFrame(UI &ui, const char *frameName, const ProfileNode *nodes, u32 nodeCount, ProfileTime frameBegin, ProfileTime frameEnd)
{
	const ProfileTime frameTicks = frameEnd - frameBegin;
	const f32 frameMillis = 1000.0f * SecondsFromTicks(frameTicks);

	UI_BeginTable(ui, frameName, 2);
	UI_TableSetupColumn(ui, "Name", UITableColumnSizing_Stretch, 0.35f);
	UI_TableSetupColumn(ui, "Time", UITableColumnSizing_Stretch, 0.65f);

	bool isFrameOpen = true;
	if ( UI_ProfilerTreeNode(ui, &isFrameOpen, 0, true, "Frame", 0.0f, 1.0f, frameMillis, 100.0f) )
	{
		bool hasChildren[MAX_PROFILE_NODES] = {};
		for (u32 i = 0; i < nodeCount; ++i) {
			const ProfileNode *node = &nodes[i];
			if (node->parentIndex != PROFILE_NODE_NONE) {
				hasChildren[node->parentIndex] = true;
			}
		}

		bool isOpen[MAX_PROFILE_NODES] = {};
		u16 depth[MAX_PROFILE_NODES] = {};

		for (u32 i = 0; i < nodeCount; ++i)
		{
			const ProfileNode *node = &nodes[i];

			depth[i] = node->parentIndex == PROFILE_NODE_NONE ? 1 : depth[node->parentIndex] + 1;

			if (node->parentIndex == PROFILE_NODE_NONE || isOpen[node->parentIndex])
			{
				const char *name = ProfileGetName(node->nameId);
				const f32 millis = 1000.0f * SecondsFromTicks(node->end - node->begin);
				const f32 pct = frameMillis > 0.0f ? 100.0f * millis / frameMillis : 0.0f;
				const f32 start = ProfileNormalizedTime(node->begin, frameBegin, frameTicks);
				const f32 end = ProfileNormalizedTime(node->end, frameBegin, frameTicks);
				UI_ProfilerTreeNode(ui, &isOpen[i], depth[i], hasChildren[i], name, start, end, millis, pct);
			}
		}
	}

	UI_EndTable(ui);
}

static void EditorUpdateUI_Profiler()
{
	Engine &engine = GetEngine();
	UI &ui = engine.ui;
	const Graphics &gfx = engine.gfx;
	Editor &editor = engine.editor;
	EditorInspector &inspector = editor.inspector;
	EditorContext &context = editor.context;

	constexpr uint2 size = {500, 500};
	UI_SetNextWindowDefaultSize(ui, size);
	UI_SetNextWindowAnchor(ui, {0.5, 0.5});

	UI_BeginWindow(ui, "Profiler", &editor.showProfiler);

	const ProfileFrame frame = ProfileGetFrame(0);
	const f32 frameMillis = 1000.0f*SecondsFromTicks(frame.end - frame.begin);

	const f32 xcenter = (f32)(frame.index % MAX_PROFILE_FRAMES) / MAX_PROFILE_FRAMES;
	const f32 xleft = Max(0.0f, xcenter - 0.05f);
	const f32 xright = Min(1.0f, xcenter + 0.05f);
	const float2 a = { xleft, 0.0f };
	const float2 b = { xright, 1.0f };
	UI_BeginCanvas(ui);
	UI_DrawBox(ui, a, b);
	UI_EndCanvas(ui);

	const char *threadNames[MAX_PROFILE_THREADS] = {};
	u32 threadNameCount = 0;

	const u32 threadCount = ProfileGetThreadCount();
	for (u32 t = 0; t < threadCount; ++t)
	{
		if (ProfileGetThreadFrameCount(t) > 0)
		{
			const char *threadName = ProfileGetThreadName(t);
			bool threadProcessed = false;
			for ( u32 i = 0; i < threadNameCount; ++i ) {
				if ( StrEq(threadName, threadNames[i]) ) {
					threadProcessed = true;
					break;
				}
			}
			if ( threadProcessed ) {
				continue;
			}
			threadNames[threadNameCount++] = threadName;

			const ProfileFrame frame = ProfileGetThreadFrame(t, 0); // by-value snapshot

			if (UI_Section(ui, threadName))
			{
				UI_ProfilerFrame(ui, threadName, frame.nodes, frame.nodeCount, frame.begin, frame.end);

				UI_Text(ui, "Dropped events", "%u", frame.droppedEventCount);
			}
		}
	}

#if USE_PROFILE_GPU
	// The GPU timeline lags MAX_FRAMES_IN_FLIGHT frames behind the CPU one, and its clock is
	// unrelated to the CPU one, so only durations are comparable between both timelines.
	if (ProfileGpuGetFrameCount() > 0)
	{
		const ProfileGpuFrame gpuFrame = ProfileGpuGetFrame(0);

		if (UI_Section(ui, "GPU"))
		{
			UI_ProfilerFrame(ui, "GPU", gpuFrame.nodes, gpuFrame.nodeCount, gpuFrame.begin, gpuFrame.end);

			UI_Text(ui, "Dropped events", "%u", gpuFrame.droppedEventCount);
		}
	}
#endif // USE_PROFILE_GPU

	if ( UI_Section(ui, "Memory") )
	{
		UI_BeginLayout(ui, UILayout_Horizontal);
		const char *unitsStrArray[] = { "B", "KB", "MB" };
		const u32 unitsSizeArray[] = { 1, KB(1), MB(1) };
		static u32 units = 0;
		for (u32 i = 0; i < ARRAY_COUNT(unitsStrArray); ++i) {
			if ( UI_Radio(ui, unitsStrArray[i], units == i) ) {
				units = i;
			}
		}
		UI_EndLayout(ui);

		const char *unitsStr = unitsStrArray[units];
		const u32 unitsSize = unitsSizeArray[units];
		UI_Text(ui, "Global Arena", "%u / %u %s", GlobalArena.used / unitsSize, GlobalArena.size / unitsSize, unitsStr);
		UI_Text(ui, "Frame Arena", "%u / %u %s", FrameArena.used / unitsSize, FrameArena.size / unitsSize, unitsStr);
		UI_Text(ui, "String Arena", "%u / %u %s", StringArena.used / unitsSize, StringArena.size / unitsSize, unitsStr);
		UI_Text(ui, "Data Arena", "%u / %u %s", DataArena.used / unitsSize, DataArena.size / unitsSize, unitsStr);
	}

	UI_EndWindow(ui);
}
#endif // USE_PROFILE

static void EditorUpdateUI_About()
{
	Engine &engine = GetEngine();
	UI &ui = engine.ui;
	Editor &editor = engine.editor;

	UI_SetNextWindowModal(ui);
	UI_SetNextWindowAnchor(ui, {0.5, 0.5});
	UI_SetNextWindowSize(ui, uint2{ 512, 350 });
	UI_SetNextWindowDefaultDisplacement(ui, {0, 0});

	UI_BeginWindow(ui, "About", nullptr, UIWindowFlag_Border | UIWindowFlag_Background);

	UI_RaiseWindow(ui);
	UI_FocusWindow(ui);

	UI_Label(ui, "");
	UI_Image(ui, editor.iluLogo, float2{256, 256}, UIWidgetFlag_Centered);
	UI_Label(ui, "                          ILU engine");
	UI_Label(ui, "                     (by Jesus Diaz Garcia)");

	UI_EndWindow(ui);

	static bool wasShown = false;
	if ( UI_IsMousePressWithAnyButton(ui) && wasShown ) {
		engine.editor.showAbout = false;
	}
	wasShown = engine.editor.showAbout;
}

static ID EditorSpawnEntityAtMouse(ID spriteId)
{
	Engine &engine = GetEngine();

	if ( !EditorMode2D() )
	{
		LOG(Debug, "Drag and Drop not implemented in 3D mode.\n");
		return {};
	}

	const Mouse &mouse = GetWindow().mouse;
	const Camera &camera = engine.editor.camera[ProjectionOrthographic];
	const float2 worldPos = Floor(GetWorld2DCoord(engine, camera, mouse.pos));
	//LOG(Info, "World x: %f, y: %f\n", worldPos.x, worldPos.y);

	const EntityDesc entityDesc = {
		.name = InternString("entity"),
		.pos = Float3(worldPos, 0.0),
		.scale = 1.0f,
		.spriteId = spriteId,
	};

	return CreateEntity(engine, entityDesc);
}

static void EditorUpdateUI_DragAndDropLost()
{
	Engine &engine = GetEngine();
	UI &ui = engine.ui;

	if ( UI_DragAndDropTargetLost(ui, "Sprite") )
	{
		const ID spriteId = { UI_DragAndDropPayload(ui).uvalue };
		if ( spriteId ) {
			EditorSpawnEntityAtMouse(spriteId);
		}
	}

	if ( UI_DragAndDropTargetLost(ui, "Texture") )
	{
		const ID textureId = { UI_DragAndDropPayload(ui).uvalue };
		if ( textureId )
		{
			const Texture &texture = GetTexture(textureId);
			const SpriteDesc spriteDesc = {
				.name = EditorMakeSpriteName(engine.scene, texture.desc.name),
				.textureId = textureId,
				.pos = { 0, 0 },
				.size = texture.size,
			};
			const ID spriteId = GetOrCreateSprite(engine, spriteDesc);
			if ( spriteId ) {
				EditorSpawnEntityAtMouse(spriteId);
			}
		}
	}

	if ( UI_DragAndDropTargetLost(ui, "FileNode") )
	{
		FileNode *node = (FileNode*) UI_DragAndDropPayload(ui).ptr;
		if (node->type == FileNodeType_Image)
		{
			//LOG(Info, "Image asset dropped: %s\n", node->filename);

			const char *basename = NameFromFilename(node->filename);

			const TextureDesc textureDesc = {
				.name = MakeName("tex_%s", basename),
				.filename = node->filename,
				.mipmap = true,
			};
			const ID textureId = GetOrCreateTexture(engine.gfx, textureDesc);

			const SpriteDesc spriteDesc = {
				.name = MakeName("spr_%s", basename),
				.textureId = textureId,
			};
			const ID spriteId = GetOrCreateSprite(engine, spriteDesc);

			EditorSpawnEntityAtMouse(spriteId);
		}
		else if (node->type == FileNodeType_Sound)
		{
			//LOG(Info, "AudioClip asset dropped: %s\n", node->filename);

			const char *basename = NameFromFilename(node->filename);
			const char *clipname = MakeName("snd_%s", basename);

			const AudioClipDesc desc = {
				.name = clipname,
				.filename = node->filename,
				//.flags = AssetFlag_Ghost,
			};
			const ID clipId = GetOrCreateAudioClip(engine.audio, desc);
		}
		else if (node->type == FileNodeType_Music)
		{
			//LOG(Info, "MusicFile asset dropped: %s\n", node->filename);

			const char *basename = NameFromFilename(node->filename);
			const char *modname = MakeName("mod_%s", basename);

			const MusicFileDesc desc = {
				.name = modname,
				.filename = node->filename,
				//.flags = AssetFlag_Ghost,
			};
			const ID musicId = GetOrCreateMusicFile(engine.audio, desc);
		}
	}
}

static void EditorUpdateUI_ContextMenu()
{
	Engine &engine = GetEngine();
	UI &ui = engine.ui;

	const float2 pos = Float2(ui.input.lastMouseClickPos);
	UI_SetNextWindowDisplacement(ui, pos);
	if ( UI_BeginMenu(ui, "Context", &engine.editor.showContextMenu) )
	{
		UI_MenuItem(ui, "Option 1");
		UI_MenuItem(ui, "Option 2");
		UI_MenuItem(ui, "Option 3");
		UI_MenuItem(ui, "Option 4");
		UI_EndMenu(ui);
	}
}

static void EditorUpdateUI_Settings()
{
	Engine &engine = GetEngine();
	UI &ui = engine.ui;
	Editor &editor = engine.editor;

	constexpr uint2 size = {300, 400};
	UI_SetNextWindowDefaultSize(ui, size);
	UI_SetNextWindowAnchor(ui, {0.5, 0.5});

	UI_BeginWindow(ui, "Settings", &editor.showSettings);

	if ( UI_Section(ui, "General") )
	{
		UI_Checkbox(ui, "Hot reload", &engine.settings.hotReload);
	}
	if ( UI_Section(ui, "UI Colors") )
	{
		static float4 *colorToEdit = nullptr;

		UI_BeginLayout(ui, UILayout_Horizontal);
		UI_Label(ui, "Base    ");
		UI_Label(ui, "Hover");
		UI_EndLayout(ui);

		for ( u32 i = 0; i < UIElementCount; ++i )
		{
			float4 &base = ui.style.colors[i].base;
			float4 &hover = ui.style.colors[i].hovered;
			//const float4 active = ui.style.colors[i].active;
			//const float4 inactive = ui.style.colors[i].inactive;

			const float4 hbase = Lerp(base, UiColorWhite, 0.2);
			const float4 hhover = Lerp(hover, UiColorWhite, 0.2);

			UI_BeginLayout(ui, UILayout_Horizontal);

			UI_PushElemColor(ui, UIElementButton, {base, hbase});
			if ( UI_Button(ui, "        ") )
			{
				colorToEdit = &base;
			}
			UI_PopElemColor(ui, UIElementButton);

			UI_PushElemColor(ui, UIElementButton, {hover, hhover});
			if ( UI_Button(ui, "        ") )
			{
				colorToEdit = &hover;
			}
			UI_PopElemColor(ui, UIElementButton);

			UI_Label(ui, UIElementName[i]);
			UI_EndLayout(ui);
		}

		if ( colorToEdit )
		{
			bool isOpen = true;
			UI_ColorPicker(ui, colorToEdit, &isOpen);
			if ( !isOpen )
			{
				colorToEdit = nullptr;
			}
		}

		if ( UI_Button(ui, "Reset") ) {
			UI_ResetStyle(ui);
		}
	}

	UI_EndWindow(ui);
}

enum EditorFileDialogMode
{
	EditorFileDialog_LoadFile,
	EditorFileDialog_SaveFile,
	EditorFileDialogModeCount,
};
struct EditorFileDialogStrings
{
	const char *caption;
	const char *button;
};
static const EditorFileDialogStrings EditorFileDialogStringsArray[] = {
	{ .caption = "Load file", .button = "Load"},
	{ .caption = "Save file", .button = "Save"},
};
CT_ASSERT( ARRAY_COUNT(EditorFileDialogStringsArray) == EditorFileDialogModeCount );

static bool EditorFileDialog(EditorFileDialogMode mode, const char *extension, bool *isOpen, FilePath *filePath)
{
	Engine &engine = GetEngine();
	bool ret = false;

	UI &ui = engine.ui;
	Editor &editor = engine.editor;

	static bool wasOpen = false;
	static char filename[MAX_PATH_LENGTH] = {};

	const bool justOpened = !wasOpen;
	if ( justOpened ) {
		StrCopy(filename, "");
	}

	const char *caption = EditorFileDialogStringsArray[mode].caption;
	const char *button = EditorFileDialogStringsArray[mode].button;

	UI_SetNextWindowModal(ui);
	UI_SetNextWindowDefaultSize(ui, {400, 300});
	UI_SetNextWindowAnchor(ui, {0.5, 0.5});
	UI_BeginWindow(ui, caption, isOpen);

	Dir dir;
	if ( OpenDir(dir, AssetDir) )
	{
		DirEntry entry;
		while ( ReadDir(dir, entry) )
		{
			if ( HasFileExtension(entry.name, extension) )
			{
				if ( UI_Radio(ui, entry.name, StrEq(entry.name, filename)) )
				{
					StrCopy(filename, entry.name);
				}
			}
		}

		CloseDir(dir);
	}

	if ( mode == EditorFileDialog_SaveFile )
	{
		UI_InputText(ui, "File name", filename, ARRAY_COUNT(filename));
	}

	if ( UI_Button(ui, button) && !StrEq(filename, "") ) {
		*filePath = MakePath(AssetDir, filename);
		*isOpen = false;
		ret = true;
	}

	UI_EndWindow(ui);

	wasOpen = *isOpen;

	return ret;
}

static void EditorUpdateUI()
{
	Engine &engine = GetEngine();
	UI &ui = engine.ui;
	Graphics &gfx = engine.gfx;
	Scene &scene = engine.scene;
	Editor &editor = engine.editor;

	EditorUpdateUI_MenuBar();
	EditorUpdateUI_ToolBar();

	if ( editor.showLoadScene )
	{
		static FilePath filePath = {};
		if ( EditorFileDialog(EditorFileDialog_LoadFile, "txt", &editor.showLoadScene, &filePath) )
		{
			const EditorCommand command = { .type = EditorCommandLoadTxt, .filepath = filePath.str };
			AddEditorCommand(command);
		}
	}

	static FilePath saveSceneFilepath = {};
	static bool saveScene = false;
	if ( editor.showSaveScene )
	{
		if ( EditorFileDialog(EditorFileDialog_SaveFile, "txt", &editor.showSaveScene, &saveSceneFilepath) )
		{
			saveScene = true;
		}
	}
	if ( saveScene )
	{
		const char *buttons[] = { "Yes", "No", nullptr };
		i32 result = 0;
		if ( ExistsFile(saveSceneFilepath.str) )
		{
			if ( UI_MessageBox(ui, "Save scene", "File already exists. Overwrite contents?", buttons, &result) )
			{
				if ( result == 0 ) {
					EditorCommand command = { .type = EditorCommandSaveTxt, .filepath = saveSceneFilepath.str };
					AddEditorCommand(command);
				}
				saveScene = false;
			}
		}
		else
		{
			EditorCommand command = { .type = EditorCommandSaveTxt, .filepath = saveSceneFilepath.str };
			AddEditorCommand(command);

			saveScene = false;
		}
	}

	if ( editor.showOutliner )
	{
		EditorUpdateUI_Outliner();
	}

	if ( editor.showAssets )
	{
		EditorUpdateUI_Assets();
	}

	if ( editor.showInspector )
	{
		EditorUpdateUI_Inspector();
	}

	if ( editor.showSpriteSheet )
	{
		EditorUpdateUI_SpriteSheet();
	}

	#if USE_PROFILE
	if ( editor.showProfiler )
	{
		EditorUpdateUI_Profiler();
	}
	#endif // USE_PROFILE

	if ( editor.showDebugUI )
	{
		EditorUpdateUI_DebugUI();
	}

	if ( editor.showAbout )
	{
		EditorUpdateUI_About();
	}

	if ( editor.showContextMenu )
	{
		EditorUpdateUI_ContextMenu();
	}

	if ( editor.showSettings )
	{
		EditorUpdateUI_Settings();
	}

	if ( editor.showQuit )
	{
		i32 result = 0;
		const char *buttons[] = { "Yes", "No", nullptr };
		if ( UI_MessageBox(ui, "Quit", "Do you really want to quit?", buttons, &result) )
		{
			if ( result == 0 ) {
				PlatformQuit();
			}
			editor.showQuit = false;
		}
	}

	EditorUpdateUI_DragAndDropLost();
}

#if PLATFORM_ANDROID
static bool GetOrientationTouchId(const Window &window, u32 *touchId)
{
	ASSERT( touchId != 0 );
	for (u32 i = 0; i < ARRAY_COUNT(window.touches); ++i)
	{
		if (window.touches[i].state == TOUCH_STATE_PRESSED &&
			window.touches[i].x0 > window.width/2 )
		{
			*touchId = i;
			return true;
		}
	}
	return false;
}

static bool GetMovementTouchId(const Window &window, u32 *touchId)
{
	ASSERT( touchId != 0 );
	for (u32 i = 0; i < ARRAY_COUNT(window.touches); ++i)
	{
		if (window.touches[i].state == TOUCH_STATE_PRESSED &&
			window.touches[i].x0 <= window.width/2 )
		{
			*touchId = i;
			return true;
		}
	}
	return false;
}
#endif

static void EditorUpdateCamera3DOrbit(Camera &camera, f32 deltaSeconds)
{
	static f32 yaw = 0;
	yaw += 0.25 * Pi * deltaSeconds;

	const f32 pitch = -0.45f;
	const float2 angles = {yaw, pitch};
	const float3 forward = ForwardDirectionFromAngles(angles);
	const float3 position = -3.0f * forward;

	camera.position = position;
	camera.orientation = angles;
}

static void EditorUpdateCamera3D(const Window &window, Camera &camera, f32 deltaSeconds, bool handleInput)
{
	const Mouse &mouse = window.mouse;

	float3 dir = { 0, 0, 0 };

	if ( handleInput )
	{
		// Camera rotation
		f32 deltaYaw = 0.0f;
		f32 deltaPitch = 0.0f;
#if PLATFORM_ANDROID
		u32 touchId;
		if ( GetOrientationTouchId(window, &touchId) )
		{
			deltaYaw = - window.touches[touchId].dx * ToRadians * 0.2f;
			deltaPitch = - window.touches[touchId].dy * ToRadians * 0.2f;
		}
#else
		if (MouseButtonPressed(mouse, MOUSE_BUTTON_LEFT)) {
			deltaYaw = - mouse.delta.x * ToRadians * 0.2f;
			deltaPitch = - mouse.delta.y * ToRadians * 0.2f;
		}
#endif
		float2 angles = camera.orientation;
		angles.x = angles.x + deltaYaw;
		angles.y = Clamp(angles.y + deltaPitch, -Pi * 0.49, Pi * 0.49);

		camera.orientation = angles;

		// Movement direction
#if PLATFORM_ANDROID
		if ( GetMovementTouchId(window, &touchId) )
		{
			const float3 forward = ForwardDirectionFromAngles(angles);
			const float3 right = RightDirectionFromAngles(angles);
			const f32 scaleForward = -window.touches[touchId].dy;
			const f32 scaleRight = window.touches[touchId].dx;
			dir = Add(Mul(forward, scaleForward), Mul(right, scaleRight));
		}
#else
		if ( KeyPressed(window.keyboard, K_W) ) { dir = Add(dir, ForwardDirectionFromAngles(angles)); }
		if ( KeyPressed(window.keyboard, K_S) ) { dir = Add(dir, Negate( ForwardDirectionFromAngles(angles) )); }
		if ( KeyPressed(window.keyboard, K_D) ) { dir = Add(dir, RightDirectionFromAngles(angles)); }
		if ( KeyPressed(window.keyboard, K_A) ) { dir = Add(dir, Negate( RightDirectionFromAngles(angles) )); }
#endif
		dir = NormalizeIfNotZero(dir);
	}

	// Accelerated translation
	static constexpr f32 MAX_SPEED = 100.0f;
	static constexpr f32 ACCELERATION = 50.0f;
	static float3 speed = { 0, 0, 0 };
	const float3 speed0 = speed;

	// Apply acceleration, then limit speed
	speed = Add(speed, Mul(dir, ACCELERATION * deltaSeconds));
	speed = Length(speed) > MAX_SPEED ?  Mul( Normalize(speed), MAX_SPEED) : speed;

	// Based on speed, translate camera position
	const float3 translation = Add( Mul(speed0, deltaSeconds), Mul(speed, 0.5f * deltaSeconds) );
	camera.position = Add(camera.position, translation);

	// Apply deceleration
	speed = Mul(speed, 0.9);
}

static void EditorUpdateInteraction2D(const Window &window, const Gamepad &gamepad, Camera &camera, f32 deltaSeconds, bool handleInput)
{
	Engine &engine = GetEngine();
	Editor &editor = engine.editor;
	UI &ui = engine.ui;
	const Mouse &mouse = window.mouse;

	const ID selectedEntity = EditorGetSelectedEntity(editor);

	// Object transformations
	if ( handleInput && selectedEntity )
	{
		Entity &entity = GetEntity(selectedEntity);

		const float2 mouseWorldPos = GetWorld2DCoord(engine, camera, mouse.pos);

		static float2 initialWorldOffset = {};
		static float2 initialWorldPos = {};

		static bool wasTranslating = false;

		if ( !editor.isTranslating && KeyPress(window.keyboard, K_T) )
		{
			editor.isTranslating = true;
		}

		if ( editor.isTranslating )
		{
			if (!wasTranslating) {
				initialWorldPos = float2{entity.position.x, entity.position.y};
				initialWorldOffset = Floor(initialWorldPos) - Floor(mouseWorldPos);
			} else if (MouseButtonPress(mouse, MOUSE_BUTTON_RIGHT) || KeyPress(window.keyboard, K_ESCAPE)) {
				EntitySetPosition(entity, Float3(initialWorldPos, entity.position.z));
				editor.isTranslating = false;
			} else if (MouseButtonRelease(mouse, MOUSE_BUTTON_LEFT) || MouseButtonPress(mouse, MOUSE_BUTTON_LEFT)) {
				editor.isTranslating = false;
			} else {
				const float2 finalPos = Floor(mouseWorldPos) + initialWorldOffset;
				EntitySetPosition(entity, Float3(finalPos, entity.position.z));
			}
		}

		wasTranslating = editor.isTranslating;

		static bool isScaling = false;
		static f32 initialScale = 0.0;
		if (KeyPress(window.keyboard, K_E)) {
			isScaling = true;
			initialWorldOffset = float2{entity.position.x, entity.position.y} - mouseWorldPos;
			initialScale = entity.scale;
		} else if (isScaling && MouseButtonPress(mouse, MOUSE_BUTTON_RIGHT)) {
			entity.scale = initialScale;
			isScaling = false;
		} else if (isScaling && MouseButtonPress(mouse, MOUSE_BUTTON_LEFT)) {
			isScaling = false;
		} else if (isScaling) {
			const float2 worldOffset = float2{entity.position.x, entity.position.y} - mouseWorldPos;
			const f32 initialOffsetLen = Length(initialWorldOffset);
			const f32 offsetLen = Length(worldOffset);
			entity.scale = initialScale * offsetLen / initialOffsetLen;
		}
	}

	// Camera navigation
	if ( handleInput )
	{
		static int2 clickPos = {};
		static float3 cameraPositionOnClick = camera.position;
		if (MouseButtonPress(mouse, MOUSE_BUTTON_MIDDLE))
		{
			clickPos = mouse.pos;
			cameraPositionOnClick = camera.position;
		}
		if (MouseButtonPressed(mouse, MOUSE_BUTTON_MIDDLE))
		{
			const f32 windowHeight = window.height;
			const int2 dragPixels = mouse.pos - clickPos;
			const float2 dragNorm = {dragPixels.x/windowHeight, dragPixels.y/windowHeight};
			const float2 dragScaled = 2.0f * camera.height * dragNorm;
			camera.position.x = cameraPositionOnClick.x - dragScaled.x;
			camera.position.y = cameraPositionOnClick.y + dragScaled.y;
		}
	}

	if ( ui.hoveredWindow == nullptr )
	{
		if (mouse.wheel.y != 0.0)
		{
			const f32 ar = (f32) window.width / window.height;
			const f32 heightPrev = camera.height;
			camera.height = Max(2.0f, camera.height + mouse.wheel.y);
			const f32 heightDiff = heightPrev - camera.height;
			const f32 widthDiff = ar * heightDiff;
			const f32 xScale = 2.0f * ( (f32) mouse.pos.x / window.width ) - 1.0f;
			const f32 yScale = 1.0 - 2.0f * ( (f32) mouse.pos.y / window.height );
			camera.position.x += widthDiff * xScale;
			camera.position.y += heightDiff * yScale;
		}
	}

	float3 dir = { 0, 0, 0 };

	// Interaction with keyboard / gamepad
	if ( handleInput )
	{
		f32 dx = 0.0f;
		f32 dy = 0.0f;

		// Keyboard
		dy += KeyPressed(window.keyboard, K_W) ? 1.0f : 0.0f;
		dx += KeyPressed(window.keyboard, K_A) ? -1.0f : 0.0f;
		dy += KeyPressed(window.keyboard, K_S) ? -1.0f : 0.0f;
		dx += KeyPressed(window.keyboard, K_D) ? 1.0f : 0.0f;

		// Gamepad
		dx += gamepad.leftAxis.x;
		dy += gamepad.leftAxis.y;

		const float3 upVector = {0, 1, 0};
		const float3 rightVector = {1, 0, 0};
		dir = NormalizeIfNotZero(dx * rightVector + dy * upVector);
	}

	// Accelerated translation
	static constexpr f32 MAX_SPEED = 25.0f;
	static constexpr f32 ACCELERATION = 50.0f;
	static float3 speed = { 0, 0, 0 };
	const float3 speed0 = speed;

	// Apply acceleration, then limit speed
	speed = Add(speed, Mul(dir, ACCELERATION * deltaSeconds));
	speed = Length(speed) > MAX_SPEED ?  Mul( Normalize(speed), MAX_SPEED) : speed;

	// Based on speed, translate camera position
	const float3 translation = Add( Mul(speed0, deltaSeconds), Mul(speed, 0.5f * deltaSeconds) );
	camera.position = Add(camera.position, translation);

	// Apply deceleration
	if ( Length2(dir) == 0.0f )
	{
		speed = Mul(speed, 0.9);
	}
}

static void EditorBeginSceneEditing(bool handleInput)
{
	const Mouse &mouse = GetWindow().mouse;
	Engine &engine = GetEngine();
	Editor &editor = GetEditor();
	Scene &scene = engine.scene;

	if ( handleInput )
	{
		// While the Tilesets panel is open, left click paints/erases tiles instead of selecting entities
		const bool tileEditMode = EditorMode2D() && EditorGetContextLayer() != nullptr;

		if (!tileEditMode && MouseButtonPress(mouse, MOUSE_BUTTON_LEFT) && !editor.isTranslating)
		{
			editor.selectEntity = true;
		}
		else if ( editor.selectEntity )
		{
			editor.selectEntity = false;

			WaitDeviceIdle(engine.gfx.device);
			// What the id pass wrote is a draw id, not an ID: its low half is the entity's
			// ID slot, the high half its position in the array at the time it was drawn
			const u32 drawId = *(u32*)GetBufferPtr(engine.gfx.device, engine.gfx.selectionBufferH);
			const ID entityId = EntityFromDrawId(drawId);

			if (entityId)
			{
				EditorSelectEntity(entityId);
			}
			else
			{
				EditorUnselectAll();
			}
		}

		if (tileEditMode && MouseButtonPressed(mouse, MOUSE_BUTTON_LEFT))
		{
			Layer *contextLayer = EditorGetContextLayer();
			if (contextLayer)
			{
				const Camera &camera = editor.camera[ProjectionOrthographic];
				const int2 gridCoord = GetGridTileCoord(engine, camera, mouse.pos) - EditorGetContextRoom()->pos;

				Layer &layer = *contextLayer;

				if ( layer.isCollider )
				{
					const u32 collider =
						editor.context.tool == EditorTool_ColliderSolid ? 1
						: editor.context.tool == EditorTool_ColliderPlatform ? 2
						: 0;
					SetGridTileAtCoord(engine, layer, collider, gridCoord);
				}
				else
				{
					const ID spriteId = editor.context.tool == EditorTool_Draw
						? editor.context.spriteId : ID{};
					SetGridTileAtCoord(engine, layer, spriteId, gridCoord);
				}
			}
		}
	}
}

void EditorDebugDraw()
{
	Engine &engine = GetEngine();
	Editor &editor = engine.editor;

	Room *contextRoom = EditorGetContextRoom();
	Layer *contextLayer = EditorGetContextLayer();

	if (contextLayer != nullptr)
	{
		Room &room = *contextRoom;
		Layer &layer = *contextLayer;
		const float2 pos = Float2(room.pos);
		const float2 size = LayerSize(layer);
		DrawBoxOutline(pos, size, ColorOrange);

		if (EditorMode2D())
		{
			const Mouse &mouse = GetWindow().mouse;
			const float2 worldPos = Floor(GetWorld2DCoord(engine, editor.camera[ProjectionOrthographic], mouse.pos));

			if ( layer.isCollider )
			{
				const float4 color = {1.0, 0.0, 0.0, 0.3};

				if ( !UI_IsHovered(engine.ui) )
				{
					DrawBox(worldPos, float2{1, 1}, color);
				}

				for (u32 y = 0; y < layer.size.y; ++y)
				{
					for (u32 x = 0; x < layer.size.x; ++x)
					{
						if ( layer.cells[x][y].collider != 0 )
						{
							const float2 cellWorldPos = {(f32)x, (f32)y};
							DrawBox(cellWorldPos, float2{1, 1}, color);
						}
					}
				}
			}
			else
			{
				const ID spriteId = editor.context.spriteId;
				if ( spriteId )
				{
					const float4 color = {1, 1, 1, 0.3};
					DrawSprite(spriteId, worldPos, color);
				}
			}
		}
	}
	else if (contextRoom != nullptr)
	{
	}
}

static void EditorProcessCommands(Arena scratch)
{
	Engine &engine = GetEngine();
	Editor &editor = engine.editor;
	Graphics &gfx = engine.gfx;

	if ( editor.commandCount > 0 )
	{
		WaitDeviceIdle(gfx.device);

		for (u32 i = 0; i < editor.commandCount; ++i)
		{
			const EditorCommand &command = editor.commands[i];

			switch (command.type)
			{
				case EditorCommandRemoveTexture:
				{
					RemoveTexture(engine.gfx, command.textureId);
					break;
				}
				case EditorCommandNew:
				{
					EditorUnselectAll();
					CleanScene(engine);
					CreateScene(engine);
					break;
				}
				case EditorCommandLoadTxt:
				{
					EditorUnselectAll();
					CleanScene(engine);
					LoadSceneFromTxt(engine, command.filepath);
					break;
				}
				case EditorCommandSaveTxt:
				{
					SaveSceneToTxt(engine, command.filepath);
					break;
				}
				case EditorCommandLoadBin:
				{
					EditorUnselectAll();
					CleanScene(engine);
					LoadSceneFromBin(engine);
					break;
				}
				case EditorCommandBuildBin:
				{
					const FilePath assetsFilepath = MakePath(DataDir, "assets.dat");
					const FilePath descriptorsFilepath = MakePath(AssetDir, "assets.txt");
					BuildAssetsFromTxt(engine, descriptorsFilepath.str, assetsFilepath.str);
					break;
				}

				default:;
			}
		}

		editor.commandCount = 0;
	}
}

static FileNode *GetFreeFileNode()
{
	Editor &editor = GetEditor();
	ASSERT(editor.freeNodes != nullptr);
	FileNode *res = editor.freeNodes;
	editor.freeNodes = res->next;
	if ( editor.freeNodes) { editor.freeNodes->prev = nullptr; }
	res->type = FileNodeType_COUNT,
	res->filename = nullptr;
	res->next = nullptr;
	res->prev = nullptr;
	res->child = nullptr;
	return res;
}

static void FreeFileNode(FileNode *node)
{
	Editor &editor = GetEditor();
	FileNode *first = editor.freeNodes;
	if (first) { first->prev = node; }
	node->prev = nullptr;
	node->next = first;
	editor.freeNodes = node;
}

static FileNode * InsertFileNode(FileNode *node, FileNode *first)
{
	if ( first ) {
		first->prev = node;
	}
	node->next = first;
	node->prev = nullptr;
	return node;
}

void EditorInitialize(Engine &engine)
{
	Editor &editor = engine.editor;

	editor.showOutliner = true;
	editor.showAssets = true;
	editor.showInspector = true;
	editor.showSpriteSheet = false;
	editor.showProfiler = false;
	editor.showDebugUI = false;
	editor.showGrid = true;
	editor.showAbout = false;
	editor.showSettings = false;
	editor.showQuit = false;

	// Zero is a valid layer index, so "nothing selected" has to be set explicitly
	editor.context.roomId = {};
	editor.context.layerIndex = EditorNoLayer;

	// projectionType is derived from the array index so it can never drift out of sync with it
	for (u32 i = 0; i < ProjectionTypeCount; ++i)
	{
		editor.camera[i].projectionType = (ProjectionType)i;
	}

	editor.camera[ProjectionPerspective].position = {0, 1, 2};
	editor.camera[ProjectionPerspective].orientation = {0, -0.45f};
	editor.camera[ProjectionPerspective].fovy = 60.0f;
	editor.camera[ProjectionPerspective].znear = 0.1f;
	editor.camera[ProjectionPerspective].zfar = 1000.0f;

	editor.camera[ProjectionOrthographic].position = {0, 0, -5};
	editor.camera[ProjectionOrthographic].orientation = {};
	editor.camera[ProjectionOrthographic].height = 8.0;
	editor.camera[ProjectionOrthographic].znear = -10.0f;
	editor.camera[ProjectionOrthographic].zfar = 10.0f;

	editor.cameraType = ProjectionOrthographic;
	SetCamera(editor.camera[ProjectionOrthographic]);

	editor.spriteSheet.textureId = {};
	editor.inspector.audioSourceIndex = U32_MAX;

	editor.iconAsset = EditorLoadIcon("editor/file_32x32.png", "file_32x32");
	editor.iconWav = EditorLoadIcon("editor/wav_32x32.png", "wav_32x32");
	editor.iconMod = EditorLoadIcon("editor/mod_32x32.png", "mod_32x32");
	editor.iconImg = EditorLoadIcon("editor/img_32x32.png", "img_32x32");
	editor.iluLogo = EditorLoadIcon("editor/ilu_logo.png", "ilu_logo");

	EditorUnselectAll();

	// Make a liked list of free file nodes
	constexpr u32 maxFileNodes = 4092;
	editor.freeNodes = PushZeroArray(GlobalArena, FileNode, maxFileNodes);
	for (u32 i = 0; i < maxFileNodes; ++i) {
		if ( i < maxFileNodes - 1) {
			editor.freeNodes[i].next = &editor.freeNodes[i+1];
		}
		if ( i > 0 ) {
			editor.freeNodes[i].prev = &editor.freeNodes[i-1];
		}
	}
	editor.root = nullptr;

	// Read the asset directory contents into file nodes
	Dir dir;
	if ( OpenDir(dir, AssetDir) )
	{
		DirEntry entry;
		while ( ReadDir(dir, entry) )
		{
			FileNode *node = GetFreeFileNode();
			node->filename = InternString(entry.name);
			node->type = IsImgFile(entry.name) ? FileNodeType_Image :
						IsMusicFile(entry.name) ? FileNodeType_Music :
						IsWavFile(entry.name) ? FileNodeType_Sound:
						FileNodeType_COUNT;
			editor.root = InsertFileNode(node, editor.root);
		}

		CloseDir(dir);
	}

	CleanScene(engine);
}

static bool EditorHandleKeyboardShortcuts()
{
	Engine &engine = GetEngine();
	Editor &editor = engine.editor;
	const Window &window = GetWindow();

	bool handleInput = true;

	// Delete takes any selection, the rest of the shortcuts only act on an entity
	if (KeyPress(window.keyboard, K_DELETE))
	{
		EditorRemoveSelection();
	}

	const ID selectedEntity = EditorGetSelectedEntity(editor);
	if ( !selectedEntity )
	{
		return handleInput;
	}

	if (KeyPressed(window.keyboard, K_SHIFT))
	{
		handleInput = false; // K_SHIFT is for commands, so abort camera translation

		if (KeyPress(window.keyboard, K_D))
		{
			const ID newEntity = DuplicateEntity(engine, selectedEntity);
			EditorSelectEntity(newEntity);
			editor.isTranslating = true;
		}
	}

	return handleInput;
}

void EditorUpdate(Engine &engine)
{
	const Plat &platform = GetPlatform();
	const Gamepad &gamepad = *platform.gamepad;
	const Window &window = GetWindow();
	Graphics &gfx = engine.gfx;

	if ( KeyPress(window.keyboard, K_F5) )
	{
		if ( engine.game.state == GameStateStopped ) {
			engine.game.state = GameStateStarting;
		}
	}
	else if ( KeyPress(window.keyboard, K_ESCAPE) )
	{
		if ( engine.game.state == GameStateRunning ) {
			engine.game.state = GameStateStopping;
			EditorSetCamera();
		}
	}

	if ( engine.game.state != GameStateStopped )
	{
		return;
	}

	EditorUpdateInspectedAsset();

	EditorUpdateUI();

	bool handleInput = !engine.ui.wantsInput && engine.game.state == GameStateStopped;

	EditorHandleKeyboardShortcuts();

	if ( handleInput )
	{
		if ( UI_IsMousePress(engine.ui, MOUSE_BUTTON_RIGHT) )
		{
			engine.editor.showContextMenu = true;
		}
	}

	if (EditorMode3D())
	{
		if (engine.editor.cameraOrbit)
		{
			EditorUpdateCamera3DOrbit(engine.editor.camera[ProjectionPerspective], gfx.deltaSeconds);
		}
		else
		{
			EditorUpdateCamera3D(window, engine.editor.camera[ProjectionPerspective], gfx.deltaSeconds, handleInput);
		}

		SetCamera(engine.editor.camera[ProjectionPerspective]);
	}
	else
	{
		EditorUpdateInteraction2D(window, gamepad, engine.editor.camera[ProjectionOrthographic], gfx.deltaSeconds, handleInput);
		SetCamera(engine.editor.camera[ProjectionOrthographic]);
	}

	EditorBeginSceneEditing(handleInput);

	EditorDebugDraw();
}

void EditorRender(Engine &engine, CommandList &commandList)
{
	Editor &editor = engine.editor;
	Graphics &gfx = engine.gfx;
	Scene &scene = engine.scene;
	const u32 frameIndex = gfx.device.frameIndex;
	const BufferH vertexBuffer = gfx.globalVertexArena.buffer;
	const BufferH indexBuffer = gfx.globalIndexArena.buffer;

	if ( editor.selectEntity )
	{
		BeginDebugGroup(commandList, "Entity selection", ColorBlack);

		{ // Draw entity IDs
			// Draw id 0 is the background: no live entity ever has ID slot 0
			SetClearColorU32(commandList, 0, 0);

			BeginRenderPass(commandList, gfx.renderTargets.idFramebuffer );

			const uint2 framebufferSize = GetFramebufferSize(gfx.renderTargets.idFramebuffer);
			SetViewportAndScissor(commandList, framebufferSize);

			SetPipeline(commandList, gfx.pipelines[Pipeline_ModelId]);
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);

			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);

			for (u32 i = 0; i < scene.entityCount; ++i)
			{
				const Entity &entity = scene.entities[i];

				if ( !entity.visible || entity.culled ) continue;
				if ( !entity.materialId ) continue;

				// Draw!!!
				const uint32_t indexCount = entity.indices.size/2; // div 2 (2 bytes per index)
				const uint32_t firstIndex = entity.indices.offset/2; // div 2 (2 bytes per index)
				const int32_t firstVertex = entity.vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same
				DrawIndexed(commandList, indexCount, firstIndex, firstVertex, EntityDrawId(scene, scene.entities[i].id), 1);
			}

			{ // Sprite entities
				const uint32_t spriteIndexCount = gfx.spriteIndices.size / sizeof(Index);
				const uint32_t spriteFirstIndex = gfx.spriteIndices.offset / sizeof(Index);
				const int32_t spriteFirstVertex = gfx.spriteVertices.offset / sizeof(Vertex);

				SetPipeline(commandList, gfx.pipelines[Pipeline_SpriteId]);

				for (u32 i = 0; i < scene.entityCount; ++i)
				{
					const Entity &entity = scene.entities[i];

					if ( !entity.visible || entity.culled ) continue;
					if ( !entity.spriteId ) continue;

					DrawIndexed(commandList, spriteIndexCount, spriteFirstIndex, spriteFirstVertex, EntityDrawId(scene, scene.entities[i].id), 1);
				}
			}

			EndRenderPass(commandList);
		}

		{ // Write entity ID under mouse cursor into selection buffer
			const Pipeline &pipeline = GetPipeline(gfx.device, gfx.pipelines[Pipeline_ComputeSelect]);

			SetPipeline(commandList, gfx.pipelines[Pipeline_ComputeSelect]);

			const BindGroupDesc bindGroupDesc = {
				.layout = pipeline.layout.bindGroupLayouts[3],
				.bindings = {
					{ .index = 0, .bufferView = gfx.selectionBufferViewH },
					{ .index = 1, .image = gfx.renderTargets.idImage },
				},
			};
			const BindGroup dynamicBindGroup = CreateFullBindGroup(gfx.device, bindGroupDesc, gfx.dynamicBindGroupAllocator[frameIndex]);

			SetBindGroup(commandList, 0, dynamicBindGroup);
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
			SetBindGroup(commandList, 3, dynamicBindGroup);

			TransitionImageLayout(commandList, gfx.renderTargets.idImage, ImageStateRenderTarget, ImageStateShaderInput, 0, 1);

			Dispatch(commandList, 1, 1, 1);

			TransitionImageLayout(commandList, gfx.renderTargets.idImage, ImageStateShaderInput, ImageStateRenderTarget, 0, 1);
		}

		EndDebugGroup(commandList);
	}
}

void EditorPostRender(Engine &engine)
{
	Scratch scratch;
	EditorProcessCommands(scratch.arena);
}

#if 0
		const float4 white = {1.0f, 1.0f, 1.0f, 1.0f};
		const float4 hueColor = {hueRgb.r, hueRgb.g, hueRgb.b, 1.0f};
		const float4 transparentBlack = {0.0f, 0.0f, 0.0f, 0.0f};
		const float4 opaqueBlack = {0.0f, 0.0f, 0.0f, 1.0f};
		UI_AddGradientQuad(ui, svPos, svSize, white, hueColor, white, hueColor);
		UI_AddGradientQuad(ui, svPos, svSize, transparentBlack, transparentBlack, opaqueBlack, opaqueBlack);
#endif
