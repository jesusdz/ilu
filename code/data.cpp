
static u32 U32FromChars(char a, char b, char c, char d)
{
	const u32 res =
		(u32)a << 0 |
		(u32)b << 8 |
		(u32)c << 16 |
		(u32)d << 24 ;
	return res;
}

#if USE_DATA_BUILD

////////////////////////////////////////////////////////////////////////
// Shader compilation

static FilePath GetShaderHlslFilePath(u32 index)
{
	FilePath filepath;
	const ShaderSourceDesc &desc = shaderSourceDescs[index];
	SPrintf(filepath.str, "%s/code/shaders/%s", ProjectDir, desc.filename);
	return filepath;
}

static FilePath GetShaderSpirvFilePath(u32 index)
{
	FilePath filepath;
	const ShaderSourceDesc &desc = shaderSourceDescs[index];
	SPrintf(filepath.str, "%s/shaders/%s.spv", DataDir, desc.name);
	return filepath;
}

static FilePath GetShaderDisasmFilePath(u32 index)
{
	FilePath filepath;
	const ShaderSourceDesc &desc = shaderSourceDescs[index];
	SPrintf(filepath.str, "%s/shaders/%s.dis", DataDir, desc.name);
	return filepath;
}

void CompileShader(u32 index)
{
	ASSERT(index < ARRAY_COUNT(shaderSourceDescs));
	const ShaderSourceDesc &desc = shaderSourceDescs[index];

#if PLATFORM_WINDOWS
	constexpr const char *dxc = "dxc/windows/bin/x64/dxc.exe";
#elif PLATFORM_LINUX
	constexpr const char *dxc = "dxc/linux/bin/dxc";
#else
	constexpr const char *dxc = "<none>";
#endif
	constexpr const char *flags = "-spirv -O3";

	const char *target =
		desc.type == ShaderTypeVertex ? "vs_6_7" :
		desc.type == ShaderTypeFragment ? "ps_6_7" :
		desc.type == ShaderTypeCompute ? "cs_6_7" :
		"unknown";
	const char *entry = desc.entryPoint;
	const char *defines = desc.defines ? desc.defines : "";

	const FilePath filepathSpirv = GetShaderSpirvFilePath(index);
	const FilePath filepathDisasm = GetShaderDisasmFilePath(index);
	const FilePath filepathHlsl = GetShaderHlslFilePath(index);

	char commandline[MAX_PATH_LENGTH];
	SPrintf(commandline,
			"%s/%s "
			"%s -T %s -E %s "
			"%s "
			"-Fo %s -Fc %s "
			"%s",
			ProjectDir, dxc,
			flags, target, entry,
			defines,
			filepathSpirv.str, filepathDisasm.str,
			filepathHlsl.str);
	LOG(Info, "%s\n", commandline);
	ExecuteProcess(commandline);
}

void CompileShaders()
{
	CreateDirectory( MakePath(ProjectDir, "build").str );
	CreateDirectory( MakePath(ProjectDir, "build/shaders").str );

	for (u32 i = 0; i < ARRAY_COUNT(shaderSourceDescs); ++i)
	{
		CompileShader(i);
	}
}

bool CompileModifiedShaders()
{
	bool recompiled = false;

	for (u32 i = 0; i < ARRAY_COUNT(shaderSourceDescs); ++i)
	{
		const FilePath filepathHlsl = GetShaderHlslFilePath(i);
		const FilePath filepathSpirv = GetShaderSpirvFilePath(i);

		u64 timestampHlsl = 0;
		u64 timestampSpirv = 0;
		const bool success1 = GetFileLastWriteTimestamp(filepathHlsl.str, timestampHlsl);
		const bool success2 = GetFileLastWriteTimestamp(filepathSpirv.str, timestampSpirv);
		const bool someError = !success1 || !success2;

		// In case of error recovering the timestamp, we compile just in case
		if (someError || timestampHlsl > timestampSpirv)
		{
			CompileShader(i);
			recompiled = true;
		}
	}

	return recompiled;
}










////////////////////////////////////////////////////////////////////////
// Text output

struct WriteContext
{
	Arena arena;
	char line[MAX_PATH_LENGTH];
	u32 indent;
};

static void WriteIndentation(WriteContext &ctx)
{
	for (u32 i = 0; i < ctx.indent; ++i) {
		PushChar(ctx.arena, ' ');
	}
}

static void NewLine(WriteContext &ctx)
{
	PushChar(ctx.arena, '\n');
}

static void WriteSectionLine(WriteContext &ctx, const char *name)
{
	constexpr u32 maxLineLength = 72;
	u32 lineLength = 0;

	NewLine(ctx);

	WriteIndentation(ctx);
	lineLength += ctx.indent;

	PushChar(ctx.arena, '/'); lineLength++;
	PushChar(ctx.arena, '/'); lineLength++;
	PushChar(ctx.arena, ' '); lineLength++;

	while (*name) {
		PushChar(ctx.arena, *name++);
		lineLength++;
	}

	PushChar(ctx.arena, ' '); lineLength++;

	while (lineLength < maxLineLength) {
		PushChar(ctx.arena, '/'); lineLength++;
	}

	NewLine(ctx);
	NewLine(ctx);
}

static void WriteLine(WriteContext &ctx, const char *format, ...)
{
	va_list vaList;
	va_start(vaList, format);

	// Push indentation
	WriteIndentation(ctx);

	// Push text
	const i32 len = VSPrintf(ctx.line, format, vaList);
	char *chars = (char*)PushSize(ctx.arena, len);
	MemCopy(chars, ctx.line, len);
	
	// Push new line
	NewLine(ctx);

	va_end(vaList);
}

// Neither indents nor ends the line, for pieces of one
static void WriteText(WriteContext &ctx, const char *format, ...)
{
	va_list vaList;
	va_start(vaList, format);

	const i32 len = VSPrintf(ctx.line, format, vaList);
	char *chars = (char*)PushSize(ctx.arena, len);
	MemCopy(chars, ctx.line, len);

	va_end(vaList);
}

static void PushIndent(WriteContext &ctx)
{
	ctx.indent++;
}

static void PopIndent(WriteContext &ctx)
{
	ASSERT(ctx.indent > 0);
	ctx.indent--;
}

static void WriteProperty(WriteContext &ctx, const ReflexMember &member, const void *field)
{
	WriteIndentation(ctx);
	WriteText(ctx, ".%s = ", member.name);
	member.ops->write(ctx, member, field);
	WriteText(ctx, ",");
	NewLine(ctx);
}

static void WriteProperties(WriteContext &ctx, const ReflexStruct &type, const void *base)
{
	for (u32 i = 0; i < type.memberCount; ++i)
	{
		const ReflexMember &member = type.members[i];
		WriteProperty(ctx, member, (const byte*)base + member.offset);
	}
}

static void WriteScriptComponentDesc(WriteContext &ctx, const ScriptComponentDesc &script)
{
	WriteLine(ctx, ".script = {");
	PushIndent(ctx);

	WriteLine(ctx, ".name = \"%s\",", script.name);

	const ReflexStruct *type = ReflexGetStructFromName(script.name);
	if ( script.propertyCount > 0 && !type ) {
		LOG(Warning, "Script <%s> is not reflected, its properties are not saved.\n", script.name);
	}

	if ( script.propertyCount > 0 && type )
	{
		WriteLine(ctx, ".properties = {");
		PushIndent(ctx);
		for (u32 p = 0; p < script.propertyCount; ++p)
		{
			const ScriptPropertyDesc &property = script.properties[p];
			const ReflexMember *member = FindProperty(*type, property.name);
			if ( member && member->reflexId == property.type ) {
				WriteProperty(ctx, *member, property.value);
			} else {
				LOG(Warning, "Script <%s> has no property <%s> of its type anymore, it is not saved.\n", script.name, property.name);
			}
		}
		PopIndent(ctx);
		WriteLine(ctx, "},");
	}

	PopIndent(ctx);
	WriteLine(ctx, "},");
}

static void WriteComponentDesc(WriteContext &ctx, const ComponentDesc &component)
{
	if ( component.type == ComponentType_Script ) {
		WriteScriptComponentDesc(ctx, component.script);
		return;
	}

	const ReflexStruct *type = ComponentDescReflexStruct(component.type);
	if ( !type ) {
		LOG(Warning, "Component <%s> has no reflected descriptor, it is not saved.\n", ComponentNames[component.type]);
		return;
	}

	WriteLine(ctx, ".%s = {", ComponentFieldNames[component.type]);
	PushIndent(ctx);
	WriteProperties(ctx, *type, ComponentDescData(component));
	PopIndent(ctx);
	WriteLine(ctx, "},");
}

static void WriteEntityDescBody(WriteContext &ctx, const EntityDesc &desc, u32 entityIndex,
		const ComponentDesc *components, u32 componentCount)
{
	WriteLine(ctx, ".pos = {%f, %f, %f},", desc.pos.x, desc.pos.y, desc.pos.z);
	WriteLine(ctx, ".scale = %f,", desc.scale);

	for (u32 i = 0; i < componentCount; ++i)
	{
		if (components[i].entityIndex == entityIndex) {
			WriteComponentDesc(ctx, components[i]);
		}
	}
}

