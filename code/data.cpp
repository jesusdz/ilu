
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

static void PushIndent(WriteContext &ctx)
{
	ctx.indent++;
}

static void PopIndent(WriteContext &ctx)
{
	ASSERT(ctx.indent > 0);
	ctx.indent--;
}

static const char *GeometryTypeToString(GeometryType type)
{
	switch (type)
	{
		case GeometryTypeCube: return "GeometryTypeCube";
		case GeometryTypePlane: return "GeometryTypePlane";
		case GeometryTypeQuad: return "GeometryTypeQuad";
		case GeometryTypeScreen: return "GeometryTypeScreen";
		case GeometryTypeSprite: return "GeometryTypeSprite";
		default:;
	}
	return "<unknown>";
}

static GeometryType StrToGeometryType(String str)
{
	static const String sGeometryTypeCube = MakeString("GeometryTypeCube");
	static const String sGeometryTypePlane = MakeString("GeometryTypePlane");
	static const String sGeometryTypeQuad = MakeString("GeometryTypeQuad");
	static const String sGeometryTypeScreen = MakeString("GeometryTypeScreen");
	static const String sGeometryTypeSprite = MakeString("GeometryTypeSprite");
	if ( StrEq(str, sGeometryTypeCube) ) return GeometryTypeCube;
	else if ( StrEq(str, sGeometryTypePlane) ) return GeometryTypePlane;
	else if ( StrEq(str, sGeometryTypeScreen) ) return GeometryTypeScreen;
	else if ( StrEq(str, sGeometryTypeSprite) ) return GeometryTypeSprite;
	return GeometryTypeCube;
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
		if (desc.spriteId.slot != 0) {
			WriteLine(ctx, ".spriteId = %u,", desc.spriteId.slot);
		} else if (desc.materialId.slot != 0) {
			WriteLine(ctx, ".materialId = %u,", desc.materialId.slot);
			WriteLine(ctx, ".geometryType = %s,", GeometryTypeToString(desc.geometryType));
		}
		WriteLine(ctx, ".pos = {%f, %f, %f},", desc.pos.x, desc.pos.y, desc.pos.z);
		WriteLine(ctx, ".scale = %f,", desc.scale);
		if (desc.spriteId.slot != 0) {
			WriteLine(ctx, ".layerId = %u,", desc.layerId.slot);
		}
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

	// Scripts are last because they can have references to previously declared objects
	WriteSectionLine(ctx, "Scripts");

	for (u32 i = 0; i < assets.scriptDescCount; ++i)
	{
		const ScriptDesc &desc = assets.scriptDescs[i];

		// The identifier is the script type, the way an Entity's is its name
		WriteLine(ctx, "Script %s = {", desc.name);

		PushIndent(ctx);
		WriteLine(ctx, ".entity = %u,", desc.entity.slot);

		if ( desc.propertyCount > 0 )
		{
			WriteLine(ctx, ".properties = {");
			PushIndent(ctx);
			for (u32 p = 0; p < desc.propertyCount; ++p)
			{
				const ScriptPropertyDesc &propDesc = desc.properties[p];
				const char *typeStr = PropertyTypeToString(propDesc.value.type);
				WriteLine(ctx, "{\"%s\", %s, %u},", propDesc.name, typeStr, propDesc.value.uValue);
			}
			PopIndent(ctx);
			WriteLine(ctx, "},");
		}

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

static GeometryType DParser_ConsumeGeometryType( DParser &parser )
{
	const String strGeometryType = DParser_ConsumeLexeme(parser);
	const GeometryType res = StrToGeometryType(strGeometryType);
	return res;
}

static PropertyType DParser_ConsumePropertyType( DParser &parser )
{
	const String strPropertyType = DParser_ConsumeLexeme(parser);
	const PropertyType res = StringToPropertyType(strPropertyType);
	if (res == ReflexID_Null) {
		LOG(Warning, "DParser_ConsumeGeometryType: unrecognized geometry type %.*s\n", strPropertyType.size, strPropertyType.str);
	}
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

static void DParser_ConsumeScriptProperties( DParser &parser, ScriptDesc &script)
{
	DParser_TryConsume(parser, TOKEN_LEFT_BRACE);
	script.propertyCount = 0;

	while ( DParser_TryConsume(parser, TOKEN_LEFT_BRACE) && !DParser_HasFinished(parser) )
	{
		ScriptPropertyDesc propertyDesc = {};
		propertyDesc.name = PushString(*parser.arena, DParser_ConsumeString(parser));
		DParser_TryConsume(parser, TOKEN_COMMA);
		propertyDesc.value.type = DParser_ConsumePropertyType(parser);
		DParser_TryConsume(parser, TOKEN_COMMA);
		propertyDesc.value.uValue = DParser_ConsumeU32(parser);
		DParser_TryConsume(parser, TOKEN_RIGHT_BRACE);
		DParser_TryConsume(parser, TOKEN_COMMA);

		if ( script.propertyCount < MAX_SCRIPT_PROPERTIES ) {
			script.properties[script.propertyCount++] = propertyDesc;
		}
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
static const String sRoomStr = MakeString("Room");
static const String sAudioClipStr = MakeString("AudioClip");
static const String sMusicFileStr = MakeString("MusicFile");
static const String sScriptStr = MakeString("Script");

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
					if ( StrEq( field, sProjectionType ) ) {
						const char *cstr = StringToCStr( DParser_ConsumeString(parser) );
						desc.projectionType = StrToProjectionType( cstr );
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

					static const String sId = MakeString("id");
					static const String sMaterialId = MakeString("materialId");
					static const String sSpriteId = MakeString("spriteId");
					static const String sPos = MakeString("pos");
					static const String sScale = MakeString("scale");
					static const String sLayerId = MakeString("layerId");
					static const String sGeometryType = MakeString("geometryType");

					if ( StrEq( field, sId ) ) {
						desc.id = DParser_ConsumeID(parser);
					} else if ( StrEq( field, sMaterialId ) ) {
						desc.materialId = DParser_ConsumeID(parser);
					} else if ( StrEq( field, sSpriteId ) ) {
						desc.spriteId = DParser_ConsumeID(parser);
					} else if ( StrEq( field, sPos ) ) {
						desc.pos = DParser_ConsumeFloat3(parser);
					} else if ( StrEq( field, sScale ) ) {
						desc.scale = DParser_ConsumeF32(parser);
					} else if ( StrEq( field, sLayerId ) ) {
						desc.layerId = DParser_ConsumeID(parser);
					} else if ( StrEq( field, sGeometryType ) ) {
						desc.geometryType = DParser_ConsumeGeometryType(parser);
					}

					DParser_ConsumeUntil( parser, TOKEN_COMMA );
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

			// Script
			} else if ( StrEq(type, sScriptStr) ) {

				const u32 index = descriptors.scriptDescCount++;
				if ( countOnly ) goto parse_descriptors_continue;

				ScriptDesc &desc = descriptors.scriptDescs[index];
				const String name = DParser_ConsumeLexeme( parser );
				desc.name = PushString(*parser.arena, name);
				DParser_TryConsume( parser, TOKEN_EQUAL );
				DParser_TryConsume( parser, TOKEN_LEFT_BRACE );
				while ( !DParser_IsNextToken( parser, TOKEN_RIGHT_BRACE ) )
				{
					DParser_TryConsume( parser, TOKEN_DOT );

					const String field = DParser_ConsumeLexeme( parser );

					DParser_TryConsume( parser, TOKEN_EQUAL );

					static const String sEntity = MakeString("entity");
					static const String sProperties = MakeString("properties");

					if ( StrEq( field, sEntity ) ) {
						desc.entity = DParser_ConsumeID(parser);
					} else if ( StrEq( field, sProperties ) ) {
						DParser_ConsumeScriptProperties(parser, desc);
					} else {
						LOG(Warning, "Unknown Script field <%.*s>.\n", field.size, field.str);
						DParser_SkipFieldValue(parser);
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
				descriptors.entityDescCount = 0;
				descriptors.roomDescs = PushZeroArray(arena, RoomDesc, descriptors.roomDescCount);
				descriptors.roomDescCount = 0;
				descriptors.audioClipDescs = PushZeroArray(arena, AudioClipDesc, descriptors.audioClipDescCount);
				descriptors.audioClipDescCount = 0;
				descriptors.musicFileDescs = PushZeroArray(arena, MusicFileDesc, descriptors.musicFileDescCount);
				descriptors.musicFileDescCount = 0;
				descriptors.scriptDescs = PushZeroArray(arena, ScriptDesc, descriptors.scriptDescCount);
				descriptors.scriptDescCount = 0;

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

		const u32 roomCount = descriptors.roomDescCount;
		const u32 roomsSize = roomCount * sizeof(BinRoomDesc);
		const u32 roomsOffset = PostIncrement(&offset, roomsSize);

		const u32 scriptCount = descriptors.scriptDescCount;
		const u32 scriptsSize = scriptCount * sizeof(BinScriptDesc);
		const u32 scriptsOffset = PostIncrement(&offset, scriptsSize);

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
		BinRoomDesc *binRoomDescs = PushArray(tempArena, BinRoomDesc, roomCount);
		BinScriptDesc *binScriptDescs = PushArray(tempArena, BinScriptDesc, scriptCount);

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
			const EntityDesc &desc = descriptors.entityDescs[i];

			BinEntityDesc &d = binEntityDescs[i];
			d.id           = desc.id;
			d.name         = DataInternString(stringPool, desc.name);
			d.materialId   = desc.materialId;
			d.spriteId     = desc.spriteId;
			d.pos          = desc.pos;
			d.scale        = desc.scale;
			d.layerId      = desc.layerId;
			d.geometryType = desc.geometryType;
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

		// Scripts (property payloads continue after the last written payload)
		for (u32 i = 0; i < scriptCount; ++i)
		{
			const ScriptDesc &desc = descriptors.scriptDescs[i];

			ASSERT(desc.propertyCount <= MAX_SCRIPT_PROPERTIES);

			BinScriptPropertyDesc binProperties[MAX_SCRIPT_PROPERTIES];
			for (u32 p = 0; p < desc.propertyCount; ++p)
			{
				const ScriptPropertyDesc &property = desc.properties[p];

				BinScriptPropertyDesc &pd = binProperties[p];
				pd.name  = DataInternString(stringPool, property.name);
				pd.type  = property.value.type;
				pd.value = property.value.uValue;
			}

			const u64 payloadSize = desc.propertyCount * sizeof(BinScriptPropertyDesc);

			BinScriptDesc &d = binScriptDescs[i];
			d.entity            = desc.entity;
			d.name              = DataInternString(stringPool, desc.name);
			d.properties.offset = PostIncrement(&offset, payloadSize);
			d.properties.size   = U64ToU32(payloadSize);

			if ( payloadSize > 0 ) {
				fwrite(binProperties, payloadSize, 1, file);
			}
		}

		// Write string pool after payloads
		const u32 stringPoolOffset = offset;
		const u32 stringPoolSize   = stringPool.size;
		fwrite(stringPool.str, stringPoolSize, 1, file);

		// Write asset descs
		fseek(file, sceneOffset, SEEK_SET);
		const BinSceneDesc binSceneDesc = { .projectionType = descriptors.sceneDesc.projectionType };
		fwrite(&binSceneDesc,     sizeof(binSceneDesc),         1,              file);
		fwrite(binShaderDescs,    sizeof(binShaderDescs[0]),    shaderCount,    file);
		fwrite(binImageDescs,     sizeof(binImageDescs[0]),     imageCount,     file);
		fwrite(binAudioClipDescs, sizeof(binAudioClipDescs[0]), audioClipCount, file);
		fwrite(binMusicFileDescs, sizeof(binMusicFileDescs[0]), musicFileCount, file);
		fwrite(binMaterialDescs,  sizeof(binMaterialDescs[0]),  materialCount,  file);
		fwrite(binSpriteDescs,    sizeof(binSpriteDescs[0]),    spriteCount,    file);
		fwrite(binEntityDescs,    sizeof(binEntityDescs[0]),    entityCount,    file);
		fwrite(binRoomDescs,      sizeof(binRoomDescs[0]),      roomCount,      file);
		fwrite(binScriptDescs,    sizeof(binScriptDescs[0]),    scriptCount,    file);

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
			.roomsOffset      = roomsOffset,
			.roomCount        = roomCount,
			.scriptsOffset    = scriptsOffset,
			.scriptCount      = scriptCount,
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
	assets.rooms = PushZeroArray(dataArena, BinRoom, assets.header.roomCount);
	assets.scripts = PushZeroArray(dataArena, BinScript, assets.header.scriptCount);

	const char *stringPool = (const char*)PushDataFromFile(
		dataArena, file, assets.header.stringPoolOffset, assets.header.stringPoolSize);

	// Scene
	BinSceneDesc *binSceneDesc = (BinSceneDesc*)PushDataFromFile(
		dataArena, file, assets.header.sceneOffset, sizeof(BinSceneDesc));
	assets.scene.projectionType = binSceneDesc->projectionType;

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
		d.name         = DataGetString(stringPool, d.name);
		assets.entities[i].desc = &d;
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

	// Scripts
	if (assets.header.scriptCount > 0)
	{
		BinScriptDesc *scriptDescs = (BinScriptDesc*)PushDataFromFile(
			dataArena, file, assets.header.scriptsOffset, assets.header.scriptCount * sizeof(BinScriptDesc));
		for (u32 i = 0; i < assets.header.scriptCount; ++i)
		{
			BinScriptDesc &d = scriptDescs[i];
			d.name = DataGetString(stringPool, d.name);
			assets.scripts[i].desc = &d;

			if (d.properties.size > 0)
			{
				BinScriptPropertyDesc *properties = (BinScriptPropertyDesc*)PushDataFromFile(
					dataArena, file, d.properties.offset, d.properties.size);
				const u32 propertyCount = d.properties.size / sizeof(BinScriptPropertyDesc);
				for (u32 p = 0; p < propertyCount; ++p)
				{
					properties[p].name = DataGetString(stringPool, properties[p].name);
				}
				assets.scripts[i].properties = properties;
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