void SaveAssetDescriptors(const char *path, const AssetDescriptors &assets)
{
	Scratch scratch(MB(16)); // room tile lists can make the text large
	WriteContext ctx = {
		.arena = scratch.arena,
	};

	LOG(Info, "Saving data to text file: %s\n", path);

	char line[MAX_PATH_LENGTH];

	WriteSectionLine(ctx, "Scene");

	const SceneDesc &desc = assets.sceneDesc;

	WriteLine(ctx, "Scene scene = {");

	PushIndent(ctx);
	WriteLine(ctx, ".projectionType = \"%s\",", ProjectionTypeToStr(desc.projectionType));
	WriteLine(ctx, ".ambientLight = {%f, %f, %f},", desc.ambientLight.x, desc.ambientLight.y, desc.ambientLight.z);
	PopIndent(ctx);

	WriteLine(ctx, "};");
	NewLine(ctx);

	WriteSectionLine(ctx, "Textures");

	for (u32 i = 0; i < assets.textureDescCount; ++i)
	{
		const TextureDesc &desc = assets.textureDescs[i];

		WriteLine(ctx, "Texture %s = {", desc.name);

		PushIndent(ctx);
		WriteLine(ctx, ".id = %u,", desc.id.slot);
		WriteLine(ctx, ".filename = \"%s\",", desc.filename);
		WriteLine(ctx, ".mipmap = %d,", desc.mipmap);
		PopIndent(ctx);

		WriteLine(ctx, "};");
		NewLine(ctx);
	}

	WriteSectionLine(ctx, "Sprites");

	for (u32 i = 0; i < assets.spriteDescCount; ++i)
	{
		const SpriteDesc &desc = assets.spriteDescs[i];

		WriteLine(ctx, "Sprite %s = {", desc.name);

		PushIndent(ctx);
		WriteLine(ctx, ".id = %u,", desc.id.slot);
		WriteLine(ctx, ".textureId = %u,", desc.textureId.slot);
		if (desc.pos.x || desc.pos.y)
			WriteLine(ctx, ".pos = {%u, %u},", desc.pos.x, desc.pos.y);
		if (desc.size.x || desc.size.y)
			WriteLine(ctx, ".size = {%u, %u},", desc.size.x, desc.size.y);
		if (desc.frameCount > 1)
		{
			WriteLine(ctx, ".frameCount = %u,", desc.frameCount);
			WriteLine(ctx, ".fps = %u,", desc.fps);
			WriteLine(ctx, ".loop = %d,", desc.loop);
		}
		PopIndent(ctx);

		WriteLine(ctx, "};");
		NewLine(ctx);
	}

	WriteSectionLine(ctx, "Materials");

	for (u32 i = 0; i < assets.materialDescCount; ++i)
	{
		const MaterialDesc &desc = assets.materialDescs[i];

		WriteLine(ctx, "Material %s = {", desc.name);

		PushIndent(ctx);
		WriteLine(ctx, ".id = %u,", desc.id.slot);
		WriteLine(ctx, ".textureId = %u,", desc.textureId.slot);
		WriteLine(ctx, ".pipelineName = \"%s\",", desc.pipelineName);
		WriteLine(ctx, ".uvScale = %f,", desc.uvScale);
		PopIndent(ctx);

		WriteLine(ctx, "};");
		NewLine(ctx);
	}

	WriteSectionLine(ctx, "Entities");

	for (u32 i = 0; i < assets.entityDescCount; ++i)
	{
		const EntityDesc &desc = assets.entityDescs[i];

		WriteLine(ctx, "Entity %s = {", desc.name);

		PushIndent(ctx);
		WriteLine(ctx, ".id = %u,", desc.id.slot);
		WriteEntityDescBody(ctx, desc, i, assets.componentPool.components, assets.componentPool.componentCount);
		PopIndent(ctx);

		WriteLine(ctx, "};");
		NewLine(ctx);
	}

	WriteSectionLine(ctx, "Prefabs");

	for (u32 i = 0; i < assets.prefabDescCount; ++i)
	{
		const PrefabDesc &desc = assets.prefabDescs[i];

		WriteLine(ctx, "Prefab %s = {", desc.name);

		PushIndent(ctx);
		WriteLine(ctx, ".id = %u,", desc.id.slot);
		WriteLine(ctx, ".entities = {");
		PushIndent(ctx);

		for (u32 e = 0; e < desc.entityCount; ++e)
		{
			const EntityDesc &entity = desc.entities[e];

			WriteLine(ctx, "{");
			PushIndent(ctx);
			WriteLine(ctx, ".name = \"%s\",", entity.name);
			WriteEntityDescBody(ctx, entity, e, desc.components, desc.componentCount);
			PopIndent(ctx);
			WriteLine(ctx, "},");
		}

		PopIndent(ctx);
		WriteLine(ctx, "},");
		PopIndent(ctx);

		WriteLine(ctx, "};");
		NewLine(ctx);
	}

	WriteSectionLine(ctx, "Rooms");

	for (u32 i = 0; i < assets.roomDescCount; ++i)
	{
		const RoomDesc &desc = assets.roomDescs[i];

		WriteLine(ctx, "Room %s = {", desc.name);

		PushIndent(ctx);
		WriteLine(ctx, ".id = %u,", desc.id.slot);
		WriteLine(ctx, ".pos = {%d, %d},", desc.pos.x, desc.pos.y);
		WriteLine(ctx, ".layers = {");
		PushIndent(ctx);

		for (u32 l = 0; l < desc.layerCount; ++l)
		{
			const LayerDesc &layer = desc.layers[l];

			WriteLine(ctx, "{");
			PushIndent(ctx);
			WriteLine(ctx, ".id = %u,", layer.id.slot);
			WriteLine(ctx, ".name = \"%s\",", layer.name);
			WriteLine(ctx, ".isBase = %d,", layer.isBase);
			WriteLine(ctx, ".visible = %d,", layer.visible);
			WriteLine(ctx, ".isCollider = %d,", layer.isCollider);
			WriteLine(ctx, ".size = {%u, %u},", layer.size.x, layer.size.y);

			if (layer.tileCount > 0)
			{
				WriteLine(ctx, ".tiles = {");
				PushIndent(ctx);

				char tileLine[MAX_PATH_LENGTH];
				i32 tileLineLen = 0;
				u32 tilesInLine = 0;
				for (u32 t = 0; t < layer.tileCount; ++t)
				{
					const TileDesc &tile = layer.tiles[t];
					// Both union members are u32, so one raw view serializes either
					tileLineLen += SPrintf(tileLine + tileLineLen, "{%u, %u, %u}, ", tile.x, tile.y, tile.collider);
					if (++tilesInLine == 8 || t + 1 == layer.tileCount)
					{
						WriteLine(ctx, "%s", tileLine);
						tileLineLen = 0;
						tilesInLine = 0;
					}
				}

				PopIndent(ctx);
				WriteLine(ctx, "},");
			}

			PopIndent(ctx);
			WriteLine(ctx, "},");
		}

		PopIndent(ctx);
		WriteLine(ctx, "},");
		PopIndent(ctx);

		WriteLine(ctx, "};");
		NewLine(ctx);
	}

	WriteSectionLine(ctx, "Audio clips");

	for (u32 i = 0; i < assets.audioClipDescCount; ++i)
	{
		const AudioClipDesc &desc = assets.audioClipDescs[i];

		WriteLine(ctx, "AudioClip %s = {", desc.name);

		PushIndent(ctx);
		WriteLine(ctx, ".id = %u,", desc.id.slot);
		WriteLine(ctx, ".filename = \"%s\",", desc.filename);
		PopIndent(ctx);

		WriteLine(ctx, "};");
		NewLine(ctx);
	}

	WriteSectionLine(ctx, "Music files");

	for (u32 i = 0; i < assets.musicFileDescCount; ++i)
	{
		const MusicFileDesc &desc = assets.musicFileDescs[i];

		WriteLine(ctx, "MusicFile %s = {", desc.name);

		PushIndent(ctx);
		WriteLine(ctx, ".id = %u,", desc.id.slot);
		WriteLine(ctx, ".filename = \"%s\",", desc.filename);
		PopIndent(ctx);

		WriteLine(ctx, "};");
		NewLine(ctx);
	}

	if ( !WriteEntireFile(path, ctx.arena.base, ctx.arena.used) )
	{
		LOG(Warning, "Could not write data file: %s\n", path);
	}
}










////////////////////////////////////////////////////////////////////////
// Text scanner / parser

enum DTokenId
{
	// Single character tokens
	TOKEN_LEFT_BRACE,
	TOKEN_RIGHT_BRACE,
	TOKEN_COMMA,
	TOKEN_DOT,
	TOKEN_MINUS,
	TOKEN_SEMICOLON,
	TOKEN_EQUAL,
	// Literals
	TOKEN_IDENTIFIER,
	TOKEN_STRING,
	TOKEN_CHARACTER,
	TOKEN_NUMBER,
	// End
	TOKEN_EOF,
	TOKEN_COUNT,
};

static const char *DTokenIdNames[] =
{
	// Single character tokens
	"TOKEN_LEFT_BRACE",
	"TOKEN_RIGHT_BRACE",
	"TOKEN_COMMA",
	"TOKEN_DOT",
	"TOKEN_MINUS",
	"TOKEN_SEMICOLON",
	"TOKEN_EQUAL",
	// Literals
	"TOKEN_IDENTIFIER",
	"TOKEN_STRING",
	"TOKEN_CHARACTER",
	"TOKEN_NUMBER",
	// End
	"TOKEN_EOF",
};

CT_ASSERT(ARRAY_COUNT(DTokenIdNames) == TOKEN_COUNT);

struct DToken
{
	DTokenId id;
	u32 line;
	String lexeme;
};

static DToken gNullToken = {
	.id = TOKEN_COUNT,
	.line = 0,
	.lexeme = { "", 0 },
};

struct DTokenList
{
	Arena *arena;
	DToken *tokens;
	i32 count;
	bool valid;
};

struct DScanner
{
	i32 start;
	i32 current;
	i32 currentInLine;
	u32 line;
	bool hasErrors;

	const char *text;
	u32 textSize;
};

static bool Char_IsEOL(char character)
{
	return character == '\n';
}

static bool Char_IsAlpha(char character)
{
	return
		( character >= 'a' && character <= 'z' ) ||
		( character >= 'A' && character <= 'Z' ) ||
		( character == '_' );
}

static bool Char_IsDigit(char character)
{
	return character >= '0' && character <= '9';
}

static bool Char_IsAlphaNumeric(char character)
{
	return Char_IsAlpha(character) || Char_IsDigit(character);
}

static bool DScanner_IsAtEnd(const DScanner &scanner)
{
	return scanner.current >= scanner.textSize;
}

static char DScanner_Advance(DScanner &scanner)
{
	ASSERT(scanner.current < scanner.textSize);
	char currentChar = scanner.text[scanner.current];
	scanner.current++;
	scanner.currentInLine++;
	return currentChar;
}

static bool DScanner_Consume(DScanner &scanner, char expected)
{
	if ( DScanner_IsAtEnd(scanner) ) return false;
	if ( scanner.text[scanner.current] != expected ) return false;
	scanner.current++;
	scanner.currentInLine++;
	return true;
}

static char DScanner_Peek(const DScanner &scanner)
{
	return DScanner_IsAtEnd(scanner) ? '\0' : scanner.text[ scanner.current ];
}

static char DScanner_PeekNext(const DScanner &scanner)
{
	if (scanner.current + 1 >= scanner.textSize) return '\0';
	return scanner.text[ scanner.current + 1 ];
}

static String DScanner_ScannedString(const DScanner &scanner)
{
	const char *lexemeStart = scanner.text + scanner.start;
	u32 lexemeSize = scanner.current - scanner.start;
	String scannedString = { lexemeStart, lexemeSize };
	return scannedString;
}

static void DScanner_AddToken(const DScanner &scanner, DTokenList &tokenList, DTokenId tokenId)
{
	DToken newToken = {};
	newToken.id = tokenId;
	newToken.lexeme = DScanner_ScannedString(scanner);
	newToken.line = scanner.line;

	PushStruct(*tokenList.arena, DToken);
	tokenList.tokens[tokenList.count++] = newToken;
}

static void DScanner_SetError(DScanner &scanner, const char *format, ...)
{
	va_list vaList;
	va_start(vaList, format);
	Printf("ERROR: %d:%d: ", scanner.line, scanner.currentInLine);
	VPrintf(format, vaList);
	Printf("\n");
	va_end(vaList);
	scanner.hasErrors = true;
}

static void DScanner_ScanToken(DScanner &scanner, DTokenList &tokenList)
{
	scanner.start = scanner.current;

	char c = DScanner_Advance(scanner);

	switch (c)
	{
		case '{': DScanner_AddToken(scanner, tokenList, TOKEN_LEFT_BRACE); break;
		case '}': DScanner_AddToken(scanner, tokenList, TOKEN_RIGHT_BRACE); break;
		case ',': DScanner_AddToken(scanner, tokenList, TOKEN_COMMA); break;
		case '.': DScanner_AddToken(scanner, tokenList, TOKEN_DOT); break;
		case '-': DScanner_AddToken(scanner, tokenList, TOKEN_MINUS); break;
		case ';': DScanner_AddToken(scanner, tokenList, TOKEN_SEMICOLON); break;
		case '=': DScanner_AddToken(scanner, tokenList, TOKEN_EQUAL ); break;
		case '/':
			if ( DScanner_Consume(scanner, '/') )
			{
				// Discard all chars until the end of line is reached
				while ( !Char_IsEOL( DScanner_Peek(scanner) ) && !DScanner_IsAtEnd(scanner) )
				{
					DScanner_Advance(scanner);
				}
			}
			else if ( DScanner_Consume(scanner, '*') )
			{
				while( !(DScanner_Consume(scanner, '*') && DScanner_Consume(scanner, '/')) || DScanner_IsAtEnd(scanner) )
				{
					if ( Char_IsEOL( DScanner_Peek(scanner) ) ) {
						scanner.line++;
						scanner.currentInLine = 0;
					}
					DScanner_Advance(scanner);
				}
			}
			else
			{
				DScanner_SetError( scanner, "Unterminated comment." );
				return;
			}
			break;

		// Skip whitespaces
		case ' ':
		case '\r':
		case '\t':
			break;

		// End of line counter
		case '\n':
			scanner.line++;
			scanner.currentInLine = 0;
			break;

		case '\"':
			while ( DScanner_Peek(scanner) != '\"' && !DScanner_IsAtEnd(scanner) )
			{
				if ( Char_IsEOL( DScanner_Peek(scanner) ) ) {
					scanner.line++;
					scanner.currentInLine = 0;
				}
				DScanner_Advance(scanner);
			}

			if ( DScanner_IsAtEnd(scanner) )
			{
				DScanner_SetError( scanner, "Unterminated string." );
				return;
			}

			DScanner_Advance(scanner);

			DScanner_AddToken(scanner, tokenList, TOKEN_STRING);
			break;

		case '\'':
			{
				if ( DScanner_IsAtEnd(scanner) )
				{
					DScanner_SetError( scanner, "Unterminated character" );
					return;
				}
				char character = DScanner_Advance(scanner);
				if ( DScanner_Advance(scanner) != '\'' )
				{
					DScanner_SetError( scanner, "Invalid char literal '%c'", character);
					return;
				}
				DScanner_AddToken(scanner, tokenList, TOKEN_CHARACTER);
			}
			break;

		default:
			if ( Char_IsDigit(c) )
			{
				while ( Char_IsDigit( DScanner_Peek(scanner) ) ) DScanner_Advance(scanner);

				if ( DScanner_Peek(scanner) == '.' && Char_IsDigit( DScanner_PeekNext(scanner) ) )
				{
					DScanner_Advance(scanner);
					while ( Char_IsDigit(DScanner_Peek(scanner)) ) DScanner_Advance(scanner);
					DScanner_Consume(scanner, 'f');
				}

				DScanner_AddToken(scanner, tokenList, TOKEN_NUMBER);
			}
			else if ( Char_IsAlpha(c) )
			{
				while ( Char_IsAlphaNumeric( DScanner_Peek(scanner) ) ) DScanner_Advance(scanner);

				DScanner_AddToken(scanner, tokenList, TOKEN_IDENTIFIER);
			}
			else
			{
				DScanner_SetError( scanner, "Unexpected character '%c'.", c );
			}
	}
}

static DTokenList DScan(Arena &arena, const char *text, u32 textSize)
{
	DTokenList tokenList = {};
	tokenList.arena = &arena;
	tokenList.tokens = (DToken*)(arena.base + arena.used);

	DScanner scanner = {};
	scanner.line = 1;
	scanner.hasErrors = false;
	scanner.text = text;
	scanner.textSize = textSize;

	while ( ! DScanner_IsAtEnd(scanner) )
	{
		DScanner_ScanToken(scanner, tokenList);
	}

	DScanner_AddToken(scanner, tokenList, TOKEN_EOF);
	tokenList.valid = !scanner.hasErrors;

	return tokenList;
}

struct DParser
{
	const DTokenList *tokenList;
	Arena *arena;
	u32 nextToken;
	u32 lastToken;
	bool hasErrors;
	bool hasFinished;
	AssetDescriptors *descriptors;
};


static DParser DParser_Init(const DTokenList &tokenList, Arena &arena, AssetDescriptors &descriptors)
{
	const DParser parser = {
		.tokenList = &tokenList,
		.arena = &arena,
		.descriptors = &descriptors,
	};
	return parser;
}

static u32 DParser_RemainingTokens(const DParser &parser)
{
	return parser.tokenList->count - parser.nextToken;
}

static const DToken &DParser_GetPreviousToken( const DParser &parser )
{
	ASSERT(parser.nextToken > 0);
	return parser.tokenList->tokens[parser.nextToken-1];
}

static const DToken &DParser_GetNextToken( const DParser &parser )
{
	ASSERT(parser.nextToken < parser.tokenList->count);
	return parser.tokenList->tokens[parser.nextToken];
}

static bool DParser_HasFinished(const DParser &parser)
{
	const bool hasErrors = parser.hasErrors;
	const bool hasFinished = DParser_GetNextToken(parser).id == TOKEN_EOF;
	return hasErrors || hasFinished;
}

static bool DParser_IsNextToken( const DParser &parser, DTokenId tokenId )
{
	if ( DParser_HasFinished( parser ) ) {
		return tokenId == TOKEN_EOF;
	} else {
		ASSERT(parser.nextToken < parser.tokenList->count);
		return tokenId == parser.tokenList->tokens[parser.nextToken].id;
	}
}

static const DToken &DParser_GetLastToken( const DParser &parser )
{
	return parser.tokenList->tokens[parser.lastToken];
}

static void DParser_SetError(DParser &parser, const char *message)
{
	DToken token = DParser_GetNextToken(parser);
	LOG(Error, "DParser error: %d:%.*s %s\n", token.line, token.lexeme.size, token.lexeme.str, message);
	parser.hasErrors = true;
}

static const DToken &DParser_Consume( DParser &parser )
{
	if ( !DParser_HasFinished( parser ) ) {
		parser.nextToken++;
		parser.lastToken = parser.nextToken > parser.lastToken ? parser.nextToken : parser.lastToken;
		return DParser_GetPreviousToken(parser);
	} else {
		DParser_SetError(parser, "Reached end of file");
		return gNullToken;
	}
}

static bool DParser_TryConsume( DParser &parser, DTokenId tokenId )
{
	if ( DParser_IsNextToken( parser, tokenId ) ) {
		DParser_Consume( parser );
		return true;
	}
	return false;
}

static String DParser_ConsumeLexeme( DParser &parser )
{
	const DToken &token = DParser_Consume(parser);
	return token.lexeme;
}

static String DParser_ConsumeString( DParser &parser )
{
	const DToken &token = DParser_Consume(parser);
	String str = token.lexeme;
	str.str += 1; // Remove ""
	str.size -= 2; // Remove ""
	return str;
}

static u8 DParser_ConsumeU8( DParser &parser )
{
	const DToken &token = DParser_Consume(parser);
	const u8 res = I32ToU8(StrToInt(token.lexeme));
	return res;
}

static u16 DParser_ConsumeU16( DParser &parser )
{
	const DToken &token = DParser_Consume(parser);
	const u16 res = I32ToU16(StrToInt(token.lexeme));
	return res;
}

static f32 DParser_ConsumeF32( DParser &parser )
{
	const DToken &token = DParser_Consume(parser);
	const f32 res = StrToFloat(token.lexeme);
	return res;
}

static i32 DParser_ConsumeI32( DParser &parser )
{
	const bool neg = DParser_TryConsume(parser, TOKEN_MINUS);
	const DToken &token = DParser_Consume(parser);
	const i32 res = StrToInt(token.lexeme);
	return neg ? -res : res;
}

static u32 DParser_ConsumeU32( DParser &parser )
{
	const DToken &token = DParser_Consume(parser);
	const u32 res = StrToUnsignedInt(token.lexeme);
	return res;
}

static ID DParser_ConsumeID( DParser &parser )
{
	ID id = { .slot = DParser_ConsumeU32(parser) };
	return id;
}

static void DParser_SkipFieldValue( DParser &parser )
{
	u32 depth = 0;

	while ( !DParser_HasFinished( parser ) )
	{
		const DTokenId next = DParser_GetNextToken(parser).id;

		if ( depth == 0 && ( next == TOKEN_COMMA || next == TOKEN_RIGHT_BRACE ) ) {
			break;
		}

		if ( next == TOKEN_LEFT_BRACE ) { depth++; }
		else if ( next == TOKEN_RIGHT_BRACE ) { depth--; }

		DParser_Consume(parser);
	}
}

static uint2 DParser_ConsumeUint2( DParser &parser )
{
	uint2 res = {};
	DParser_TryConsume(parser, TOKEN_LEFT_BRACE);
	res.x = (u32)StrToInt(DParser_ConsumeLexeme(parser));
	DParser_TryConsume(parser, TOKEN_COMMA);
	res.y = (u32)StrToInt(DParser_ConsumeLexeme(parser));
	DParser_TryConsume(parser, TOKEN_RIGHT_BRACE);
	return res;
}

static float2 DParser_ConsumeFloat2( DParser &parser )
{
	float2 res = {};
	DParser_TryConsume(parser, TOKEN_LEFT_BRACE);
	const bool sx = DParser_TryConsume(parser, TOKEN_MINUS);
	res.x = DParser_ConsumeF32(parser);
	res.x = sx ? -res.x : res.x;
	DParser_TryConsume(parser, TOKEN_COMMA);
	const bool sy = DParser_TryConsume(parser, TOKEN_MINUS);
	res.y = DParser_ConsumeF32(parser);
	res.y = sy ? -res.y : res.y;
	DParser_TryConsume(parser, TOKEN_COMMA);
	DParser_TryConsume(parser, TOKEN_RIGHT_BRACE);
	return res;
}

static float3 DParser_ConsumeFloat3( DParser &parser )
{
	float3 res = {};
	DParser_TryConsume(parser, TOKEN_LEFT_BRACE);
	const bool sx = DParser_TryConsume(parser, TOKEN_MINUS);
	res.x = DParser_ConsumeF32(parser);
	res.x = sx ? -res.x : res.x;
	DParser_TryConsume(parser, TOKEN_COMMA);
	const bool sy = DParser_TryConsume(parser, TOKEN_MINUS);
	res.y = DParser_ConsumeF32(parser);
	res.y = sy ? -res.y : res.y;
	DParser_TryConsume(parser, TOKEN_COMMA);
	const bool sz = DParser_TryConsume(parser, TOKEN_MINUS);
	res.z = DParser_ConsumeF32(parser);
	res.z = sz ? -res.z : res.z;
	DParser_TryConsume(parser, TOKEN_RIGHT_BRACE);
	return res;
}

static int2 DParser_ConsumeInt2( DParser &parser )
{
	int2 res = {};
	DParser_TryConsume(parser, TOKEN_LEFT_BRACE);
	res.x = DParser_ConsumeI32(parser);
	DParser_TryConsume(parser, TOKEN_COMMA);
	res.y = DParser_ConsumeI32(parser);
	DParser_TryConsume(parser, TOKEN_RIGHT_BRACE);
	return res;
}

static void DParser_ConsumeTiles( DParser &parser, LayerDesc &layer )
{
	Arena &arena = *parser.arena;

	DParser_TryConsume(parser, TOKEN_LEFT_BRACE);

	// Tiles are pushed contiguously (nothing else allocates from the arena in this loop)
	layer.tiles = (TileDesc*)(arena.base + arena.used);
	layer.tileCount = 0;

	while ( DParser_TryConsume(parser, TOKEN_LEFT_BRACE) && !DParser_HasFinished(parser) )
	{
		TileDesc &tile = *PushStruct(arena, TileDesc);
		tile.x = I32ToU16(DParser_ConsumeI32(parser));
		DParser_TryConsume(parser, TOKEN_COMMA);
		tile.y = I32ToU16(DParser_ConsumeI32(parser));
		DParser_TryConsume(parser, TOKEN_COMMA);
		tile.collider = DParser_ConsumeU32(parser); // Raw view; a sprite layer reads it back as spriteId
		DParser_TryConsume(parser, TOKEN_RIGHT_BRACE);
		DParser_TryConsume(parser, TOKEN_COMMA);
		layer.tileCount++;
	}

	DParser_TryConsume(parser, TOKEN_RIGHT_BRACE);
}

static void DParser_ConsumeScriptProperties( DParser &parser, ScriptComponentDesc &script, ComponentDescPool &pool);

static void DParser_ConsumeProperty( DParser &parser, const ReflexMember &member, void *field )
{
	if ( !member.ops->parse(parser, member, field) ) {
		LOG(Warning, "Property <%s> has a value that is not a valid <%s>.\n", member.name, member.typeName);
	}
}

static void DParser_ConsumeProperties( DParser &parser, const ReflexStruct &type, void *base )
{
	DParser_TryConsume(parser, TOKEN_LEFT_BRACE);

	while ( !DParser_IsNextToken(parser, TOKEN_RIGHT_BRACE) && !DParser_HasFinished(parser) )
	{
		DParser_TryConsume(parser, TOKEN_DOT);

		const String field = DParser_ConsumeLexeme(parser);

		DParser_TryConsume(parser, TOKEN_EQUAL);

		if ( const ReflexMember *member = FindProperty(type, field) ) {
			DParser_ConsumeProperty(parser, *member, (byte*)base + member->offset);
		} else {
			LOG(Warning, "<%s> has no property <%.*s>, its value is skipped.\n", type.name, field.size, field.str);
			DParser_SkipFieldValue(parser);
		}

		DParser_TryConsume(parser, TOKEN_COMMA);
	}

	DParser_TryConsume(parser, TOKEN_RIGHT_BRACE);
}

static void DParser_ConsumeEntityComponent( DParser &parser, ComponentDescPool &pool, u32 entityIndex, ComponentType componentType )
{
	const ReflexStruct *type = ComponentDescReflexStruct(componentType);
	if ( !type ) {
		LOG(Warning, "Component <%s> has no reflected descriptor, it is skipped.\n", ComponentNames[componentType]);
	}

	ComponentDesc *component = type ? PushComponentDesc(pool, entityIndex, componentType) : nullptr;
	if ( component ) {
		DParser_ConsumeProperties(parser, *type, ComponentDescData(*component));
	} else {
		DParser_SkipFieldValue(parser);
	}
}

static void DParser_ConsumeEntityScript( DParser &parser, ComponentDescPool &pool, u32 entityIndex )
{
	DParser_TryConsume(parser, TOKEN_LEFT_BRACE);

	ScriptComponentDesc scriptDesc = {};

	while ( !DParser_IsNextToken(parser, TOKEN_RIGHT_BRACE) && !DParser_HasFinished(parser) )
	{
		DParser_TryConsume(parser, TOKEN_DOT);

		const String field = DParser_ConsumeLexeme(parser);

		DParser_TryConsume(parser, TOKEN_EQUAL);

		static const String sName = MakeString("name");
		static const String sProperties = MakeString("properties");

		if ( StrEq( field, sName ) ) {
			scriptDesc.name = PushString(*parser.arena, DParser_ConsumeString(parser));
		} else if ( StrEq( field, sProperties ) ) {
			DParser_ConsumeScriptProperties(parser, scriptDesc, pool);
		}

		DParser_TryConsume(parser, TOKEN_COMMA);
	}

	DParser_TryConsume(parser, TOKEN_RIGHT_BRACE);

	// The name is what AddScript resolves the component from, so a block without one
	// leaves the entity scriptless rather than handing it a null to look up
	if ( !scriptDesc.name ) {
		LOG(Warning, "An entity has a script without a name, it is ignored.\n");
		return;
	}

	if ( ComponentDesc *component = PushComponentDesc(pool, entityIndex, ComponentType_Script) ) {
		component->script = scriptDesc;
	}
}

static bool DParser_ConsumeEntityField( DParser &parser, String field, EntityDesc &entity, ComponentDescPool &pool, u32 entityIndex )
{
	static const String sId = MakeString("id");
	static const String sName = MakeString("name");
	static const String sPos = MakeString("pos");
	static const String sScale = MakeString("scale");

	if ( StrEq( field, sId ) ) {
		entity.id = DParser_ConsumeID(parser);
		return true;
	} else if ( StrEq( field, sName ) ) {
		entity.name = PushString(*parser.arena, DParser_ConsumeString(parser));
		return true;
	} else if ( StrEq( field, sPos ) ) {
		entity.pos = DParser_ConsumeFloat3(parser);
		return true;
	} else if ( StrEq( field, sScale ) ) {
		entity.scale = DParser_ConsumeF32(parser);
		return true;
	}

	for (u32 type = 0; type < ComponentType_Count; ++type)
	{
		if ( StrEq( field, ComponentFieldNames[type] ) )
		{
			if ( type == ComponentType_Script ) {
				DParser_ConsumeEntityScript(parser, pool, entityIndex);
			} else {
				DParser_ConsumeEntityComponent(parser, pool, entityIndex, (ComponentType)type);
			}
			return true;
		}
	}

	return false;
}





static void DParser_ConsumePrefabEntities( DParser &parser, PrefabDesc &prefab )
{
	DParser_TryConsume(parser, TOKEN_LEFT_BRACE);

	ComponentDescPool pool = {
		.components = PushZeroArray(*parser.arena, ComponentDesc, MAX_PREFAB_COMPONENTS),
		.componentCapacity = MAX_PREFAB_COMPONENTS,
		.properties = PushZeroArray(*parser.arena, ScriptPropertyDesc, MAX_PREFAB_SCRIPT_PROPERTIES),
		.propertyCapacity = MAX_PREFAB_SCRIPT_PROPERTIES,
	};

	while ( DParser_TryConsume(parser, TOKEN_LEFT_BRACE) && !DParser_HasFinished(parser) )
	{
		EntityDesc entityDesc = {};
		const u32 entityIndex = prefab.entityCount;

		while ( !DParser_IsNextToken(parser, TOKEN_RIGHT_BRACE) && !DParser_HasFinished(parser) )
		{
			DParser_TryConsume(parser, TOKEN_DOT);

			const String field = DParser_ConsumeLexeme(parser);

			DParser_TryConsume(parser, TOKEN_EQUAL);

			if ( !DParser_ConsumeEntityField(parser, field, entityDesc, pool, entityIndex) ) {
				LOG(Warning, "Unknown prefab entity field <%.*s>.\n", field.size, field.str);
				DParser_SkipFieldValue(parser);
			}

			DParser_TryConsume(parser, TOKEN_COMMA);
		}

		DParser_TryConsume(parser, TOKEN_RIGHT_BRACE);
		DParser_TryConsume(parser, TOKEN_COMMA);

		if ( prefab.entityCount < ARRAY_COUNT(prefab.entities) ) {
			prefab.entities[prefab.entityCount++] = entityDesc;
		}
	}

	prefab.components = pool.components;
	prefab.componentCount = pool.componentCount;

	DParser_TryConsume(parser, TOKEN_RIGHT_BRACE);
}

static void DParser_ConsumeRoomLayers( DParser &parser, RoomDesc &room )
{
	DParser_TryConsume(parser, TOKEN_LEFT_BRACE);

	while ( DParser_TryConsume(parser, TOKEN_LEFT_BRACE) && !DParser_HasFinished(parser) )
	{
		LayerDesc layerDesc = {};

		while ( !DParser_IsNextToken(parser, TOKEN_RIGHT_BRACE) && !DParser_HasFinished(parser) )
		{
			DParser_TryConsume(parser, TOKEN_DOT);

			const String field = DParser_ConsumeLexeme(parser);

			DParser_TryConsume(parser, TOKEN_EQUAL);

			static const String sId = MakeString("id");
			static const String sName = MakeString("name");
			static const String sIsBase = MakeString("isBase");
			static const String sVisible = MakeString("visible");
			static const String sIsCollider = MakeString("isCollider");
			static const String sSize = MakeString("size");
			static const String sTiles = MakeString("tiles");

			if ( StrEq( field, sId ) ) {
				layerDesc.id = DParser_ConsumeID(parser);
			} else if ( StrEq( field, sName ) ) {
				layerDesc.name = PushString(*parser.arena, DParser_ConsumeString(parser));
			} else if ( StrEq( field, sIsBase ) ) {
				layerDesc.isBase = DParser_ConsumeU8(parser) != 0;
			} else if ( StrEq( field, sVisible ) ) {
				layerDesc.visible = DParser_ConsumeU8(parser) != 0;
			} else if ( StrEq( field, sIsCollider ) ) {
				layerDesc.isCollider = DParser_ConsumeU8(parser) != 0;
			} else if ( StrEq( field, sSize ) ) {
				layerDesc.size = DParser_ConsumeUint2(parser);
			} else if ( StrEq( field, sTiles ) ) {
				DParser_ConsumeTiles(parser, layerDesc);
			}

			DParser_TryConsume(parser, TOKEN_COMMA);
		}

		DParser_TryConsume(parser, TOKEN_RIGHT_BRACE);
		DParser_TryConsume(parser, TOKEN_COMMA);

		if ( room.layerCount < ARRAY_COUNT(room.layers) ) {
			room.layers[room.layerCount++] = layerDesc;
		}
	}

	DParser_TryConsume(parser, TOKEN_RIGHT_BRACE);
}

// Parsing runs before RegisterScripts, so the script's layout comes from reflex by name. That
// name is the reason .name has to come before .properties in the script block.
static void DParser_ConsumeScriptProperties( DParser &parser, ScriptComponentDesc &script, ComponentDescPool &pool)
{
	const ReflexStruct *type = script.name ? ReflexGetStructFromName(script.name) : nullptr;
	if ( !type ) {
		LOG(Warning, "Script <%s> is not reflected, its properties are skipped.\n", script.name ? script.name : "");
		DParser_SkipFieldValue(parser);
		return;
	}

	DParser_TryConsume(parser, TOKEN_LEFT_BRACE);

	script.properties = pool.properties + pool.propertyCount;
	script.propertyCount = 0;

	while ( !DParser_IsNextToken(parser, TOKEN_RIGHT_BRACE) && !DParser_HasFinished(parser) )
	{
		DParser_TryConsume(parser, TOKEN_DOT);

		const String field = DParser_ConsumeLexeme(parser);

		DParser_TryConsume(parser, TOKEN_EQUAL);

		const ReflexMember *member = FindProperty(*type, field);
		if ( !member ) {
			LOG(Warning, "Script <%s> has no property <%.*s>, its value is skipped.\n", script.name, field.size, field.str);
			DParser_SkipFieldValue(parser);
		} else if ( ReflexGetTypeSize(member->reflexId) > MAX_PROPERTY_VALUE_SIZE ) {
			LOG(Warning, "Script <%s> property <%s> is too large to be stored, its value is skipped.\n", script.name, member->name);
			DParser_SkipFieldValue(parser);
		} else if ( pool.propertyCount == pool.propertyCapacity ) {
			LOG(Warning, "Script <%s> drops property <%s>, the property pool is full.\n", script.name, member->name);
			DParser_SkipFieldValue(parser);
		} else {
			ScriptPropertyDesc &property = pool.properties[pool.propertyCount++];
			property = {
				.name = PushString(*parser.arena, member->name),
				.type = member->reflexId,
			};
			DParser_ConsumeProperty(parser, *member, property.value);
			script.propertyCount++;
		}

		DParser_TryConsume(parser, TOKEN_COMMA);
	}

	DParser_TryConsume(parser, TOKEN_RIGHT_BRACE);
}

static void DParser_ConsumeUntil( DParser &parser, DTokenId tokenId )
{
	while ( !DParser_IsNextToken( parser, tokenId ) && !DParser_HasFinished( parser ) )
	{
		DParser_Consume( parser );
	}
	while ( DParser_TryConsume( parser, tokenId ) ) {
		// Do nothing, just consume the expected token
	}
}

static const String sSceneStr = MakeString("Scene");
static const String sMaterialStr = MakeString("Material");
static const String sTextureStr = MakeString("Texture");
static const String sSpriteStr = MakeString("Sprite");
static const String sEntityStr = MakeString("Entity");
static const String sPrefabStr = MakeString("Prefab");
static const String sRoomStr = MakeString("Room");
static const String sAudioClipStr = MakeString("AudioClip");
static const String sMusicFileStr = MakeString("MusicFile");

const char *StringToCStr( String str )
{
	static char cstr[128];
	StrCopy(cstr, str);
	return cstr;
}

static void DParseDescriptors(DParser &parser, bool countOnly)
{
	AssetDescriptors &descriptors = *parser.descriptors;

	while ( !DParser_HasFinished( parser ) )
	{
		if ( DParser_TryConsume(parser, TOKEN_IDENTIFIER) )
		{
			const String type = DParser_GetPreviousToken(parser).lexeme;

			// Scene
			if ( StrEq(type, sSceneStr) ) {
				if ( countOnly ) goto parse_descriptors_continue;

				SceneDesc &desc = descriptors.sceneDesc;
				const String name = DParser_ConsumeLexeme( parser ); // Unused for now

				DParser_TryConsume( parser, TOKEN_EQUAL );
				DParser_TryConsume( parser, TOKEN_LEFT_BRACE );
				while ( !DParser_IsNextToken( parser, TOKEN_RIGHT_BRACE ) )
				{
					DParser_TryConsume( parser, TOKEN_DOT );

					const String field = DParser_ConsumeLexeme( parser );

					DParser_TryConsume( parser, TOKEN_EQUAL );

					static const String sProjectionType = MakeString("projectionType");
					static const String sAmbientLight = MakeString("ambientLight");
					if ( StrEq( field, sProjectionType ) ) {
						const char *cstr = StringToCStr( DParser_ConsumeString(parser) );
						desc.projectionType = StrToProjectionType( cstr );
					} else if ( StrEq( field, sAmbientLight ) ) {
						desc.ambientLight = DParser_ConsumeFloat3(parser);
					}
				}
			}

			// Texture
			else if ( StrEq(type, sTextureStr) ) {

				const u32 index = descriptors.textureDescCount++;
				if ( countOnly ) goto parse_descriptors_continue;

				TextureDesc &desc = descriptors.textureDescs[index];
				const String name = DParser_ConsumeLexeme( parser );
				desc.name = PushString(*parser.arena, name);
				DParser_TryConsume( parser, TOKEN_EQUAL );
				DParser_TryConsume( parser, TOKEN_LEFT_BRACE );
				while ( !DParser_IsNextToken( parser, TOKEN_RIGHT_BRACE ) )
				{
					DParser_TryConsume( parser, TOKEN_DOT );

					const String field = DParser_ConsumeLexeme( parser );

					DParser_TryConsume( parser, TOKEN_EQUAL );

					static const String sId = MakeString("id");
					static const String sFilename = MakeString("filename");
					static const String sMipmap = MakeString("mipmap");
					if ( StrEq( field, sId ) ) {
						desc.id = DParser_ConsumeID(parser);
					} else if ( StrEq( field, sFilename ) ) {
						desc.filename = PushString(*parser.arena, DParser_ConsumeString(parser));
					} else if ( StrEq( field, sMipmap ) ) {
						desc.mipmap = DParser_ConsumeU8(parser);
					} else {
						LOG(Warning, "Unknown Texture field <%.*s>.\n", field.size, field.str);
						DParser_SkipFieldValue(parser);
					}

					DParser_TryConsume( parser, TOKEN_COMMA );
				}

			// Material
			} else if ( StrEq(type, sMaterialStr) ) {

				const u32 index = descriptors.materialDescCount++;
				if ( countOnly ) goto parse_descriptors_continue;

				MaterialDesc &desc = descriptors.materialDescs[index];
				const String name = DParser_ConsumeLexeme( parser );
				desc.name = PushString(*parser.arena, name);
				DParser_TryConsume( parser, TOKEN_EQUAL );
				DParser_TryConsume( parser, TOKEN_LEFT_BRACE );
				while ( !DParser_IsNextToken( parser, TOKEN_RIGHT_BRACE ) )
				{
					DParser_TryConsume( parser, TOKEN_DOT );

					const String field = DParser_ConsumeLexeme( parser );

					DParser_TryConsume( parser, TOKEN_EQUAL );

					static const String sId = MakeString("id");
					static const String sTextureId = MakeString("textureId");
					static const String sPipelineName = MakeString("pipelineName");
					static const String sUvScale = MakeString("uvScale");

					if ( StrEq( field, sId ) ) {
						desc.id = DParser_ConsumeID(parser);
					} else if ( StrEq( field, sTextureId ) ) {
						desc.textureId = DParser_ConsumeID(parser);
					} else if ( StrEq( field, sPipelineName ) ) {
						desc.pipelineName = PushString(*parser.arena, DParser_ConsumeString(parser));
					} else if ( StrEq( field, sUvScale ) ) {
						desc.uvScale = DParser_ConsumeF32(parser);
					} else {
						LOG(Warning, "Unknown Material field <%.*s>.\n", field.size, field.str);
						DParser_SkipFieldValue(parser);
					}

					DParser_TryConsume( parser, TOKEN_COMMA );
				}

			// Sprite
			} else if ( StrEq(type, sSpriteStr) ) {

				const u32 index = descriptors.spriteDescCount++;
				if ( countOnly ) goto parse_descriptors_continue;

				SpriteDesc &desc = descriptors.spriteDescs[index];
				const String name = DParser_ConsumeLexeme( parser );
				desc.name = PushString(*parser.arena, name);
				DParser_TryConsume( parser, TOKEN_EQUAL );
				DParser_TryConsume( parser, TOKEN_LEFT_BRACE );
				while ( !DParser_IsNextToken( parser, TOKEN_RIGHT_BRACE ) )
				{
					DParser_TryConsume( parser, TOKEN_DOT );

					const String field = DParser_ConsumeLexeme( parser );

					DParser_TryConsume( parser, TOKEN_EQUAL );

					static const String sId         = MakeString("id");
					static const String sTextureId = MakeString("textureId");
					static const String sPos        = MakeString("pos");
					static const String sSize       = MakeString("size");
					static const String sFrameCount = MakeString("frameCount");
					static const String sFps        = MakeString("fps");
					static const String sLoop       = MakeString("loop");

					if ( StrEq( field, sId ) ) {
						desc.id = DParser_ConsumeID(parser);
					} else if ( StrEq( field, sTextureId ) ) {
						desc.textureId = DParser_ConsumeID(parser);
					} else if ( StrEq( field, sPos ) ) {
						desc.pos = DParser_ConsumeUint2(parser);
					} else if ( StrEq( field, sSize ) ) {
						desc.size = DParser_ConsumeUint2(parser);
					} else if ( StrEq( field, sFrameCount ) ) {
						desc.frameCount = (u32)StrToInt(DParser_ConsumeLexeme(parser));
					} else if ( StrEq( field, sFps ) ) {
						desc.fps = (u32)StrToInt(DParser_ConsumeLexeme(parser));
					} else if ( StrEq( field, sLoop ) ) {
						desc.loop = DParser_ConsumeU8(parser);
					} else {
						LOG(Warning, "Unknown Sprite field <%.*s>.\n", field.size, field.str);
						DParser_SkipFieldValue(parser);
					}

					DParser_TryConsume( parser, TOKEN_COMMA );
				}

			// Entity
			} else if ( StrEq(type, sEntityStr) ) {

				const u32 index = descriptors.entityDescCount++;
				if ( countOnly ) goto parse_descriptors_continue;

				EntityDesc &desc = descriptors.entityDescs[index];
				const String name = DParser_ConsumeLexeme( parser );
				desc.name = PushString(*parser.arena, name);
				DParser_TryConsume( parser, TOKEN_EQUAL );
				DParser_TryConsume( parser, TOKEN_LEFT_BRACE );
				while ( !DParser_IsNextToken( parser, TOKEN_RIGHT_BRACE ) )
				{
					DParser_TryConsume( parser, TOKEN_DOT );

					const String field = DParser_ConsumeLexeme( parser );

					DParser_TryConsume( parser, TOKEN_EQUAL );

					if ( !DParser_ConsumeEntityField( parser, field, desc, descriptors.componentPool, index ) ) {
						LOG(Warning, "Unknown Entity field <%.*s>.\n", field.size, field.str);
					}

					DParser_ConsumeUntil( parser, TOKEN_COMMA );
				}

			// Prefab
			} else if ( StrEq(type, sPrefabStr) ) {

				const u32 index = descriptors.prefabDescCount++;
				if ( countOnly ) goto parse_descriptors_continue;

				PrefabDesc &desc = descriptors.prefabDescs[index];
				const String name = DParser_ConsumeLexeme( parser );
				desc.name = PushString(*parser.arena, name);
				DParser_TryConsume( parser, TOKEN_EQUAL );
				DParser_TryConsume( parser, TOKEN_LEFT_BRACE );
				while ( !DParser_IsNextToken( parser, TOKEN_RIGHT_BRACE ) && !DParser_HasFinished( parser ) )
				{
					DParser_TryConsume( parser, TOKEN_DOT );

					const String field = DParser_ConsumeLexeme( parser );

					DParser_TryConsume( parser, TOKEN_EQUAL );

					static const String sId = MakeString("id");
					static const String sEntities = MakeString("entities");

					if ( StrEq( field, sId ) ) {
						desc.id = DParser_ConsumeID(parser);
					} else if ( StrEq( field, sEntities ) ) {
						DParser_ConsumePrefabEntities(parser, desc);
					}

					DParser_TryConsume( parser, TOKEN_COMMA );
				}

			// Room
			} else if ( StrEq(type, sRoomStr) ) {

				const u32 index = descriptors.roomDescCount++;
				if ( countOnly ) goto parse_descriptors_continue;

				RoomDesc &desc = descriptors.roomDescs[index];
				const String name = DParser_ConsumeLexeme( parser );
				desc.name = PushString(*parser.arena, name);
				DParser_TryConsume( parser, TOKEN_EQUAL );
				DParser_TryConsume( parser, TOKEN_LEFT_BRACE );
				while ( !DParser_IsNextToken( parser, TOKEN_RIGHT_BRACE ) && !DParser_HasFinished( parser ) )
				{
					DParser_TryConsume( parser, TOKEN_DOT );

					const String field = DParser_ConsumeLexeme( parser );

					DParser_TryConsume( parser, TOKEN_EQUAL );

					static const String sId = MakeString("id");
					static const String sPos = MakeString("pos");
					static const String sLayers = MakeString("layers");

					if ( StrEq( field, sId ) ) {
						desc.id = DParser_ConsumeID(parser);
					} else if ( StrEq( field, sPos ) ) {
						desc.pos = DParser_ConsumeInt2(parser);
					} else if ( StrEq( field, sLayers ) ) {
						DParser_ConsumeRoomLayers(parser, desc);
					}

					DParser_TryConsume( parser, TOKEN_COMMA );
				}

			// AudioClip
			} else if ( StrEq(type, sAudioClipStr) ) {

				const u32 index = descriptors.audioClipDescCount++;
				if ( countOnly ) goto parse_descriptors_continue;

				AudioClipDesc &desc = descriptors.audioClipDescs[index];
				const String name = DParser_ConsumeLexeme( parser );
				desc.name = PushString(*parser.arena, name);
				DParser_TryConsume( parser, TOKEN_EQUAL );
				DParser_TryConsume( parser, TOKEN_LEFT_BRACE );
				while ( !DParser_IsNextToken( parser, TOKEN_RIGHT_BRACE ) )
				{
					DParser_TryConsume( parser, TOKEN_DOT );

					const String field = DParser_ConsumeLexeme( parser );

					DParser_TryConsume( parser, TOKEN_EQUAL );

					static const String sId = MakeString("id");
					static const String sFilename = MakeString("filename");
					if ( StrEq( field, sId ) ) {
						desc.id = DParser_ConsumeID(parser);
					} else if ( StrEq( field, sFilename ) ) {
						desc.filename = PushString(*parser.arena, DParser_ConsumeString(parser) );
					}

					DParser_TryConsume( parser, TOKEN_COMMA );
				}

			// MusicFile
			} else if ( StrEq(type, sMusicFileStr) ) {

				const u32 index = descriptors.musicFileDescCount++;
				if ( countOnly ) goto parse_descriptors_continue;

				MusicFileDesc &desc = descriptors.musicFileDescs[index];
				const String name = DParser_ConsumeLexeme( parser );
				desc.name = PushString(*parser.arena, name);
				DParser_TryConsume( parser, TOKEN_EQUAL );
				DParser_TryConsume( parser, TOKEN_LEFT_BRACE );
				while ( !DParser_IsNextToken( parser, TOKEN_RIGHT_BRACE ) )
				{
					DParser_TryConsume( parser, TOKEN_DOT );

					const String field = DParser_ConsumeLexeme( parser );

					DParser_TryConsume( parser, TOKEN_EQUAL );

					static const String sId = MakeString("id");
					static const String sFilename = MakeString("filename");
					if ( StrEq( field, sId ) ) {
						desc.id = DParser_ConsumeID(parser);
					} else if ( StrEq( field, sFilename ) ) {
						desc.filename = PushString(*parser.arena, DParser_ConsumeString(parser) );
					}

					DParser_TryConsume( parser, TOKEN_COMMA );
				}

			// Unknown
			} else {
				LOG(Warning, "Unexpected descriptor\n");
			}

			parse_descriptors_continue:

			DParser_ConsumeUntil(parser, TOKEN_SEMICOLON);
		}
		else
		{
			DParser_Consume( parser );
		}
	}
}

AssetDescriptors ParseDescriptors(const char *filepath, Arena &arena)
{
	AssetDescriptors descriptors = {};
	descriptors.sceneDesc.ambientLight = Float3(1.0f); // a file without the field predates it
	DataChunk *chunk = PushFile( arena, filepath );

	if ( chunk )
	{
		DTokenList tokenList = DScan(arena, (const char *)chunk->bytes, chunk->size);
		if ( tokenList.valid )
		{
			DParser parser = DParser_Init(tokenList, arena, descriptors);
			DParseDescriptors(parser, true);

			if (!parser.hasErrors)
			{
				// Reserve memory and reset counts
				descriptors.textureDescs = PushZeroArray(arena, TextureDesc, descriptors.textureDescCount);
				descriptors.textureDescCount = 0;
				descriptors.spriteDescs = PushZeroArray(arena, SpriteDesc, descriptors.spriteDescCount + 1);
				descriptors.spriteDescCount = 0;
				descriptors.materialDescs = PushZeroArray(arena, MaterialDesc, descriptors.materialDescCount);
				descriptors.materialDescCount = 0;
				descriptors.entityDescs = PushZeroArray(arena, EntityDesc, descriptors.entityDescCount);
				// The counting pass does not descend into entity bodies, so the component
				// pools are sized to the worst case an entity could ask for
				descriptors.componentPool.componentCapacity = descriptors.entityDescCount * ComponentType_Count + 1;
				descriptors.componentPool.components = PushZeroArray(arena, ComponentDesc, descriptors.componentPool.componentCapacity);
				descriptors.componentPool.propertyCapacity = descriptors.entityDescCount * MAX_SCRIPT_PROPERTIES + 1;
				descriptors.componentPool.properties = PushZeroArray(arena, ScriptPropertyDesc, descriptors.componentPool.propertyCapacity);
				descriptors.entityDescCount = 0;
				descriptors.prefabDescs = PushZeroArray(arena, PrefabDesc, descriptors.prefabDescCount);
				descriptors.prefabDescCount = 0;
				descriptors.roomDescs = PushZeroArray(arena, RoomDesc, descriptors.roomDescCount);
				descriptors.roomDescCount = 0;
				descriptors.audioClipDescs = PushZeroArray(arena, AudioClipDesc, descriptors.audioClipDescCount);
				descriptors.audioClipDescCount = 0;
				descriptors.musicFileDescs = PushZeroArray(arena, MusicFileDesc, descriptors.musicFileDescCount);
				descriptors.musicFileDescCount = 0;

				parser = DParser_Init(tokenList, arena, descriptors);
				DParseDescriptors(parser, false);

				if ( parser.hasErrors )
				{
					LOG(Error, "Could not parser tokens: %s\n", filepath);
				}
			}
			else
			{
				LOG(Error, "Could not parse tokens for counts: %s\n", filepath);
			}
		}
		else
		{
			LOG(Error, "Could not scan tokens: %s\n", filepath);
		}
	}
	else
	{
		LOG(Warning, "Could not open file: %s\n", filepath);
	}

	return descriptors;
}










////////////////////////////////////////////////////////////////////////
// Binary output

u32 PostIncrement(u32 *value, u32 incr)
{
	ASSERT( *value );
	u32 ret = *value;
	*value += incr;
	return ret;
}

struct DataStringPool
{
	char *str;
	u32 size;
};

static const char *DataInternString( DataStringPool &stringPool, const char *str )
{
	if (!str) { return nullptr; }

	u32 offset = stringPool.size;
	char *dst = stringPool.str + offset;
	while (*str) { *dst++ = *str++; stringPool.size++; }
	*dst = '\0';
	stringPool.size++;
	const char *offsetPtr = (const char*)(uintptr_t)offset;
	return offsetPtr;
}

static const char *DataGetString( const char *stringPool, const char *offsetPtr )
{
	const char *str = offsetPtr ? stringPool + (uintptr_t)offsetPtr : nullptr;
	return str;
}

static void BuildBinEntityDesc(BinEntityDesc &d, const EntityDesc &desc, u32 entityIndex, const ComponentDesc *components, u32 componentCount, DataStringPool &stringPool)
{
	d = {};
	d.id           = desc.id;
	d.name         = DataInternString(stringPool, desc.name);
	d.pos          = desc.pos;
	d.scale        = desc.scale;

	for (u32 i = 0; i < componentCount; ++i)
	{
		const ComponentDesc &component = components[i];
		if (component.entityIndex != entityIndex) {
			continue;
		}

		switch (component.type)
		{
			case ComponentType_Model:
				d.components |= Component_Model;
				d.materialId = component.model.materialId;
				d.geometryType = component.model.geometryType;
				break;

			case ComponentType_Sprite:
				d.components |= Component_Sprite;
				d.spriteId = component.sprite.spriteId;
				d.layerId = component.sprite.layerId;
				break;

			case ComponentType_Light:
				d.components |= Component_Light;
				d.light = component.light;
				break;

			case ComponentType_Particles:
				d.components |= Component_Particles;
				d.particles = component.particles;
				break;

			case ComponentType_Script:
			{
				const ScriptComponentDesc &script = component.script;

				d.components |= Component_Script;

				BinScriptDesc &bs = d.script;
				bs.name = DataInternString(stringPool, script.name);
				bs.propertyCount = Min(script.propertyCount, (u32)ARRAY_COUNT(bs.properties));
				for (u32 p = 0; p < bs.propertyCount; ++p)
				{
					const ScriptPropertyDesc &property = script.properties[p];

					BinScriptPropertyDesc &pd = bs.properties[p];
					pd.name  = DataInternString(stringPool, property.name);
					pd.type  = property.type;
					MemCopy(pd.value, property.value, sizeof(pd.value));
				}
				break;
			}

			default:
				break;
		}
	}
}

void BuildAssets(const AssetDescriptors &descriptors, const char *filepath, Arena tempArena)
{
	LOG(Info, "Build assets: %s\n", filepath);

	CreateDirectory( MakePath(ProjectDir, "build").str );

	FILE *file = fopen(filepath, "wb");
	if ( file )
	{
		u32 offset = sizeof( BinAssetsHeader );

		const u32 sceneOffset = PostIncrement(&offset, sizeof(BinSceneDesc));

		const u32 shaderCount = descriptors.shaderDescCount;
		const u32 shadersSize = shaderCount * sizeof(BinShaderDesc);
		const u32 shadersOffset = PostIncrement(&offset, shadersSize);

		const u32 imageCount = descriptors.textureDescCount;
		const u32 imagesSize = imageCount * sizeof(BinImageDesc);
		const u32 imagesOffset = PostIncrement(&offset, imagesSize);

		const u32 audioClipCount = descriptors.audioClipDescCount;
		const u32 audioClipsSize = audioClipCount * sizeof(BinAudioClipDesc);
		const u32 audioClipsOffset = PostIncrement(&offset, audioClipsSize);

		const u32 musicFileCount = descriptors.musicFileDescCount;
		const u32 musicFilesSize = musicFileCount * sizeof(BinMusicFileDesc);
		const u32 musicFilesOffset = PostIncrement(&offset, musicFilesSize);

		const u32 materialCount = descriptors.materialDescCount;
		const u32 materialsSize = materialCount * sizeof(BinMaterialDesc);
		const u32 materialsOffset = PostIncrement(&offset, materialsSize);

		const u32 spriteCount = descriptors.spriteDescCount;
		const u32 spritesSize = spriteCount * sizeof(BinSpriteDesc);
		const u32 spritesOffset = PostIncrement(&offset, spritesSize);

		const u32 entityCount = descriptors.entityDescCount;
		const u32 entitiesSize = entityCount * sizeof(BinEntityDesc);
		const u32 entitiesOffset = PostIncrement(&offset, entitiesSize);

		const u32 prefabCount = descriptors.prefabDescCount;
		const u32 prefabsSize = prefabCount * sizeof(BinPrefabDesc);
		const u32 prefabsOffset = PostIncrement(&offset, prefabsSize);

		const u32 roomCount = descriptors.roomDescCount;
		const u32 roomsSize = roomCount * sizeof(BinRoomDesc);
		const u32 roomsOffset = PostIncrement(&offset, roomsSize);

		const u32 maxStringPoolSize = KB(128);
		char *stringPoolBase = PushArray(tempArena, char, maxStringPoolSize);
		DataStringPool stringPool = { stringPoolBase, 1 }; // offset 0 is reserved for nullptr

		// Reserve space for asset descs
		BinShaderDesc *binShaderDescs = PushArray(tempArena, BinShaderDesc, shaderCount);
		BinImageDesc *binImageDescs = PushArray(tempArena, BinImageDesc, imageCount);
		BinAudioClipDesc *binAudioClipDescs = PushArray(tempArena, BinAudioClipDesc, audioClipCount);
		BinMusicFileDesc *binMusicFileDescs = PushArray(tempArena, BinMusicFileDesc, musicFileCount);
		BinMaterialDesc *binMaterialDescs = PushArray(tempArena, BinMaterialDesc, materialCount);
		BinSpriteDesc *binSpriteDescs = PushArray(tempArena, BinSpriteDesc, spriteCount);
		BinEntityDesc *binEntityDescs = PushArray(tempArena, BinEntityDesc, entityCount);
		BinPrefabDesc *binPrefabDescs = PushArray(tempArena, BinPrefabDesc, prefabCount);
		BinRoomDesc *binRoomDescs = PushArray(tempArena, BinRoomDesc, roomCount);

		// Prepare asset descs and write asset payloads

		fseek(file, offset, SEEK_SET);

		// Scene


		// Shaders
		for (u32 i = 0; i < shaderCount; ++i)
		{
			const ShaderSourceDesc &desc = descriptors.shaderDescs[i];

			char filepath[MAX_PATH_LENGTH];
			SPrintf(filepath, "%s/shaders/%s.spv", DataDir, desc.name);

			u64 payloadSize = 0;
			GetFileSize(filepath, payloadSize);

			BinShaderDesc &d = binShaderDescs[i];
			d.name       = DataInternString(stringPool, desc.name);
			d.entryPoint = DataInternString(stringPool, desc.entryPoint);
			d.type = desc.type;
			d.location.offset = PostIncrement(&offset, payloadSize);
			d.location.size = U64ToU32(payloadSize);

			Arena scratch = MakeSubArena(tempArena, "Scratch - BuildAssets");
			void *shaderPayload = PushSize(scratch, payloadSize);
			ReadEntireFile(filepath, shaderPayload, payloadSize);

			fwrite(shaderPayload, payloadSize, 1, file);
		}

		// Images
		for (u32 i = 0; i < imageCount; ++i)
		{
			const TextureDesc &desc = descriptors.textureDescs[i];

			const FilePath imagePath = MakePath(AssetDir, desc.filename);

			byte *pixels;
			int texWidth, texHeight, texChannels;
			Arena scratch = MakeSubArena(tempArena, "Scratch - BuildAssets");
			ImagePixels imagePixels = {};
			if ( ReadImagePixels(scratch, imagePath.str, imagePixels) )
			{
				pixels = imagePixels.pixels;
				texWidth = imagePixels.width;
				texHeight = imagePixels.height;
				texChannels = imagePixels.channelCount;
			}
			else
			{
				LOG(Error, "stbi_load failed to load %s\n", imagePath.str);
				static stbi_uc constPixels[] = {255, 0, 255, 255};
				pixels = constPixels;
				texWidth = texHeight = 1;
				texChannels = 4;
			}

			const u64 payloadSize = texWidth * texHeight * texChannels;

			BinImageDesc &d = binImageDescs[i];
			d.id       = desc.id;
			d.name     = DataInternString(stringPool, desc.name);
			d.width    = I32ToU16(texWidth);
			d.height   = I32ToU16(texHeight);
			d.channels = I32ToU8(texChannels);
			d.mipmap   = desc.mipmap;
			d.unused   = 0;
			d.location.offset = PostIncrement(&offset, payloadSize);
			d.location.size   = U64ToU32(payloadSize);

			fwrite(pixels, payloadSize, 1, file);
		}

		// AudioClips
		for (u32 i = 0; i < audioClipCount; ++i)
		{
			const AudioClipDesc &desc = descriptors.audioClipDescs[i];

			const FilePath path = MakePath(AssetDir, desc.filename);

			AudioClip audioClip;
			Arena scratch = MakeSubArena(tempArena, "Scratch - BuildAssets");
			void *samples = nullptr;
			const bool ok = LoadAudioClipFromWAVFile(path.str, scratch, audioClip, &samples);

			const u64 payloadSize = audioClip.sampleCount * audioClip.sampleSize;

			const BinAudioClipDesc d = {
				.id = desc.id,
				.sampleCount = audioClip.sampleCount,
				.samplingRate = audioClip.samplingRate,
				.sampleSize = audioClip.sampleSize,
				.channelCount = audioClip.channelCount,
				.location = {
					.offset = PostIncrement(&offset, payloadSize),
					.size = U64ToU32(payloadSize),
				},
			};
			binAudioClipDescs[i] = d;

			if ( ok ) {
				fwrite(samples, payloadSize, 1, file);
			} else {
				fseek(file, payloadSize, SEEK_CUR);
			}
		}

		// MusicFiles
		for (u32 i = 0; i < musicFileCount; ++i)
		{
			const MusicFileDesc &desc = descriptors.musicFileDescs[i];

			const FilePath path = MakePath(AssetDir, desc.filename);

			Arena scratch = MakeSubArena(tempArena, "Scratch - BuildAssets");
			DataChunk *fileChunk = PushFile(scratch, path.str);
			if ( !fileChunk ) {
				continue;
			}

			const u64 payloadSize = fileChunk->size;

			BinMusicFileDesc &d = binMusicFileDescs[i];
			d.id              = desc.id;
			d.name            = DataInternString(stringPool, desc.name);
			d.location.offset = PostIncrement(&offset, payloadSize);
			d.location.size   = U64ToU32(payloadSize);

			fwrite(fileChunk->bytes, payloadSize, 1, file);
		}

		// Materials
		for (u32 i = 0; i < materialCount; ++i)
		{
			const MaterialDesc &desc = descriptors.materialDescs[i];

			BinMaterialDesc &d = binMaterialDescs[i];
			d.id           = desc.id;
			d.name         = DataInternString(stringPool, desc.name);
			d.textureId    = desc.textureId;
			d.pipelineName = DataInternString(stringPool, desc.pipelineName);
			d.uvScale = desc.uvScale;
		}

		// Sprites
		for (u32 i = 0; i < spriteCount; ++i)
		{
			const SpriteDesc &desc = descriptors.spriteDescs[i];

			BinSpriteDesc &d = binSpriteDescs[i];
			d.id          = desc.id;
			d.name        = DataInternString(stringPool, desc.name);
			d.textureId   = desc.textureId;
			d.pos  = desc.pos;
			d.size = desc.size;
			d.frameCount = desc.frameCount;
			d.fps        = desc.fps;
			d.loop       = desc.loop;
			d._pad[0] = d._pad[1] = d._pad[2] = 0;
		}

		// Entities
		for (u32 i = 0; i < entityCount; ++i)
		{
			BuildBinEntityDesc(binEntityDescs[i], descriptors.entityDescs[i], i,
					descriptors.componentPool.components, descriptors.componentPool.componentCount, stringPool);
		}

		// Prefabs
		for (u32 i = 0; i < prefabCount; ++i)
		{
			const PrefabDesc &desc = descriptors.prefabDescs[i];

			BinPrefabDesc &d = binPrefabDescs[i];
			d = {};
			d.id = desc.id;
			d.name = DataInternString(stringPool, desc.name);
			d.entityCount = desc.entityCount;

			for (u32 e = 0; e < desc.entityCount; ++e)
			{
				BuildBinEntityDesc(d.entities[e], desc.entities[e], e, desc.components, desc.componentCount, stringPool);
			}
		}

		// Rooms (tile payloads continue after the last written payload)
		for (u32 i = 0; i < roomCount; ++i)
		{
			const RoomDesc &desc = descriptors.roomDescs[i];

			BinRoomDesc &d = binRoomDescs[i];
			d = {};
			d.id         = desc.id;
			d.name       = DataInternString(stringPool, desc.name);
			d.pos        = desc.pos;
			d.layerCount = desc.layerCount;

			for (u32 l = 0; l < desc.layerCount; ++l)
			{
				const LayerDesc &layer = desc.layers[l];
				const u64 payloadSize = layer.tileCount * sizeof(TileDesc);

				BinLayerDesc &ld = d.layers[l];
				ld.id           = layer.id;
				ld.name         = DataInternString(stringPool, layer.name);
				ld.isBase       = layer.isBase ? 1 : 0;
				ld.visible      = layer.visible ? 1 : 0;
				ld.isCollider   = layer.isCollider ? 1 : 0;
				ld.size         = layer.size;
				ld.tiles.offset = PostIncrement(&offset, payloadSize);
				ld.tiles.size   = U64ToU32(payloadSize);

				if ( payloadSize > 0 ) {
					fwrite(layer.tiles, payloadSize, 1, file);
				}
			}
		}

		// Write string pool after payloads
		const u32 stringPoolOffset = offset;
		const u32 stringPoolSize   = stringPool.size;
		fwrite(stringPool.str, stringPoolSize, 1, file);

		// Write asset descs
		fseek(file, sceneOffset, SEEK_SET);
		const BinSceneDesc binSceneDesc = {
			.projectionType = descriptors.sceneDesc.projectionType,
			.ambientLight = descriptors.sceneDesc.ambientLight,
		};
		fwrite(&binSceneDesc,     sizeof(binSceneDesc),         1,              file);
		fwrite(binShaderDescs,    sizeof(binShaderDescs[0]),    shaderCount,    file);
		fwrite(binImageDescs,     sizeof(binImageDescs[0]),     imageCount,     file);
		fwrite(binAudioClipDescs, sizeof(binAudioClipDescs[0]), audioClipCount, file);
		fwrite(binMusicFileDescs, sizeof(binMusicFileDescs[0]), musicFileCount, file);
		fwrite(binMaterialDescs,  sizeof(binMaterialDescs[0]),  materialCount,  file);
		fwrite(binSpriteDescs,    sizeof(binSpriteDescs[0]),    spriteCount,    file);
		fwrite(binEntityDescs,    sizeof(binEntityDescs[0]),    entityCount,    file);
		fwrite(binPrefabDescs,    sizeof(binPrefabDescs[0]),    prefabCount,    file);
		fwrite(binRoomDescs,      sizeof(binRoomDescs[0]),      roomCount,      file);

		// Write file header last (string pool offset is now known)
		const BinAssetsHeader fileHeader = {
			.magicNumber      = U32FromChars('I', 'R', 'I', 'S'),
			.version          = BinAssetsVersion,
			.sceneOffset      = sceneOffset,
			.shadersOffset    = shadersOffset,
			.shaderCount      = shaderCount,
			.imagesOffset     = imagesOffset,
			.imageCount       = imageCount,
			.audioClipsOffset = audioClipsOffset,
			.audioClipCount   = audioClipCount,
			.musicFilesOffset = musicFilesOffset,
			.musicFileCount   = musicFileCount,
			.materialsOffset  = materialsOffset,
			.materialCount    = materialCount,
			.spritesOffset    = spritesOffset,
			.spriteCount      = spriteCount,
			.entitiesOffset   = entitiesOffset,
			.entityCount      = entityCount,
			.prefabsOffset    = prefabsOffset,
			.prefabCount      = prefabCount,
			.roomsOffset      = roomsOffset,
			.roomCount        = roomCount,
			.stringPoolOffset = stringPoolOffset,
			.stringPoolSize   = stringPoolSize,
		};
		fseek(file, 0, SEEK_SET);
		fwrite(&fileHeader, sizeof(fileHeader), 1, file);

		fclose(file);
	}
}

#endif // USE_DATA_BUILD










////////////////////////////////////////////////////////////////////////
// Binary loading

static void ResolveBinEntityStrings(BinEntityDesc &d, const char *stringPool)
{
	d.name = DataGetString(stringPool, d.name);

	if (d.components & Component_Script)
	{
		BinScriptDesc &bs = d.script;
		bs.name = DataGetString(stringPool, bs.name);
		for (u32 p = 0; p < bs.propertyCount && p < ARRAY_COUNT(bs.properties); ++p) {
			bs.properties[p].name = DataGetString(stringPool, bs.properties[p].name);
		}
	}
}

BinAssets OpenAssets(Arena &dataArena, const char *filepath)
{
	BinAssets assets = {};

	File file = OpenFile( filepath, FileModeRead );
	if ( !file.isOpen ) {
		LOG( Error, "Could not open file %s\n", filepath );
		QUIT_ABNORMALLY();
	}

	if ( !ReadFromFile( file, &assets.header, sizeof(BinAssetsHeader) ) )
	{
		LOG( Error, "Could not read file header from file %s\n", filepath );
		QUIT_ABNORMALLY();
	}

	if (assets.header.magicNumber != U32FromChars('I', 'R', 'I', 'S'))
	{
		LOG( Error, "Wrong magic number in file %s\n", filepath );
		QUIT_ABNORMALLY();
	}

	if (assets.header.version != BinAssetsVersion)
	{
		LOG( Error, "Wrong version (%u, expected %u) in file %s. Rebuild the data files.\n", assets.header.version, BinAssetsVersion, filepath );
		QUIT_ABNORMALLY();
	}

	assets.shaders = PushArray(dataArena, BinShader, assets.header.shaderCount);
	assets.images = PushArray(dataArena, BinImage, assets.header.imageCount);
	assets.audioClips = PushArray(dataArena, BinAudioClip, assets.header.audioClipCount);
	assets.musicFiles = PushArray(dataArena, BinMusicFile, assets.header.musicFileCount);
	assets.materials = PushArray(dataArena, BinMaterial, assets.header.materialCount);
	assets.sprites = PushArray(dataArena, BinSprite, assets.header.spriteCount + 1);
	assets.entities = PushArray(dataArena, BinEntity, assets.header.entityCount);
	assets.prefabs = PushArray(dataArena, BinPrefab, assets.header.prefabCount);
	assets.rooms = PushZeroArray(dataArena, BinRoom, assets.header.roomCount);

	const char *stringPool = (const char*)PushDataFromFile(
		dataArena, file, assets.header.stringPoolOffset, assets.header.stringPoolSize);

	// Scene
	BinSceneDesc *binSceneDesc = (BinSceneDesc*)PushDataFromFile(
		dataArena, file, assets.header.sceneOffset, sizeof(BinSceneDesc));
	assets.scene.projectionType = binSceneDesc->projectionType;
	assets.scene.ambientLight = binSceneDesc->ambientLight;

	// Shaders
	BinShaderDesc *binShaderDescs = (BinShaderDesc*)PushDataFromFile(
		dataArena, file, assets.header.shadersOffset, assets.header.shaderCount * sizeof(BinShaderDesc));
	for (u32 i = 0; i < assets.header.shaderCount; ++i)
	{
		BinShaderDesc &d = binShaderDescs[i];
		d.name       = DataGetString( stringPool, d.name );
		d.entryPoint = DataGetString( stringPool, d.entryPoint );
		assets.shaders[i].desc  = &d;
		assets.shaders[i].spirv = PushDataFromFile(dataArena, file, d.location.offset, d.location.size);
	}

	// Images
	BinImageDesc *binImageDescs = (BinImageDesc*)PushDataFromFile(
		dataArena, file, assets.header.imagesOffset, assets.header.imageCount * sizeof(BinImageDesc));
	for (u32 i = 0; i < assets.header.imageCount; ++i)
	{
		BinImageDesc &d = binImageDescs[i];
		d.name = DataGetString( stringPool, d.name );
		assets.images[i].desc   = &d;
		assets.images[i].pixels = PushDataFromFile(dataArena, file, d.location.offset, d.location.size);
	}

	// AudioClips
	BinAudioClipDesc *binAudioClipDescs = (BinAudioClipDesc*)PushDataFromFile(
		dataArena, file, assets.header.audioClipsOffset, assets.header.audioClipCount * sizeof(BinAudioClipDesc));
	for (u32 i = 0; i < assets.header.audioClipCount; ++i)
	{
		assets.audioClips[i].desc = binAudioClipDescs + i;
	}

	// MusicFiles
	BinMusicFileDesc *binMusicFileDescs = (BinMusicFileDesc*)PushDataFromFile(
		dataArena, file, assets.header.musicFilesOffset, assets.header.musicFileCount * sizeof(BinMusicFileDesc));
	for (u32 i = 0; i < assets.header.musicFileCount; ++i)
	{
		BinMusicFileDesc &d = binMusicFileDescs[i];
		d.name = DataGetString( stringPool, d.name );
		assets.musicFiles[i].desc = &d;
	}

	// Materials
	BinMaterialDesc *materialDescs = (BinMaterialDesc*)PushDataFromFile(
		dataArena, file, assets.header.materialsOffset, assets.header.materialCount * sizeof(BinMaterialDesc));
	for (u32 i = 0; i < assets.header.materialCount; ++i)
	{
		BinMaterialDesc &d = materialDescs[i];
		d.name         = DataGetString( stringPool, d.name );
		d.pipelineName = DataGetString( stringPool, d.pipelineName );
		assets.materials[i].desc = &d;
	}

	// Sprites
	if (assets.header.spriteCount > 0)
	{
		BinSpriteDesc *spriteDescs = (BinSpriteDesc*)PushDataFromFile(
			dataArena, file, assets.header.spritesOffset, assets.header.spriteCount * sizeof(BinSpriteDesc));
		for (u32 i = 0; i < assets.header.spriteCount; ++i)
		{
			BinSpriteDesc &d = spriteDescs[i];
			d.name        = DataGetString(stringPool, d.name);
			assets.sprites[i].desc = &d;
		}
	}

	// Entities
	BinEntityDesc *entityDescs = (BinEntityDesc*)PushDataFromFile(
		dataArena, file, assets.header.entitiesOffset, assets.header.entityCount * sizeof(BinEntityDesc));
	for (u32 i = 0; i < assets.header.entityCount; ++i)
	{
		BinEntityDesc &d = entityDescs[i];
		ResolveBinEntityStrings(d, stringPool);
		assets.entities[i].desc = &d;
	}

	// Prefabs
	if (assets.header.prefabCount > 0)
	{
		BinPrefabDesc *prefabDescs = (BinPrefabDesc*)PushDataFromFile(
			dataArena, file, assets.header.prefabsOffset, assets.header.prefabCount * sizeof(BinPrefabDesc));
		for (u32 i = 0; i < assets.header.prefabCount; ++i)
		{
			BinPrefabDesc &d = prefabDescs[i];
			d.name = DataGetString(stringPool, d.name);
			for (u32 e = 0; e < d.entityCount && e < ARRAY_COUNT(d.entities); ++e)
			{
				ResolveBinEntityStrings(d.entities[e], stringPool);
			}
			assets.prefabs[i].desc = &d;
		}
	}

	// Rooms
	if (assets.header.roomCount > 0)
	{
		BinRoomDesc *roomDescs = (BinRoomDesc*)PushDataFromFile(
			dataArena, file, assets.header.roomsOffset, assets.header.roomCount * sizeof(BinRoomDesc));
		for (u32 i = 0; i < assets.header.roomCount; ++i)
		{
			BinRoomDesc &d = roomDescs[i];
			d.name = DataGetString(stringPool, d.name);
			assets.rooms[i].desc = &d;
			for (u32 l = 0; l < d.layerCount && l < ARRAY_COUNT(d.layers); ++l)
			{
				BinLayerDesc &ld = d.layers[l];
				ld.name = DataGetString(stringPool, ld.name);
				if (ld.tiles.size > 0)
				{
					assets.rooms[i].tiles[l] = (TileDesc*)PushDataFromFile(
						dataArena, file, ld.tiles.offset, ld.tiles.size);
				}
			}
		}
	}

	assets.file = file;

	return assets;
}

void CloseAssets(BinAssets &assets)
{
	if ( assets.file.isOpen )
	{
		CloseFile(assets.file);
	}
}


////////////////////////////////////////////////////////////////////////
// Asset descriptors and scene serialization

#if USE_DATA_BUILD
static AssetDescriptors GetAssetDescriptors(Engine &engine, Arena &arena)
{
	static TextureDesc textureDescs[MAX_TEXTURES];
	u32 textureCount = 0;
	for (u16 i = 0; i < engine.gfx.textureCount; ++i) {
		const Texture &texture = engine.gfx.textures[i];
		if ( !texture.desc.id ) { continue; }
		textureDescs[textureCount] = texture.desc;
		if ( !( textureDescs[textureCount].flags & AssetFlag_Ghost ) ) {
			textureCount++;
		}
	}

	static SpriteDesc spriteDescs[MAX_SPRITES];
	u32 spriteCount = 0;
	for (u16 i = 0; i < engine.scene.spriteCount; ++i) {
		const Sprite &sprite = engine.scene.sprites[i];
		if ( !sprite.desc.id ) { continue; }
		spriteDescs[spriteCount++] = sprite.desc;
	}

	static MaterialDesc materialDescs[MAX_MATERIALS];
	u32 materialCount = 0;
	for (u16 i = 0; i < engine.gfx.materialCount; ++i) {
		const Material &material = engine.gfx.materials[i];
		if ( !material.desc.id ) { continue; }
		materialDescs[materialCount] = material.desc;
		if ( !( materialDescs[materialCount].flags & AssetFlag_Ghost ) ) {
			materialCount++;
		}
	}

	static EntityDesc entityDescs[MAX_ENTITIES];
	static ComponentDesc componentDescs[MAX_ENTITIES * ComponentType_Count];
	static ScriptPropertyDesc scriptPropertyDescs[MAX_ENTITIES * MAX_SCRIPT_PROPERTIES];
	ComponentDescPool componentPool = {
		.components = componentDescs,
		.componentCapacity = ARRAY_COUNT(componentDescs),
		.properties = scriptPropertyDescs,
		.propertyCapacity = ARRAY_COUNT(scriptPropertyDescs),
	};

	u32 entityCount = 0;
	for (u16 i = 0; i < engine.scene.entityCount; ++i) {
		const Entity &entity = engine.scene.entities[i];
		if ( !entity.id ) { continue; }
		const u32 entityIndex = entityCount++;
		EntityDesc &desc = entityDescs[entityIndex];
		desc = GetEntityDesc(engine, entity.id);
		GatherEntityComponentDescs(engine, entity.id, entityIndex, componentPool);
	}

	static PrefabDesc prefabDescs[MAX_PREFABS];
	u32 prefabCount = 0;
	for (u32 i = 0; i < engine.scene.prefabCount; ++i) {
		const Prefab &prefab = engine.scene.prefabs[i];
		if ( !prefab.id ) { continue; }
		PrefabDesc &desc = prefabDescs[prefabCount++];
		desc.id = prefab.id;
		desc.name = prefab.name;
		desc.entityCount = prefab.entityCount;
		for (u32 e = 0; e < prefab.entityCount; ++e) {
			desc.entities[e] = prefab.entities[e];
		}
		desc.components = prefab.components;
		desc.componentCount = prefab.componentCount;
	}

	static RoomDesc roomDescs[MAX_ROOMS];
	u32 roomCount = 0;
	for (u16 roomIndex = 0; roomIndex < engine.scene.roomCount; ++roomIndex) {
		const Room &room = engine.scene.rooms[roomIndex];
		if ( !room.id ) { continue; }
		RoomDesc &desc = roomDescs[roomCount++];
		desc = {};
		desc.id = room.id;
		desc.name = room.name;
		desc.pos = room.pos;
		for (u32 l = 0; l < ARRAY_COUNT(room.layers); ++l) {
			const Layer &layer = room.layers[l];
			if (!layer.initialized) {
				continue;
			}
			LayerDesc &layerDesc = desc.layers[desc.layerCount++];
			layerDesc.id = layer.id;
			layerDesc.name = layer.name;
			layerDesc.isBase = layer.isBase;
			layerDesc.visible = layer.visible;
			layerDesc.isCollider = layer.isCollider;
			layerDesc.size = layer.size;
			layerDesc.tiles = (TileDesc*)(arena.base + arena.used);
			layerDesc.tileCount = 0;
			if (layer.isCollider)
			{
				for (u32 x = 0; x < layer.size.x; ++x) {
					for (u32 y = 0; y < layer.size.y; ++y) {
						const u32 collider = layer.cells[x][y].collider;
						if (collider == 0) continue;
						TileDesc &tile = *PushStruct(arena, TileDesc);
						tile.x = (u16)x;
						tile.y = (u16)y;
						tile.collider = collider;
						layerDesc.tileCount++;
					}
				}
			}
			else
			{
				// Sprites
				for (u32 x = 0; x < layer.size.x; ++x) {
					for (u32 y = 0; y < layer.size.y; ++y) {
						const ID spriteId = layer.cells[x][y].spriteId;
						if (!spriteId) continue;
						TileDesc &tile = *PushStruct(arena, TileDesc);
						tile.x = (u16)x;
						tile.y = (u16)y;
						tile.spriteId = spriteId;
						layerDesc.tileCount++;
					}
				}
			}
		}
	}

	static AudioClipDesc audioClipDescs[MAX_AUDIO_CLIPS];
	u32 audioClipCount = 0;
	for (u16 i = 0; i < engine.audio.clipCount; ++i) {
		const AudioClipDesc &desc = engine.audio.clips[i].desc;
		if ( !desc.id ) { continue; }
		audioClipDescs[audioClipCount] = desc;
		if ( !( desc.flags & AssetFlag_Ghost ) ) {
			audioClipCount++;
		}
	}

	static MusicFileDesc musicFileDescs[MAX_MUSIC_FILES];
	u32 musicFileCount = 0;
	for (u16 i = 0; i < engine.audio.musicFileCount; ++i) {
		const MusicFileDesc &desc = engine.audio.musicFiles[i].desc;
		if ( !desc.id ) { continue; }
		musicFileDescs[musicFileCount] = desc;
		if ( !( desc.flags & AssetFlag_Ghost ) ) {
			musicFileCount++;
		}
	}

	const SceneDesc sceneDesc = {
		.projectionType = engine.scene.projectionType,
		.ambientLight = engine.scene.ambientLight,
	};

	const AssetDescriptors assetDescs = {
		.sceneDesc = sceneDesc,
		.shaderDescs = nullptr, // shaderSourceDescs,
		.shaderDescCount = 0, //ARRAY_COUNT(shaderSourceDescs),
		.textureDescs = textureDescs,
		.textureDescCount = textureCount,
		.spriteDescs = spriteDescs,
		.spriteDescCount = spriteCount,
		.materialDescs = materialDescs,
		.materialDescCount = materialCount,
		.entityDescs = entityDescs,
		.entityDescCount = entityCount,
		.componentPool = componentPool,
		.prefabDescs = prefabDescs,
		.prefabDescCount = prefabCount,
		.roomDescs = roomDescs,
		.roomDescCount = roomCount,
		.audioClipDescs = audioClipDescs,
		.audioClipDescCount = audioClipCount,
		.musicFileDescs = musicFileDescs,
		.musicFileDescCount = musicFileCount,
	};

	return assetDescs;
}
#endif // USE_DATA_BUILD

// The data arena holds whatever the current scene was loaded from, so a load pushes the
// mark its assets start at and CleanScene pops back to it.
bool PushDataArenaState(Engine &engine)
{
	const bool ok = engine.dataArenaStateCount < ARRAY_COUNT(engine.dataArenaStates);
	if (ok)
	{
		engine.dataArenaStates[engine.dataArenaStateCount++] = DataArena;
	}
	return ok;
}

bool PopDataArenaState(Engine &engine)
{
	const bool ok = engine.dataArenaStateCount > 0;
	if (ok)
	{
		DataArena = engine.dataArenaStates[--engine.dataArenaStateCount];
	}
	return ok;
}


#if USE_DATA_BUILD
void LoadSceneFromTxt(Engine &engine, const char *filepath)
{
	if ( PushDataArenaState(engine) )
	{
		Arena &dataArena = DataArena;
		AssetDescriptors assetDescriptors = ParseDescriptors(filepath, dataArena);

		engine.scene.projectionType = assetDescriptors.sceneDesc.projectionType;
		engine.scene.ambientLight = assetDescriptors.sceneDesc.ambientLight;

		// Textures
		for (u32 i = 0; i < assetDescriptors.textureDescCount; ++i)
		{
			CreateTexture(engine.gfx, assetDescriptors.textureDescs[i]);
		}

		// Materials
		for (u32 i = 0; i < assetDescriptors.materialDescCount; ++i)
		{
			CreateMaterial(engine.gfx, assetDescriptors.materialDescs[i]);
		}

		// Sprites (must be before entities and rooms, which refer to them by ID)
		for (u32 i = 0; i < assetDescriptors.spriteDescCount; ++i)
		{
			CreateSprite(engine, assetDescriptors.spriteDescs[i]);
		}

		// Entities
		for (u32 i = 0; i < assetDescriptors.entityDescCount; ++i)
		{
			const ID entityId = CreateEntity(engine, assetDescriptors.entityDescs[i]);

			const ComponentDescPool &pool = assetDescriptors.componentPool;
			for (u32 c = 0; c < pool.componentCount; ++c)
			{
				if ( pool.components[c].entityIndex == i ) {
					AddComponent(engine, entityId, pool.components[c]);
				}
			}
		}

		// Prefabs
		for (u32 i = 0; i < assetDescriptors.prefabDescCount; ++i)
		{
			CreatePrefab(engine, assetDescriptors.prefabDescs[i]);
		}

		// Rooms
		if (assetDescriptors.roomDescCount > 0)
		{
			for (u32 i = 0; i < assetDescriptors.roomDescCount; ++i)
			{
				CreateRoom(engine, assetDescriptors.roomDescs[i]);
			}
		}

		// Audio clips
		for (u32 i = 0; i < assetDescriptors.audioClipDescCount; ++i)
		{
			CreateAudioClip(engine.audio, assetDescriptors.audioClipDescs[i]);
		}

		// Music files
		for (u32 i = 0; i < assetDescriptors.musicFileDescCount; ++i)
		{
			CreateMusicFile(engine.audio, assetDescriptors.musicFileDescs[i]);
		}

		UploadMaterialData(engine.gfx);
	}
}

void SaveSceneToTxt(Engine &engine, const char *filepath)
{
	Scratch scratch(MB(16)); // holds the tile lists of all rooms
	const AssetDescriptors assetDescs = GetAssetDescriptors(engine, scratch.arena);

	SaveAssetDescriptors(filepath, assetDescs);
}
#endif // USE_DATA_BUILD

void LoadShadersFromBin(Engine &engine)
{
	const FilePath filepath = MakePath(DataDir, "shaders.dat");
	engine.shaderAssets = OpenAssets(DataArena, filepath.str);
}

void LoadSceneFromBin(Engine &engine)
{
	if (PushDataArenaState(engine))
	{
		const FilePath filepath = MakePath(DataDir, "assets.dat");
		engine.assets = OpenAssets(DataArena, filepath.str);

		engine.scene.projectionType = engine.assets.scene.projectionType;
		engine.scene.ambientLight = engine.assets.scene.ambientLight;

		// Textures
		for (u32 i = 0; i < engine.assets.header.imageCount; ++i)
		{
			CreateTexture(engine.gfx, engine.assets.images[i]);
		}

		// Materials
		for (u32 i = 0; i < engine.assets.header.materialCount; ++i)
		{
			CreateMaterial(engine.gfx, *engine.assets.materials[i].desc);
		}

		// Sprites (must be before entities and rooms, which refer to them by ID)
		for (u32 i = 0; i < engine.assets.header.spriteCount; ++i)
		{
			CreateSprite(engine, *engine.assets.sprites[i].desc);
		}

		// Entities
		for (u32 i = 0; i < engine.assets.header.entityCount; ++i)
		{
			CreateEntity(engine, *engine.assets.entities[i].desc);
		}

		// Prefabs
		for (u32 i = 0; i < engine.assets.header.prefabCount; ++i)
		{
			CreatePrefab(engine, *engine.assets.prefabs[i].desc);
		}

		// Rooms
		if (engine.assets.header.roomCount > 0)
		{
			for (u32 i = 0; i < engine.assets.header.roomCount; ++i)
			{
				CreateRoom(engine, engine.assets.rooms[i]);
			}
		}

		// Audio clips
		for (u32 i = 0; i < engine.assets.header.audioClipCount; ++i)
		{
			CreateAudioClip(engine.audio, engine.assets.audioClips[i]);
		}

		// Music files
		for (u32 i = 0; i < engine.assets.header.musicFileCount; ++i)
		{
			CreateMusicFile(engine.audio, engine.assets.musicFiles[i]);
		}

		UploadMaterialData(engine.gfx);
	}
}


#if USE_DATA_BUILD
void BuildShaders(Engine &engine, const char *outBinFilepath)
{
	CompileShaders();

	Arena scratch = MakeSubArena(DataArena, "Scratch - BuildShaders");
	AssetDescriptors assetDescriptors = {};
	assetDescriptors.shaderDescs = GetShaderSourceDescs();
	assetDescriptors.shaderDescCount = GetShaderSourceDescCount();
	BuildAssets(assetDescriptors, outBinFilepath, scratch);
}

void BuildAssetsFromTxt(Engine &engine, const char *inTxtFilepath, const char *outBinFilepath)
{
	Arena scratch = MakeSubArena(DataArena, "Scratch - BuildAssetsFromTxt");
	AssetDescriptors assetDescriptors = ParseDescriptors(inTxtFilepath, scratch);
	BuildAssets(assetDescriptors, outBinFilepath, scratch);
}
#endif // USE_DATA_BUILD
