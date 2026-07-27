#include "ilu_core.h"

#define TOOLS_GFX_FUNCTION_POINTERS
#include "ilu_gfx.h"

#include "ilu_profile.h"

#define PLATFORM_API
#include "platform.h"

#define USE_EDITOR ( PLATFORM_LINUX || PLATFORM_WINDOWS )
#define USE_UI ( PLATFORM_LINUX || PLATFORM_WINDOWS )
#define USE_DATA_BUILD ( PLATFORM_LINUX || PLATFORM_WINDOWS )

#if USE_UI

#define STB_RECT_PACK_IMPLEMENTATION
#include "libs/stb/stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "libs/stb/stb_truetype.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_NO_LINEAR // Only stbi_load_from_memory (LDR) is used, never stbi_loadf (HDR) - this also drops stb_image.h's own <math.h> include, only needed by the linear/HDR path.
#define STBI_NO_HDR
#include "libs/stb/stb_image.h"

#endif // #if USE_UI

// Needed before ilu_ui.h
struct ImagePixels
{
	stbi_uc* pixels;
	i32 width;
	i32 height;
	i32 channelCount;
	bool constPixels;
};

#if USE_UI
#include "ilu_ui.h"
#endif

// C/HLSL shared types and bindings
#include "shaders/types.hlsl"
#include "shaders/bindings.hlsl"

#include "handle_manager.h"
#include "data.h"
#include "audio.h"
#include "engine.h"
#include "game.h"
#if USE_EDITOR
#include "editor.h"
#endif

struct Engine
{
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

	BinAssets shaderAssets;
	BinAssets assets;

	Arena dataArenaStates[1];
	u32 dataArenaStateCount;
};


#define MAX_DEBUG_DRAW_VERTICES 4092
#define PIXELS_PER_METER 16

#define INVALID_HANDLE -1



#if USE_DATA_BUILD
static constexpr bool sLoadShadersFromText = true;
#else
static constexpr bool sLoadShadersFromText = false;
#endif

static constexpr float4 ColorWhite = { 1.0f, 1.0f, 1.0f, 1.0f };
static constexpr float4 ColorBlack = { 0.0f, 0.0f, 0.0f, 0.0f };
static constexpr float4 ColorRed = { 1.0f, 0.0f, 0.0f, 1.0f };
static constexpr float4 ColorGreen = { 0.0f, 1.0f, 0.0f, 1.0f };
static constexpr float4 ColorBlue = { 0.0f, 0.0f, 1.0f, 1.0f };
static constexpr float4 ColorOrange = { 1.0f, 0.5f, 0.0f, 1.0f };


static Engine *engine = nullptr;

struct ShaderAndPipelineDesc
{
	const char *vsName;
	const char *fsName;
	const char *renderPass;
	PipelineDesc desc;
};

struct ShaderAndComputeDesc
{
	const char *csName;
	ComputeDesc desc;
};

static const Vertex cubeVertices[] = {
	// front
	{{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
	{{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
	{{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
	// back
	{{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
	{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
	{{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
	{{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
	// right
	{{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
	{{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
	{{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	// left
	{{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
	{{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
	{{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	// top
	{{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
	{{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
	{{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	// bottom
	{{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
	{{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
	{{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
	{{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
};

static const Index cubeIndices[] = {
	0,  1,  2,  2,  3,  0,  // front
	4,  5,  6,  6,  7,  4,  // back
	8,  9,  10, 10, 11, 8,  // right
	12, 13, 14, 14, 15, 12, // left
	16, 17, 18, 18, 19, 16, // top
	20, 21, 22, 22, 23, 20, // bottom
};

static const Vertex quadVertices[] = {
	{{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
	{{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
	{{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
};

static const Index quadIndices[] = {
	0, 1, 2, 2, 3, 0,
};

static const Vertex spriteVertices[] = {
	{{ 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
	{{ 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{{ 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
	{{ 1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
};

static const Index spriteIndices[] = {
	0, 1, 2, 2, 3, 0,
};

static const Vertex planeVertices[] = {
	{{-0.5f, 0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{ 0.5f, 0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{ 0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
	{{-0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
};

static const Index planeIndices[] = {
	0, 1, 2, 2, 3, 0,
};

static const Vertex screenTriangleVertices[] = {
	{{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	{{-1.0f, -3.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 2.0f}},
	{{ 3.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {2.0f, 0.0f}},
};

static const Index screenTriangleIndices[] = {
	0, 1, 2,
};

static ShaderSourceDesc shaderSourceDescs[] = {
	{ .type = ShaderTypeVertex,   .filename = "shading_3d.hlsl",     .entryPoint = "VSMain",      .name = "vs_shading_3d" },
	{ .type = ShaderTypeFragment, .filename = "shading_3d.hlsl",     .entryPoint = "PSMain",      .name = "fs_shading_3d" },
	{ .type = ShaderTypeVertex,   .filename = "shading_2d.hlsl",     .entryPoint = "VSMain",      .name = "vs_shading_2d", .defines = "-D USE_ENTITY_RENDERING 1" },
	{ .type = ShaderTypeFragment, .filename = "shading_2d.hlsl",     .entryPoint = "PSMain",      .name = "fs_shading_2d", .defines = "-D USE_ENTITY_RENDERING 1" },
	{ .type = ShaderTypeVertex,   .filename = "shading_2d.hlsl",     .entryPoint = "VSMain",      .name = "vs_shading_2d_tile", .defines = "-D USE_TILE_RENDERING 1" },
	{ .type = ShaderTypeFragment, .filename = "shading_2d.hlsl",     .entryPoint = "PSMain",      .name = "fs_shading_2d_tile", .defines = "-D USE_TILE_RENDERING 1" },
	{ .type = ShaderTypeVertex,   .filename = "sky.hlsl",            .entryPoint = "VSMain",      .name = "vs_sky" },
	{ .type = ShaderTypeFragment, .filename = "sky.hlsl",            .entryPoint = "PSMain",      .name = "fs_sky" },
	{ .type = ShaderTypeVertex,   .filename = "shadowmap.hlsl",      .entryPoint = "VSMain",      .name = "vs_shadowmap" },
	{ .type = ShaderTypeFragment, .filename = "shadowmap.hlsl",      .entryPoint = "PSMain",      .name = "fs_shadowmap" },
	{ .type = ShaderTypeVertex,   .filename = "grid_2d.hlsl",        .entryPoint = "VSMain",      .name = "vs_grid_2d" },
	{ .type = ShaderTypeFragment, .filename = "grid_2d.hlsl",        .entryPoint = "PSMain",      .name = "fs_grid_2d" },
	{ .type = ShaderTypeVertex,   .filename = "grid_3d.hlsl",        .entryPoint = "VSMain",      .name = "vs_grid_3d" },
	{ .type = ShaderTypeFragment, .filename = "grid_3d.hlsl",        .entryPoint = "PSMain",      .name = "fs_grid_3d" },
	{ .type = ShaderTypeVertex,   .filename = "blit.hlsl",           .entryPoint = "VSMain",      .name = "vs_blit" },
	{ .type = ShaderTypeFragment, .filename = "blit.hlsl",           .entryPoint = "PSMain",      .name = "fs_blit" },
	{ .type = ShaderTypeVertex,   .filename = "ui.hlsl",             .entryPoint = "VSMain",      .name = "vs_ui" },
	{ .type = ShaderTypeFragment, .filename = "ui.hlsl",             .entryPoint = "PSMain",      .name = "fs_ui" },
	{ .type = ShaderTypeVertex,   .filename = "id_model.hlsl",       .entryPoint = "VSMain",      .name = "vs_id_model" },
	{ .type = ShaderTypeFragment, .filename = "id_model.hlsl",       .entryPoint = "PSMain",      .name = "fs_id_model" },
	{ .type = ShaderTypeVertex,   .filename = "id_sprite.hlsl",      .entryPoint = "VSMain",      .name = "vs_id_sprite" },
	{ .type = ShaderTypeFragment, .filename = "id_sprite.hlsl",      .entryPoint = "PSMain",      .name = "fs_id_sprite" },
	{ .type = ShaderTypeCompute,  .filename = "compute_select.hlsl", .entryPoint = "CSMain",      .name = "compute_select" },
	{ .type = ShaderTypeCompute,  .filename = "compute.hlsl",        .entryPoint = "main_clear",  .name = "compute_clear" },
	{ .type = ShaderTypeCompute,  .filename = "compute.hlsl",        .entryPoint = "main_update", .name = "compute_update" },
	{ .type = ShaderTypeVertex,   .filename = "debug_draw.hlsl",     .entryPoint = "VSMain",      .name = "vs_debug_draw" },
	{ .type = ShaderTypeFragment, .filename = "debug_draw.hlsl",     .entryPoint = "PSMain",      .name = "fs_debug_draw" },
};

static const ShaderAndPipelineDesc pipelineDescs[] =
{
	{
		.vsName = "vs_shading_3d",
		.fsName = "fs_shading_3d",
		.renderPass = "scene_renderpass",
		.desc = {
			.name = "pipeline_shading",
			.vsFunction = "VSMain",
			.fsFunction = "PSMain",
			.vertexBufferCount = 1,
			.vertexBuffers = { { .stride = 32 }, },
			.vertexAttributeCount = 3,
			.vertexAttributes = {
				{ .bufferIndex = 0, .location = 0, .offset = 0, .format = FormatFloat3, },
				{ .bufferIndex = 0, .location = 1, .offset = 12, .format = FormatFloat3, },
				{ .bufferIndex = 0, .location = 2, .offset = 24, .format = FormatFloat2, },
			},
			.depthTest = true,
			.depthWrite = true,
			.depthCompareOp = CompareOpGreaterOrEqual,
		}
	},
	{
		.vsName = "vs_shading_2d",
		.fsName = "fs_shading_2d",
		.renderPass = "scene_renderpass",
		.desc = {
			.name = "pipeline_shading_2d",
			.vsFunction = "VSMain",
			.fsFunction = "PSMain",
			.vertexBufferCount = 1,
			.vertexBuffers = { { .stride = 32 }, },
			.vertexAttributeCount = 3,
			.vertexAttributes = {
				{ .bufferIndex = 0, .location = 0, .offset = 0, .format = FormatFloat3, },
				{ .bufferIndex = 0, .location = 1, .offset = 12, .format = FormatFloat3, },
				{ .bufferIndex = 0, .location = 2, .offset = 24, .format = FormatFloat2, },
			},
			.depthTest = true,
			.depthWrite = true,
			.depthCompareOp = CompareOpGreaterOrEqual,
			.blending = true,
		}
	},
	{
		.vsName = "vs_shading_2d_tile",
		.fsName = "fs_shading_2d_tile",
		.renderPass = "scene_renderpass",
		.desc = {
			.name = "pipeline_shading_2d_tile",
			.vsFunction = "VSMain",
			.fsFunction = "PSMain",
			.vertexBufferCount = 1,
			.vertexBuffers = { { .stride = 32 }, },
			.vertexAttributeCount = 3,
			.vertexAttributes = {
				{ .bufferIndex = 0, .location = 0, .offset = 0, .format = FormatFloat3, },
				{ .bufferIndex = 0, .location = 1, .offset = 12, .format = FormatFloat3, },
				{ .bufferIndex = 0, .location = 2, .offset = 24, .format = FormatFloat2, },
			},
			.depthTest = true,
			.depthWrite = true,
			.depthCompareOp = CompareOpGreaterOrEqual,
			.blending = true,
		}
	},
	{
		.vsName = "vs_shadowmap",
		.fsName = "fs_shadowmap",
		.renderPass = "shadowmap_renderpass",
		.desc = {
			.name = "pipeline_shadowmap",
			.vsFunction = "VSMain",
			.fsFunction = "PSMain",
			.vertexBufferCount = 1,
			.vertexBuffers = { { .stride = 32 }, },
			.vertexAttributeCount = 1,
			.vertexAttributes = {
				{ .bufferIndex = 0, .location = 0, .offset = 0, .format = FormatFloat3, },
			},
			.depthTest = true,
			.depthWrite = true,
			.depthCompareOp = CompareOpGreater,
		}
	},
	{
		.vsName = "vs_sky",
		.fsName = "fs_sky",
		.renderPass = "scene_renderpass",
		.desc = {
			.name = "pipeline_sky",
			.vsFunction = "VSMain",
			.fsFunction = "PSMain",
			.vertexBufferCount = 1,
			.vertexBuffers = { { .stride = 32 }, },
			.vertexAttributeCount = 2,
			.vertexAttributes = {
				{ .bufferIndex = 0, .location = 0, .offset = 0, .format = FormatFloat3, },
				{ .bufferIndex = 0, .location = 1, .offset = 24, .format = FormatFloat2, },
			},
			.depthTest = true,
			.depthWrite = false,
			.depthCompareOp = CompareOpGreaterOrEqual,
		}
	},
	{
		.vsName = "vs_grid_2d",
		.fsName = "fs_grid_2d",
		.renderPass = "scene_renderpass",
		.desc = {
			.name = "pipeline_grid_2d",
			.vsFunction = "VSMain",
			.fsFunction = "PSMain",
			.vertexBufferCount = 1,
			.vertexBuffers = { { .stride = 32 }, },
			.vertexAttributeCount = 2,
			.vertexAttributes = {
				{ .bufferIndex = 0, .location = 0, .offset = 0, .format = FormatFloat3, },
				{ .bufferIndex = 0, .location = 1, .offset = 24, .format = FormatFloat2, },
			},
			.depthTest = true,
			.depthWrite = false,
			.depthCompareOp = CompareOpGreaterOrEqual,
			.blending = true,
		}
	},
	{
		.vsName = "vs_grid_3d",
		.fsName = "fs_grid_3d",
		.renderPass = "scene_renderpass",
		.desc = {
			.name = "pipeline_grid_3d",
			.vsFunction = "VSMain",
			.fsFunction = "PSMain",
			.vertexBufferCount = 1,
			.vertexBuffers = { { .stride = 32 }, },
			.vertexAttributeCount = 2,
			.vertexAttributes = {
				{ .bufferIndex = 0, .location = 0, .offset = 0, .format = FormatFloat3, },
				{ .bufferIndex = 0, .location = 1, .offset = 24, .format = FormatFloat2, },
			},
			.depthTest = true,
			.depthWrite = false,
			.depthCompareOp = CompareOpGreaterOrEqual,
			.blending = true,
		}
	},
	{
		.vsName = "vs_blit",
		.fsName = "fs_blit",
		.renderPass = "display_renderpass",
		.desc = {
			.name = "pipeline_blit",
			.vsFunction = "VSMain",
			.fsFunction = "PSMain",
			.vertexBufferCount = 1,
			.vertexBuffers = { { .stride = 32 }, },
			.vertexAttributeCount = 2,
			.vertexAttributes = {
				{ .bufferIndex = 0, .location = 0, .offset = 0,  .format = FormatFloat3, },
				{ .bufferIndex = 0, .location = 1, .offset = 24, .format = FormatFloat2, },
			},
			.depthTest = false,
			.blending = false,
		}
	},
	{
		.vsName = "vs_ui",
		.fsName = "fs_ui",
		.renderPass = "display_renderpass",
		.desc = {
			.name = "pipeline_ui",
			.vsFunction = "VSMain",
			.fsFunction = "PSMain",
			.vertexBufferCount = 1,
			.vertexBuffers = { { .stride = 20 }, },
			.vertexAttributeCount = 3,
			.vertexAttributes = {
				{ .bufferIndex = 0, .location = 0, .offset = 0, .format = FormatFloat2, },
				{ .bufferIndex = 0, .location = 1, .offset = 8, .format = FormatFloat2, },
				{ .bufferIndex = 0, .location = 2, .offset = 16, .format = FormatRGBA8, },
			},
			.depthTest = false,
			.blending = true,
		}
	},
#if USE_EDITOR
	{
		.vsName = "vs_id_model",
		.fsName = "fs_id_model",
		.renderPass = "id_renderpass",
		.desc = {
			.name = "pipeline_model_id",
			.vsFunction = "VSMain",
			.fsFunction = "PSMain",
			.vertexBufferCount = 1,
			.vertexBuffers = { { .stride = 32 }, },
			.vertexAttributeCount = 3,
			.vertexAttributes = {
				{ .bufferIndex = 0, .location = 0, .offset = 0, .format = FormatFloat3, },
				{ .bufferIndex = 0, .location = 1, .offset = 12, .format = FormatFloat3, },
				{ .bufferIndex = 0, .location = 2, .offset = 24, .format = FormatFloat2, },
			},
			.depthTest = true,
			.depthWrite = false,
			.depthCompareOp = CompareOpEqual,
		}
	},
	{
		.vsName = "vs_id_sprite",
		.fsName = "fs_id_sprite",
		.renderPass = "id_renderpass",
		.desc = {
			.name = "pipeline_sprite_id",
			.vsFunction = "VSMain",
			.fsFunction = "PSMain",
			.vertexBufferCount = 1,
			.vertexBuffers = { { .stride = 32 }, },
			.vertexAttributeCount = 3,
			.vertexAttributes = {
				{ .bufferIndex = 0, .location = 0, .offset = 0, .format = FormatFloat3, },
				{ .bufferIndex = 0, .location = 1, .offset = 12, .format = FormatFloat3, },
				{ .bufferIndex = 0, .location = 2, .offset = 24, .format = FormatFloat2, },
			},
			.depthTest = true,
			.depthWrite = false,
			.depthCompareOp = CompareOpEqual,
		}
	},
#endif // USE_EDITOR
	{
		.vsName = "vs_debug_draw",
		.fsName = "fs_debug_draw",
		.renderPass = "scene_renderpass",
		.desc = {
			.name = "pipeline_debug_draw",
			.vsFunction = "VSMain",
			.fsFunction = "PSMain",
			.vertexBufferCount = 1,
			.vertexBuffers = { { .stride = 20 }, },
			.vertexAttributeCount = 3,
			.vertexAttributes = {
				{ .bufferIndex = 0, .location = 0, .offset = 0,  .format = FormatFloat2, },
				{ .bufferIndex = 0, .location = 1, .offset = 8,  .format = FormatFloat2, },
				{ .bufferIndex = 0, .location = 2, .offset = 16, .format = FormatRGBA8, },
			},
			.depthTest = false,
			.blending = true,
		}
	},
};

static const ShaderAndComputeDesc computeDescs[] =
{
	//{ .name = "compute_clear", .filename = "shaders/compute_clear.spv", .function = "main_clear" },
	//{ .name = "compute_update", .filename = "shaders/compute_update.spv", .function = "main_update" },
	{
		.csName = "compute_select",
		.desc = {
			.name = "compute_select",
			.function = "CSMain"
		},
	},
};

static const char * InternString(const char *str)
{
	const char *intern = MakeStringIntern(sPlatform->stringInterning, str);
	return intern;
}

u32 U32FromChars(char a, char b, char c, char d)
{
	const u32 res =
		(u32)a << 0 |
		(u32)b << 8 |
		(u32)c << 16 |
		(u32)d << 24 ;
	return res;
}

Engine &GetEngine(Plat &platform)
{
	Engine *engine = platform.engine;
	ASSERT(engine != NULL);
	return *engine;
}

#if USE_UI
UI &GetUI(Engine &engine)
{
	return engine.ui;
}
#endif



////////////////////////////////////////////////////////////////////////
// Image loading

bool ReadImagePixels(Arena &arena, const char *filepath, ImagePixels &image)
{
	bool ok = true;

	DataChunk* chunk = PushFile(arena, filepath);
	if ( !chunk ) {
		LOG(Error, "PushFile failed to read: %s\n", filepath);
		ok = false;
		return ok;
	}

	image = {};
	image.pixels = stbi_load_from_memory(chunk->bytes, chunk->size, &image.width, &image.height, &image.channelCount, STBI_rgb_alpha);
	image.channelCount = 4; // Because we use STBI_rgb_alpha
	if ( !image.pixels )
	{
		LOG(Error, "stbi_load_from_memory failed to load %s\n", filepath);
		static stbi_uc constPixels[] = {255, 0, 255, 255};
		image.pixels = constPixels;
		image.width = image.height = 1;
		image.channelCount = 4;
		image.constPixels = true;
		ok = false;
	}
	return ok;
}

ImagePixels ResizeImagePixels(Arena &arena, ImagePixels inputImagePixels, i32 w, i32 h)
{
	i32 channelCount = inputImagePixels.channelCount;
	i32 inW = inputImagePixels.width;
	i32 inH = inputImagePixels.height;
	byte *inPixels = inputImagePixels.pixels;
	byte *outPixels = PushArray(arena, byte, w * h * channelCount);

	i32 inStride = inputImagePixels.width * channelCount;
	i32 outStride = w * channelCount;

	for (u32 i = 0; i < h; ++i) {
		for (u32 j = 0; j < w; ++j) {
			for (u32 c = 0; c < channelCount; ++c) {
				i32 inI = inH * i / h;
				i32 inJ = inW * j / w;
				byte value =  inPixels[inI * inStride + inJ * channelCount + c];
				outPixels[i * outStride + j * channelCount + c] = value;
			}
		}
	}

	ImagePixels outputImagePixels = {
		.pixels = outPixels,
		.width = w,
		.height = h,
		.channelCount = channelCount,
		.constPixels = false,
	};

	return outputImagePixels;
}



////////////////////////////////////////////////////////////////////////

void AddTimeSample(TimeSamples &timeSamples, f32 sample)
{
	timeSamples.samples[timeSamples.sampleCount] = sample;
	timeSamples.sampleCount = ( timeSamples.sampleCount + 1 ) % ARRAY_COUNT(timeSamples.samples);
	f32 sum = 0.0f;
	for (u32 i = 0; i < ARRAY_COUNT(timeSamples.samples); ++i)
	{
		sum += timeSamples.samples[i];
	}
	timeSamples.average = sum / ARRAY_COUNT(timeSamples.samples);
}

const u32 FindShaderSourceDescIndex(const char *name)
{
	for (u32 i = 0; i < ARRAY_COUNT(shaderSourceDescs); ++i) {
		if ( StrEq(shaderSourceDescs[i].name, name) ) {
			return i;
		}
	}
	LOG(Warning, "Could not find ShaderSourceDesc <%s>.\n", name);
	INVALID_CODE_PATH();
	return U32_MAX;
}

RenderPassH FindRenderPassHandle(const Graphics &gfx, const char *name)
{
	for (u32 i = 0; i < gfx.device.renderPassCount; ++i) {
		if ( StrEq(gfx.device.renderPasses[i].name, name) ) {
			return { .index = i };
		}
	}
	LOG(Warning, "Could not find render <%s> handle.\n", name);
	INVALID_CODE_PATH();
	return { .index = (u32)INVALID_HANDLE };
}

PipelineH FindPipelineHandle(const Graphics &gfx, const char *name)
{
	for (u32 i = 0; i < ARRAY_COUNT(gfx.device.pipelines); ++i) {
		if ( StrEq(gfx.device.pipelines[i].name, name) ) {
			return { .index = i };
		}
	}

	return { .index = 0 }; // For pipelines, 0 is invalid
}



struct StagedData
{
	BufferH buffer;
	u32 offset;
};

static CommandList BeginUploadCommandList(Graphics &gfx)
{
	ASSERT(!gfx.inUploadContext && "Cannot nest calls to BeginUploadCommandList");
	gfx.inUploadContext = true;

	gfx.stagingBufferOffset = 0;

	CommandList commandList = BeginTransientCommandList(gfx.device);
	return commandList;
}

static void EndUploadCommandList(Graphics &gfx, CommandList commandList)
{
	EndTransientCommandList(gfx.device, commandList);

	ASSERT(gfx.inUploadContext && "BeginUploadCommandList must have been called first");
	gfx.inUploadContext = false;
}

static StagedData StageData(Graphics &gfx, const void *data, u32 size, u32 alignment = 0)
{
	ASSERT(gfx.inUploadContext && "StageData must be called between calls to Begin/EndUploadCommandList");

	const Buffer &stagingBuffer = GetBuffer(gfx.device, gfx.stagingBuffer);

	const u32 finalAlignment = Max(alignment, gfx.device.alignment.optimalBufferCopyOffset);
	const u32 unalignedOffset = stagingBuffer.alloc.offset + gfx.stagingBufferOffset;
	const u32 alignedOffset = AlignUp(unalignedOffset, finalAlignment);

	ASSERT(alignedOffset + size <= stagingBuffer.size);

	StagedData staging = {};
	staging.buffer = gfx.stagingBuffer;
	staging.offset = alignedOffset;

	Heap &stagingHeap = gfx.device.heaps[HeapType_Staging];
	void* stagingData = stagingHeap.data + staging.offset;
	MemCopy(stagingData, data, size);

	gfx.stagingBufferOffset = staging.offset + size;

	return staging;
}

BufferH CreateStagingBuffer(Graphics &gfx)
{
	const Heap &stagingHeap = gfx.device.heaps[HeapType_Staging];
	BufferH stagingBufferHandle = CreateBuffer(gfx.device, stagingHeap.size, BufferUsageTransferSrc, HeapType_Staging);
	return stagingBufferHandle;
}

BufferH CreateVertexBuffer(Graphics &gfx, u32 size)
{
	BufferH vertexBufferHandle = CreateBuffer(
			gfx.device,
			size,
			BufferUsageVertexBuffer | BufferUsageTransferDst,
			HeapType_General);

	return vertexBufferHandle;
}

BufferH CreateIndexBuffer(Graphics &gfx, u32 size)
{
	BufferH indexBufferHandle = CreateBuffer(
			gfx.device,
			size,
			BufferUsageIndexBuffer | BufferUsageTransferDst,
			HeapType_General);

	return indexBufferHandle;
}

BufferArena MakeBufferArena(Graphics &gfx, BufferH bufferHandle)
{
	const BufferArena arena = {
		.buffer = bufferHandle,
		.used = 0,
	};
	return arena;
}

void UploadData(Graphics &gfx, const CommandList &commandList, const void *data, u32 size, BufferH destBuffer, u32 destOffset, u32 alignment = 0)
{
	StagedData staged = StageData(gfx, data, size, alignment);

	// Copy contents from the staging to the final buffer
	CopyBufferToBuffer(commandList, staged.buffer, staged.offset, destBuffer, destOffset, size);
}

BufferChunk PushData(Graphics &gfx, const CommandList &commandList, BufferArena &arena, const void *data, u32 size, u32 alignment = 0)
{
	if (data)
	{
		UploadData(gfx, commandList, data, size, arena.buffer, arena.used, alignment);
	}

	BufferChunk chunk = {};
	chunk.buffer = arena.buffer;
	chunk.offset = arena.used;
	chunk.size = size;

	arena.used += size;

	return chunk;
}


////////////////////////////////////////////////////////////////////////
// Image management

void GenerateMipmaps(const GraphicsDevice &device, const CommandList &commandList, ImageH imageH)
{
	const Image &image = GetImageConst(device, imageH);

	if (!device.formatSupport[image.format].linearFilteredSampling) {
		LOG(Error, "GenerateMipmaps() - Linear filtering not supported for format %s.\n", FormatName(image.format));
		QUIT_ABNORMALLY();
	}

	for (u32 i = 1; i < image.mipLevels; ++i)
	{
		TransitionImageLayout(commandList, imageH, ImageStateTransferDst, ImageStateTransferSrc, i - 1, 1);

		const i32 srcWidth = image.width > 1 ? image.width >> (i-1): 1;
		const i32 srcHeight = image.height > 1 ? image.height >> (i-1) : 1;
		const i32 dstWidth = image.width > 1 ? image.width >> i : 1;
		const i32 dstHeight = image.height > 1 ? image.height >> i : 1;
		const BlitRegion srcRegion = { .x = 0, .y = 0, .width = srcWidth, .height = srcHeight, .mipLevel = i - 1 };
		const BlitRegion dstRegion = { .x = 0, .y = 0, .width = dstWidth, .height = dstHeight, .mipLevel = i };

		Blit(commandList, image, srcRegion, image, dstRegion);

		TransitionImageLayout(commandList, imageH, ImageStateTransferSrc, ImageStateShaderInput, i - 1, 1);
	}

	TransitionImageLayout(commandList, imageH, ImageStateTransferDst, ImageStateShaderInput, image.mipLevels - 1, 1);
}

ImageH EngineCreateImage(Graphics &gfx, const char *name, int width, int height, int channels, bool mipmap, const byte *pixels)
{
	const u32 pixelSize = channels * sizeof(byte);
	const u32 size = width * height * pixelSize;
	const u32 alignment = channels == 1 ? 1 : 4;

	const u32 mipLevels = mipmap ?
		static_cast<uint32_t>(Floor(Log2(Max(width, height)))) + 1 :
		1;

	ASSERT(channels >= 1 && channels <= 4);
	const Format texFormat =
		channels == 4 ? FormatRGBA8_SRGB :
		channels == 3 ? FormatRGB8_SRGB :
		channels == 2 ? FormatRG8_SRGB :
		FormatR8;

	ImageH image = CreateImage(gfx.device,
			width, height, mipLevels,
			texFormat,
			ImageUsageTransferSrc | // for mipmap blits
			ImageUsageTransferDst | // for intitial copy from buffer and blits
			ImageUsageSampled, // to be sampled in shaders
			HeapType_General);

	CommandList commandList = BeginUploadCommandList(gfx);

	StagedData staged = StageData(gfx, pixels, size, alignment);

	TransitionImageLayout(commandList, image, ImageStateInitial, ImageStateTransferDst, 0, mipLevels);

	CopyBufferToImage(commandList, staged.buffer, staged.offset, image);

	if ( mipLevels > 1 )
	{
		// GenerateMipmaps takes care of transitions after creating the image
		GenerateMipmaps(gfx.device, commandList, image);
	}
	else
	{
		TransitionImageLayout(commandList, image, ImageStateTransferDst, ImageStateShaderInput, 0, 1);
	}

	EndUploadCommandList(gfx, commandList);

	SetObjectNameImage(gfx.device, image, name);

	return image;
}

ImageH EngineCreateImage(Graphics &gfx, const ImagePixels &img, const char *name, bool createMipmaps)
{
	const ImageH imageHandle = EngineCreateImage(gfx, name, img.width, img.height, img.channelCount, createMipmaps, img.pixels);
	return imageHandle;
}

////////////////////////////////////////////////////////////////////////
// Texture management

Texture &GetTexture(Graphics &gfx, TextureH handle)
{
	ASSERT( IsValidHandle(gfx.textureHandles, handle) );
	Texture &texture = gfx.textures[handle.idx];
	return texture;
}

TextureDesc &GetTextureDesc(Graphics &gfx, TextureH handle)
{
	ASSERT( IsValidHandle(gfx.textureHandles, handle) );
	TextureDesc &textureDesc = gfx.textureDescs[handle.idx];
	return textureDesc;
}

Texture &GetTextureAt(Graphics &gfx, u32 index)
{
	const u16 handleIndex = gfx.textureHandles.indices[index];
	const TextureH handle = gfx.textureHandles.handles[handleIndex];
	Texture &texture = GetTexture(gfx, handle);
	return texture;
}

TextureH CreateTexture(Graphics &gfx)
{
	const TextureH textureHandle = NewHandle(gfx.textureHandles);
	return textureHandle;
}

TextureH CreateTexture(Graphics &gfx, const TextureDesc &desc, ImageH imageH)
{
	TextureH textureHandle = CreateTexture(gfx);

	gfx.textureDescs[textureHandle.idx] = desc;

	Texture &texture = GetTexture(gfx, textureHandle);
	texture.name = desc.name;
	texture.image = imageH;

	const Image &image = GetImageConst(gfx.device, imageH);
	texture.size = { image.width, image.height };

	return textureHandle;
}

TextureH CreateTexture(Graphics &gfx, const TextureDesc &desc)
{
	Handle textureHandle = InvalidHandle;

	Scratch scratch;
	const FilePath imagePath = MakePath(AssetDir, desc.filename);
	ImagePixels img;
	if ( ReadImagePixels(scratch.arena, imagePath.str, img) )
	{
		const ImageH imageHandle = EngineCreateImage(gfx, img, desc.name, desc.mipmap);

		textureHandle = CreateTexture(gfx, desc, imageHandle);

		Texture &texture = GetTexture(gfx, textureHandle);
		GetFileLastWriteTimestamp(imagePath.str, texture.ts);
	}

	return textureHandle;
}

TextureH GetOrCreateTexture(Graphics &gfx, const TextureDesc &desc)
{
	const FilePath imagePath = MakePath(AssetDir, desc.filename);

	TextureH textureHandle = InvalidHandle;
	HandleIter it = BeginIter(gfx.textureHandles);
	while (it)
	{
		Handle handle = *it;
		const TextureDesc &desc = GetTextureDesc(gfx, handle);
		const FilePath imagePath2 = MakePath(AssetDir, desc.filename);
		if ( !( desc.flags & AssetFlag_Builtin ) && StrEq(imagePath.str, imagePath2.str)) {
			textureHandle = handle;
			break;
		}
		it++;
	}

	if ( textureHandle == InvalidHandle )
	{
		textureHandle = CreateTexture(gfx, desc);
	}
	return textureHandle;
}

TextureH CreateTexture(Graphics &gfx, const BinImage &binImage)
{
	const BinImageDesc &desc = *binImage.desc;
	const char *name = desc.name;
	const u32 width = desc.width;
	const u32 height = desc.height;
	const u32 channels = desc.channels;
	const u32 mipmap = desc.mipmap;
	const u8 *pixels = binImage.pixels;

	const ImageH imageHandle = EngineCreateImage(gfx, name, width, height, channels, mipmap, pixels);

	const TextureH textureHandle = CreateTexture(gfx);

	ZeroStruct( &gfx.textureDescs[textureHandle.idx] );

	Texture &texture = GetTexture(gfx, textureHandle);
	texture.name = desc.name;
	texture.image = imageHandle;

	return textureHandle;
}

ImageH GetTextureImage(Graphics &gfx, TextureH textureH, ImageH imageH)
{
	ImageH res = imageH;

	if ( IsValidHandle(gfx.textureHandles, textureH) ) {
		const Texture &texture = GetTexture(gfx, textureH);
		res = texture.image;
	}

	return res;
}

static bool IsTextureName( Handle handle, const char *name, void *data )
{
	Graphics &gfx = *(Graphics*)data;
	Texture &texture = GetTexture(gfx, handle);
	const bool equals = StrEq(texture.name, name);
	return equals;
}

TextureH FindTextureHandle(Graphics &gfx, const char *name)
{
	if (!name) return InvalidHandle;
	const HandleFinder finder = {
		.checkHandle = IsTextureName,
		.name = name,
		.data = &gfx,
	};
	const TextureH handle = FindHandle(gfx.textureHandles, finder);
	return handle;
}

void RemoveTexture(Graphics &gfx, TextureH textureH)
{
	if (IsValidHandle(gfx.textureHandles, textureH))
	{
		Texture &texture = GetTexture(gfx, textureH);

		DestroyImageH(gfx.device, texture.image);
		texture = {};

		FreeHandle(gfx.textureHandles, textureH);

		gfx.shouldUpdateMaterialBindGroups = true;
	}
}

static void RecreateTextureIfModifed(Handle handle, void* data)
{
	Engine &engine = *(Engine*)data;
	Graphics &gfx = engine.gfx;

	Texture &texture = GetTexture(gfx, handle);
	const TextureDesc &desc = GetTextureDesc(gfx, handle);

	// TODO(jesus): Textures loaded from bin data file do not have descriptor...
	if ( StrEq(desc.filename, "") ) { return; };

	const FilePath imagePath = MakePath(AssetDir, desc.filename);

	u64 ts;
	GetFileLastWriteTimestamp(imagePath.str, ts);

	if ( ts > texture.ts )
	{
		ImagePixels img;
		Scratch scratch;

		if ( ReadImagePixels(scratch.arena, imagePath.str, img) )
		{
			WaitDeviceIdle(gfx.device);

			texture.ts = ts;

			DestroyImageH(gfx.device, texture.image);

			texture.image = EngineCreateImage(gfx, img, desc.name, desc.mipmap);

			GetFileLastWriteTimestamp(imagePath.str, texture.ts);

			gfx.shouldUpdateMaterialBindGroups = true;
		}
	}
}

void RecreateModifiedTextures(Engine &engine)
{
	ForAllHandles(engine.gfx.textureHandles, RecreateTextureIfModifed, &engine);
}


////////////////////////////////////////////////////////////////////////
// Material management

Material &GetMaterial(Graphics &gfx, MaterialH handle)
{
	ASSERT( IsValidHandle(gfx.materialHandles, handle) );
	Material &material = gfx.materials[handle.idx];
	return material;
}

MaterialDesc &GetMaterialDesc(Graphics &gfx, MaterialH handle)
{
	ASSERT( IsValidHandle(gfx.materialHandles, handle) );
	MaterialDesc &materialDesc = gfx.materialDescs[handle.idx];
	return materialDesc;
}

void CreateMaterialBindGroup(Graphics &gfx, MaterialH materialH);

MaterialH CreateMaterial(Graphics &gfx, const MaterialDesc &desc)
{
	TextureH textureHandle = FindTextureHandle(gfx, desc.textureName);
	PipelineH pipelineHandle = FindPipelineHandle(gfx, desc.pipelineName);

	MaterialH materialHandle = NewHandle(gfx.materialHandles);
	Material& material = GetMaterial(gfx, materialHandle);
	material.name = desc.name;
	material.pipelineName = desc.pipelineName;
	material.pipelineH = pipelineHandle;
	material.albedoTexture = textureHandle;
	material.uvScale = desc.uvScale;
	material.bufferOffset = materialHandle.idx * AlignUp(sizeof(SMaterial), gfx.device.alignment.uniformBufferOffset);

	gfx.materialDescs[materialHandle.idx] = desc;
	gfx.shouldUpdateMaterials = true;

	CreateMaterialBindGroup(gfx, materialHandle);

	return materialHandle;
}

MaterialH GetOrCreateMaterial(Graphics &gfx, const MaterialDesc &desc)
{
	MaterialH materialHandle = InvalidHandle;
	HandleIter it = BeginIter(gfx.materialHandles);
	while (it)
	{
		Handle handle = *it;
		const MaterialDesc &materialDesc = GetMaterialDesc(gfx, handle);
		if ( !( desc.flags & AssetFlag_Builtin ) && StrEq(desc.name, materialDesc.name)) {
			materialHandle = handle;
			break;
		}
		it++;
	}

	if ( materialHandle == InvalidHandle )
	{
		materialHandle = CreateMaterial(gfx, desc);
	}
	return materialHandle;
}

MaterialH CreateMaterial( Graphics &gfx, const BinMaterialDesc &desc)
{
	const MaterialDesc materialDesc = {
		.name = desc.name,
		.textureName = desc.textureName,
		.pipelineName = desc.pipelineName,
		.uvScale = desc.uvScale,
	};
	MaterialH materialHandle = CreateMaterial(gfx, materialDesc);
	return materialHandle;
}

static bool IsMaterialName( Handle handle, const char *name, void *data )
{
	Graphics &gfx = *(Graphics*)data;
	Material &material = GetMaterial(gfx, handle);
	const bool equals = StrEq(material.name, name);
	return equals;
}

MaterialH FindMaterialHandle(Graphics &gfx, const char *name)
{
	if (!name) return InvalidHandle;
	const HandleFinder finder = {
		.checkHandle = IsMaterialName,
		.name = name,
		.data = &gfx,
	};
	const MaterialH handle = FindHandle(gfx.materialHandles, finder);
	return handle;
}

SpriteH CreateSprite(Engine &engine, const SpriteDesc &desc)
{
	Scene &scene = engine.scene;
	Graphics &gfx = engine.gfx;

	const TextureH textureH = FindTextureHandle(gfx, desc.textureName);

	Sprite sprite = {};
	sprite.name       = desc.name;
	sprite.textureH   = textureH;
	sprite.frameCount = desc.frameCount > 0 ? desc.frameCount : 1;
	sprite.fps        = desc.fps;
	sprite.loop       = desc.loop != 0;

	{
		const Texture &tex = GetTexture(gfx, textureH);
		const uint2 size = (desc.size.x > 0 || desc.size.y > 0) ? desc.size : tex.size;
		sprite.pos  = desc.pos;
		sprite.size = size;
	}

	SpriteH handle = NewHandle(scene.spriteHandles);
	scene.sprites[handle.idx] = sprite;
	return handle;
}

SpriteH CreateSprite(Engine &engine, const BinSpriteDesc &desc)
{
	const SpriteDesc txtDesc = {
		.name        = desc.name,
		.textureName = desc.textureName,
		.pos         = desc.pos,
		.size        = desc.size,
		.frameCount  = desc.frameCount,
		.fps         = desc.fps,
		.loop        = desc.loop,
	};
	return CreateSprite(engine, txtDesc);
};

Sprite &GetSprite(Scene &scene, SpriteH handle)
{
	ASSERT( IsValidHandle(scene.spriteHandles, handle) );
	return scene.sprites[handle.idx];
}

const SpriteDesc GetSpriteDesc(Scene &scene, SpriteH handle)
{
	const Sprite &sprite = GetSprite(scene, handle);
	const Texture &tex = GetTexture(engine->gfx, sprite.textureH);

	const SpriteDesc desc = {
		.name = sprite.name,
		.textureName = tex.name,
		.pos = sprite.pos,
		.size = sprite.size,
		.frameCount = sprite.frameCount,
		.fps = sprite.fps,
		.loop = sprite.loop,
	};
	return desc;
}

static SpriteH FindSpriteHandle(const Scene &scene, const char *name)
{
	if (!name) return InvalidHandle;
	for (HandleIter it = BeginIter(scene.spriteHandles); it; it++)
	{
		Handle handle = *it;
		if (StrEq(scene.sprites[handle.idx].name, name)) return handle;
	}
	return InvalidHandle;
}

SpriteH GetOrCreateSprite(Engine &engine, const SpriteDesc &desc)
{
	SpriteH handle = FindSpriteHandle(engine.scene, desc.name);
	if (handle == InvalidHandle)
		handle = CreateSprite(engine, desc);
	return handle;
}

void RemoveSprite(Scene &scene, SpriteH handle)
{
	scene.sprites[handle.idx] = {};
	FreeHandle(scene.spriteHandles, handle);
}


void RemoveMaterial(Graphics &gfx, MaterialH materialH)
{
	Material &material = GetMaterial(gfx, materialH);
	const MaterialDesc &desc = GetMaterialDesc(gfx, materialH);

	material = {};

	FreeHandle(gfx.materialHandles, materialH);

	// TODO: Do we need this here? I think we don't as we are removing, not modifying
	//gfx.shouldUpdateMaterialBindGroups = true;
}


////////////////////////////////////////////////////////////////////////
// Builtin geometry

BufferChunk GetVerticesForGeometryType(Graphics &gfx, GeometryType geometryType)
{
	if ( geometryType == GeometryTypeCube) {
		return gfx.cubeVertices;
	} else if ( geometryType == GeometryTypeQuad ) {
		return gfx.quadVertices;
	} else if ( geometryType == GeometryTypePlane ) {
		return gfx.planeVertices;
	} else if ( geometryType == GeometryTypeSprite ) {
		return gfx.spriteVertices;
	} else {
		return gfx.screenTriangleVertices;
	}
}

BufferChunk GetIndicesForGeometryType(Graphics &gfx, GeometryType geometryType)
{
	if ( geometryType == GeometryTypeCube) {
		return gfx.cubeIndices;
	} else if ( geometryType == GeometryTypeQuad ) {
		return gfx.quadIndices;
	} else if ( geometryType == GeometryTypePlane ) {
		return gfx.planeIndices;
	} else if ( geometryType == GeometryTypeSprite ) {
		return gfx.spriteIndices;
	} else {
		return gfx.screenTriangleIndices;
	}
}


////////////////////////////////////////////////////////////////////////
// Room management

Room &GetRoom(Scene &scene, Handle handle)
{
	ASSERT( IsValidHandle(scene.roomHandles, handle) );
	Room &room = scene.rooms[handle.idx];
	return room;
}


////////////////////////////////////////////////////////////////////////
// Entity management

Entity &GetEntity(Scene &scene, Handle handle)
{
	ASSERT( IsValidHandle(scene.entityHandles, handle) );
	Entity &entity = scene.entities[handle.idx];
	return entity;
}

void EntitySetPosition(Entity &entity, float3 position)
{
	entity.position = position;
}

EntityDesc GetEntityDesc(Scene &scene, Handle handle)
{
	ASSERT( IsValidHandle(scene.entityHandles, handle) );
	const Entity &entity = GetEntity(scene, handle);
	EntityDesc entityDesc = {
		.name  = entity.name,
		.layer = entity.layer,
		.pos   = entity.position,
		.scale = entity.scale,
	};
	if (IsValidHandle(scene.spriteHandles, entity.spriteH)) {
		entityDesc.spriteName = GetSprite(scene, entity.spriteH).name;
	} else {
		const Material &material = GetMaterial(engine->gfx, entity.materialH);
		entityDesc.materialName = material.name;
		entityDesc.geometryType = entity.geometryType;
	}
	return entityDesc;
}

Handle CreateEntity(Engine &engine, const EntityDesc &desc)
{
	Scene &scene = engine.scene;

	BufferChunk vertices = GetVerticesForGeometryType(engine.gfx, desc.geometryType);
	BufferChunk indices = GetIndicesForGeometryType(engine.gfx, desc.geometryType);

	Handle handle = NewHandle(scene.entityHandles);
	Entity &entity = GetEntity(scene, handle);
	entity.name = desc.name;
	entity.visible = true;
	EntitySetPosition(entity, desc.pos);
	entity.scale = desc.scale;
	entity.layer = desc.layer;
	entity.geometryType = desc.geometryType;
	entity.vertices = vertices;
	entity.indices = indices;
	entity.materialH = FindMaterialHandle(engine.gfx, desc.materialName);
	entity.spriteH = FindSpriteHandle(scene, desc.spriteName);

	return handle;
}

Handle CreateEntity(Engine &engine, const BinEntityDesc &desc)
{
	const EntityDesc entityDesc = {
		.name = desc.name,
		.materialName = desc.materialName,
		.geometryType = desc.geometryType,
		.spriteName = desc.spriteName,
		.layer = desc.layer,
		.pos = desc.pos,
		.scale = desc.scale,
	};
	return CreateEntity(engine, entityDesc);
}

void RemoveEntity(Engine &engine, Handle handle)
{
	Entity &entity = GetEntity(engine.scene, handle);
	entity = {};

	FreeHandle(engine.scene.entityHandles, handle);
}

Handle DuplicateEntity(Engine &engine, Handle entityHandle)
{
	const EntityDesc &desc = GetEntityDesc(engine.scene, entityHandle);
	return CreateEntity(engine, desc);
}


////////////////////////////////////////////////////////////////////////
// Tile grid management

float2 GetWorld2DCoord(const Engine &engine, const Camera &camera, int2 pixelCoord)
{
	const Window &window = *sPlatform->window;
	const uint2 windowSize = { window.width, window.height };
	const float2 uvCoords = {(f32)pixelCoord.x/windowSize.x, 1.0f - (f32)pixelCoord.y/windowSize.y};
	const float2 ndcCoords = 2.0f * uvCoords - float2{1.0f, 1.0f};
	const f32 aspect = (f32)windowSize.x / (f32)windowSize.y;
	const float2 scale = {camera.height * aspect, camera.height};
	const float2 worldCoords = scale * ndcCoords + float2{camera.position.x, camera.position.y};
	return worldCoords;
}

int2 GetGridTileCoord(const Engine &engine, const Camera &camera, int2 pixelCoord)
{
	const float2 worldCoords = GetWorld2DCoord(engine, camera, pixelCoord);
	const f32 cellWorldSize = TILE_SIZE_PIXELS / PIXELS_PER_METER;
	const int2 res = {(i32)Floor(worldCoords.x / cellWorldSize), (i32)Floor(worldCoords.y / cellWorldSize)};
	//LOG(Debug, "Tile coord: (%f, %f)\n", uvCoords.x, uvCoords.y);
	//LOG(Debug, "Tile coord: (%d, %d)\n", res.x, res.y);
	return res;
}

void SetGridTileAtCoord(Engine &engine, Layer &layer, u32 collider, int2 coord)
{
	const bool coordValid = coord.x >= 0 && coord.x < layer.size.x &&
			coord.y >= 0 && coord.y < layer.size.y;
	if (coordValid)
	{
		layer.cells[coord.x][coord.y].collider = collider;
	}
}

void SetGridTileAtCoord(Engine &engine, Layer &layer, SpriteH spriteH, int2 coord)
{
	const bool coordValid = coord.x >= 0 && coord.x < layer.size.y &&
			coord.y >= 0 && coord.y < layer.size.y;
	if (coordValid)
	{
		layer.cells[coord.x][coord.y].handle = spriteH;
	}
}

static int2 WorldPosToGridCoord(float2 worldPos)
{
	const f32 cellWorldSize = TILE_SIZE_PIXELS / PIXELS_PER_METER;
	return int2{ (i32)Floor(worldPos.x / cellWorldSize), (i32)Floor(worldPos.y / cellWorldSize) };
}

static u32 GetColliderAtGridCoord(Scene &scene, int2 coord)
{
	for (HandleIter it = BeginIter(scene.roomHandles); it; it++)
	{
		const Room &room = GetRoom(scene, *it);
		const int2 localCoord = coord - room.pos;

		for (u32 i = 0; i < ARRAY_COUNT(room.layers); ++i)
		{
			const Layer &layer = room.layers[i];
			if (!layer.initialized || !layer.isCollider) continue;

			if ( localCoord.x >= 0 && localCoord.x < layer.size.x &&
				localCoord.y >= 0 && localCoord.y < layer.size.y )
			{
				if (layer.cells[localCoord.x][localCoord.y].collider != 0)
				{
					return layer.cells[localCoord.x][localCoord.y].collider;
				}
			}
		}
	}
	return 0;
}

u32 GetColliderAtWorldPos(float2 worldPos)
{
	return GetColliderAtGridCoord(engine->scene, WorldPosToGridCoord(worldPos));
}

// pos is the box's bottom-left corner, size its width/height (same convention as DrawBox).
// Tiles the box only touches at an edge count as colliding.
bool IsColliderInBox(float2 pos, float2 size, u32 collider)
{
	const int2 minCoord = WorldPosToGridCoord(pos);
	const int2 maxCoord = WorldPosToGridCoord(pos + size);

	for (i32 y = minCoord.y; y <= maxCoord.y; ++y)
	{
		for (i32 x = minCoord.x; x <= maxCoord.x; ++x)
		{
			if (GetColliderAtGridCoord(engine->scene, int2{x, y}) == collider)
				return true;
		}
	}
	return false;
}


////////////////////////////////////////////////////////////////////////
// Immediate draw

// Appends a new debug-draw batch for imageH, or extends the last one if it
// already targets the same image, so consecutive same-texture draws are
// submitted as a single draw call.
static void DebugDrawAppendBatch(Graphics &gfx, ImageH imageH, u32 vertexCount)
{
	if ( gfx.debugDrawBatchCount > 0 )
	{
		DebugDrawBatch &lastBatch = gfx.debugDrawBatches[gfx.debugDrawBatchCount - 1];
		if ( lastBatch.imageH.index == imageH.index )
		{
			lastBatch.vertexCount += vertexCount;
			return;
		}
	}

	ASSERT( gfx.debugDrawBatchCount < MAX_DEBUG_DRAW_BATCHES );
	DebugDrawBatch &batch = gfx.debugDrawBatches[gfx.debugDrawBatchCount++];
	batch.imageH = imageH;
	batch.vertexIndex = gfx.debugDrawVertexCount - vertexCount;
	batch.vertexCount = vertexCount;
}

void DrawSprite(Handle spriteH, float2 worldPos, float4 pcolor)
{
	Graphics &gfx = engine->gfx;

	ASSERT( gfx.debugDrawVertexCount + 6 <= MAX_DEBUG_DRAW_VERTICES );

	const Sprite &sprite = GetSprite(engine->scene, spriteH);
	const Texture &texture = GetTexture(gfx, sprite.textureH);

	const float2 uvPos  = { (f32)sprite.pos.x / texture.size.x, (f32)sprite.pos.y / texture.size.y };
	const float2 uvSize = { (f32)sprite.size.x / texture.size.x, (f32)sprite.size.y / texture.size.y };
	const float2 worldSize = float2{ (f32)sprite.size.x, (f32)sprite.size.y } / PIXELS_PER_METER;
	const rgba color = Rgba(pcolor);

	DebugDrawVertex *v = gfx.debugDrawVerticesCPU + gfx.debugDrawVertexCount;
	v[0] = DebugDrawVertex{ worldPos + float2{0, 0},                     uvPos + float2{0, uvSize.y}, color };
	v[1] = DebugDrawVertex{ worldPos + float2{worldSize.x, worldSize.y}, uvPos + float2{uvSize.x, 0}, color };
	v[2] = DebugDrawVertex{ worldPos + float2{0, worldSize.y},           uvPos,                       color };
	v[3] = DebugDrawVertex{ worldPos + float2{0, 0},                     uvPos + float2{0, uvSize.y}, color };
	v[4] = DebugDrawVertex{ worldPos + float2{worldSize.x, 0},           uvPos + uvSize,               color };
	v[5] = DebugDrawVertex{ worldPos + float2{worldSize.x, worldSize.y}, uvPos + float2{uvSize.x, 0}, color };
	gfx.debugDrawVertexCount += 6;

	const ImageH imageH = GetTextureImage(gfx, sprite.textureH, gfx.pinkImageH);
	DebugDrawAppendBatch(gfx, imageH, 6);
}


////////////////////////////////////////////////////////////////////////
// Debug draw

void DrawBox(float2 pos, float2 size, float4 color)
{
	Graphics &gfx = engine->gfx;

	ASSERT( gfx.debugDrawVertexCount + 6 <= MAX_DEBUG_DRAW_VERTICES );
	const rgba c = Rgba(color);
	const float2 uv = {}; // Any UV works here, gfx.whiteImageH is a single white pixel.
	DebugDrawVertex *v = gfx.debugDrawVerticesCPU + gfx.debugDrawVertexCount;
	v[0] = DebugDrawVertex{ pos + float2{0, 0}, uv, c };
	v[1] = DebugDrawVertex{ pos + float2{size.x, size.y}, uv, c };
	v[2] = DebugDrawVertex{ pos + float2{0, size.y}, uv, c };
	v[3] = DebugDrawVertex{ pos + float2{0, 0}, uv, c };
	v[4] = DebugDrawVertex{ pos + float2{size.x, 0}, uv, c };
	v[5] = DebugDrawVertex{ pos + float2{size.x, size.y}, uv, c };
	gfx.debugDrawVertexCount += 6;

	DebugDrawAppendBatch(gfx, gfx.whiteImageH, 6);
}

void DrawBoxOutline(float2 pos, float2 size, float4 color)
{
	constexpr f32 d = 1.0f / PIXELS_PER_METER;
	DrawBox(pos, float2{size.x, d}, color);
	DrawBox(pos, float2{d, size.y}, color);
	DrawBox(pos + dX(size) - float2{d, 0}, float2{d, size.y}, color);
	DrawBox(pos + dY(size) - float2{0, d}, float2{size.x, d}, color);
}


////////////////////////////////////////////////////////////////////////
// Render targets

void CreateRenderTargets(Graphics &gfx, u32 sceneWidth = 0, u32 sceneHeight = 0)
{
	RenderTargets renderTargets = {};

	CommandList commandList = BeginTransientCommandList(gfx.device);

	const Format depthFormat = gfx.device.defaultDepthFormat;
	const u32 swapchainWidth = gfx.device.swapchain.extent.width;
	const u32 swapchainHeight = gfx.device.swapchain.extent.height;
	if (sceneWidth == 0) sceneWidth = swapchainWidth;
	if (sceneHeight == 0) sceneHeight = swapchainHeight;
	renderTargets.sceneSize = { sceneWidth, sceneHeight };

	// Depth buffer
	renderTargets.depthImage = CreateImage(gfx.device,
			sceneWidth, sceneHeight, 1,
			depthFormat,
			ImageUsageDepthStencilAttachment,
			HeapType_RTs);
	TransitionImageLayout(commandList, renderTargets.depthImage, ImageStateInitial, ImageStateRenderTarget, 0, 1);
	SetObjectNameImage(gfx.device, renderTargets.depthImage, "scene_depth");

	// Scene color buffer
	renderTargets.sceneImage = CreateImage(gfx.device,
		sceneWidth, sceneHeight, 1,
		gfx.device.swapchainInfo.format,
		ImageUsageColorAttachment | ImageUsageSampled,
		HeapType_RTs);
	TransitionImageLayout(commandList, renderTargets.sceneImage, ImageStateInitial, ImageStateRenderTarget, 0, 1);
	SetObjectNameImage(gfx.device, renderTargets.sceneImage, "scene_image");

	// Scene framebuffer
	{
		const FramebufferDesc desc = {
			.renderPass = gfx.litRenderPassH,
			.attachments = {
				renderTargets.sceneImage,
				renderTargets.depthImage,
			},
			.attachmentCount = 2,
		};

		renderTargets.sceneFramebuffer = CreateFramebuffer(gfx.device, desc);
	}

	// Display framebuffer
	for ( u32 i = 0; i < gfx.device.swapchain.imageCount; ++i )
	{
		const FramebufferDesc desc = {
			.renderPass = gfx.displayRenderPassH,
			.attachments = {
				gfx.device.swapchain.imageHandles[i],
			},
			.attachmentCount = 1,
		};

		renderTargets.displayFramebuffers[i] = CreateFramebuffer(gfx.device, desc);
	}

	// Shadowmap
	{
		renderTargets.shadowmapImage = CreateImage(gfx.device,
				1024, 1024, 1,
				depthFormat,
				ImageUsageDepthStencilAttachment | ImageUsageSampled,
				HeapType_RTs);
		TransitionImageLayout(commandList, renderTargets.shadowmapImage, ImageStateInitial, ImageStateRenderTarget, 0, 1);
		SetObjectNameImage(gfx.device, renderTargets.shadowmapImage, "scene_shadowmap");

		const FramebufferDesc desc = {
			.renderPass = gfx.shadowmapRenderPassH,
			.attachments = { renderTargets.shadowmapImage },
			.attachmentCount = 1,
		};

		renderTargets.shadowmapFramebuffer = CreateFramebuffer( gfx.device, desc );
	}

#if USE_EDITOR
	// ID buffer
	{
		renderTargets.idImage = CreateImage(gfx.device,
			 sceneWidth, sceneHeight, 1,
			 FormatUInt,
			 ImageUsageColorAttachment | ImageUsageSampled,
			 HeapType_RTs);
		TransitionImageLayout(commandList, renderTargets.idImage, ImageStateInitial, ImageStateRenderTarget, 0, 1);
		SetObjectNameImage(gfx.device, renderTargets.idImage, "scene_id");

		const FramebufferDesc desc = {
			.renderPass = gfx.idRenderPassH,
			.attachments = { renderTargets.idImage, renderTargets.depthImage },
			.attachmentCount = 2,
		};

		renderTargets.idFramebuffer = CreateFramebuffer( gfx.device, desc );
	}
#endif

	EndTransientCommandList(gfx.device, commandList);

	renderTargets.initialized = true;

	gfx.renderTargets = renderTargets;
	gfx.shouldUpdateGlobalBindGroups = true;
}

void DestroyRenderTargets(Graphics &gfx, RenderTargets &renderTargets)
{
	if ( !renderTargets.initialized )
	{
		return;
	}

	DestroyImageH(gfx.device, renderTargets.depthImage);
	DestroyImageH(gfx.device, renderTargets.sceneImage);

	// Reset the heap used for render targets
	Heap &rtHeap = gfx.device.heaps[HeapType_RTs];
	rtHeap.used = 0;

	DestroyFramebuffer( gfx.device, renderTargets.sceneFramebuffer );

	for ( u32 i = 0; i < gfx.device.swapchain.imageCount; ++i )
	{
		DestroyFramebuffer( gfx.device, renderTargets.displayFramebuffers[i] );
	}

	DestroyImageH(gfx.device, renderTargets.shadowmapImage);
	DestroyFramebuffer( gfx.device, renderTargets.shadowmapFramebuffer );

	DestroyImageH(gfx.device, renderTargets.idImage);
	DestroyFramebuffer( gfx.device, renderTargets.idFramebuffer );

	renderTargets = {};
}


////////////////////////////////////////////////////////////////////////
// Shaders and pipelines

static ShaderSource GetShaderSource(Arena &arena, const char *shaderName)
{
	char filename[MAX_PATH_LENGTH];
	SPrintf(filename, "shaders/%s.spv", shaderName);
	FilePath shaderPath = MakePath(DataDir, filename);
	DataChunk *chunk = PushFile( arena, shaderPath.str );
	if ( !chunk ) {
		LOG( Error, "Could not open shader file %s\n", shaderPath.str );
		QUIT_ABNORMALLY();
	}
	ShaderSource shaderSource = { chunk->bytes, chunk->size };
	return shaderSource;
}

static ShaderSource GetShaderSource(BinAssets &assets, const char *shaderName)
{
	byte *bytes = 0;
	u32 size = 0;
	for (u32 i = 0; i < assets.header.shaderCount; ++i)
	{
		BinShader &loadedShader = assets.shaders[i];
		if ( StrEq( loadedShader.desc->name, shaderName) )
		{
			bytes = loadedShader.spirv;
			size = loadedShader.desc->location.size;
		}
	}
	if ( !bytes ) {
		LOG( Error, "Could not find shader in assets: %s\n", shaderName );
		LOG( Error, "Shaders in assets:\n");
		for (u32 i = 0; i < assets.header.shaderCount; ++i)
		{
			BinShader &loadedShader = assets.shaders[i];
			LOG( Error, "- %s\n", loadedShader.desc->name);
		}
		QUIT_ABNORMALLY();
	}
	const ShaderSource shaderSource = { .data = bytes, .dataSize = size };
	return shaderSource;
}

static void CompileGraphicsPipeline(Engine &engine, Arena scratch, u32 pipelineIndex)
{
	Graphics &gfx = engine.gfx;

	const RenderPassH renderPassH = FindRenderPassHandle(gfx, pipelineDescs[pipelineIndex].renderPass);

	PipelineDesc desc = pipelineDescs[pipelineIndex].desc;
	desc.renderPass = GetRenderPass(gfx.device, renderPassH);

	if ( sLoadShadersFromText )
	{
		desc.vertexShaderSource = GetShaderSource(scratch, pipelineDescs[pipelineIndex].vsName);
		desc.fragmentShaderSource = GetShaderSource(scratch, pipelineDescs[pipelineIndex].fsName);
	}
	else
	{
		desc.vertexShaderSource = GetShaderSource(engine.shaderAssets, pipelineDescs[pipelineIndex].vsName);
		desc.fragmentShaderSource = GetShaderSource(engine.shaderAssets, pipelineDescs[pipelineIndex].fsName);
	}

	LOG(Info, "Creating Graphics Pipeline: %s\n", desc.name);
	PipelineH pipelineH = FindPipelineHandle(gfx, desc.name);
	if ( IsValid(pipelineH) ) {
		DestroyPipelineH( gfx.device, pipelineH );
	}
	pipelineH = CreateGraphicsPipeline(gfx.device, scratch, desc, gfx.globalBindGroupLayout);
	SetObjectNamePipeline(gfx.device, pipelineH, desc.name);
}

static void CompileComputePipeline(Engine &engine, Arena scratch, u32 pipelineIndex)
{
	Graphics &gfx = engine.gfx;

	ComputeDesc desc = computeDescs[pipelineIndex].desc;

	if ( sLoadShadersFromText )
	{
		desc.computeShaderSource = GetShaderSource(scratch, computeDescs[pipelineIndex].csName);
	}
	else
	{
		desc.computeShaderSource = GetShaderSource(engine.shaderAssets, computeDescs[pipelineIndex].csName);
	}

	LOG(Info, "Creating Compute Pipeline: %s\n", desc.name);
	PipelineH pipelineH = FindPipelineHandle(gfx, desc.name);
	if ( IsValid(pipelineH) ) {
		DestroyPipelineH( gfx.device, pipelineH );
	}
	pipelineH = CreateComputePipeline(gfx.device, scratch, desc, gfx.globalBindGroupLayout);
	SetObjectNamePipeline(gfx.device, pipelineH, desc.name);
}


////////////////////////////////////////////////////////////////////////
// Builtin handles

void LinkHandles(Graphics &gfx)
{
	// Textures
	gfx.skyTextureH = FindTextureHandle(gfx, "tex_sky");
	gfx.defaultTexture = FindTextureHandle(gfx, "tex_default");

	// Graphics pipelines
	gfx.shadowmapPipelineH = FindPipelineHandle(gfx, "pipeline_shadowmap");
	gfx.skyPipelineH = FindPipelineHandle(gfx, "pipeline_sky");
	gfx.spritePipelineH = FindPipelineHandle(gfx, "pipeline_shading_2d");
	gfx.tilePipelineH = FindPipelineHandle(gfx, "pipeline_shading_2d_tile");
	gfx.blitPipelineH = FindPipelineHandle(gfx, "pipeline_blit");
	gfx.guiPipelineH = FindPipelineHandle(gfx, "pipeline_ui");
#if USE_EDITOR
	gfx.grid2dPipelineH = FindPipelineHandle(gfx, "pipeline_grid_2d");
	gfx.grid3dPipelineH = FindPipelineHandle(gfx, "pipeline_grid_3d");
	gfx.modelIdPipelineH = FindPipelineHandle(gfx, "pipeline_model_id");
	gfx.spriteIdPipelineH = FindPipelineHandle(gfx, "pipeline_sprite_id");
#endif // USE_EDITOR
	gfx.debugDrawPipelineH = FindPipelineHandle(gfx, "pipeline_debug_draw");

	// Compute pipelines
#if USE_COMPUTE_TEST
	gfx.computeClearH = FindPipelineHandle(gfx, "compute_clear");
	gfx.computeUpdateH = FindPipelineHandle(gfx, "compute_update");
#endif // USE_COMPUTE_TEST
	gfx.computeSelectH = FindPipelineHandle(gfx, "compute_select");

	for (u32 i = 0; i < gfx.materialHandles.handleCount; ++i)
	{
		MaterialH handle = GetHandleAt(gfx.materialHandles, i);
		Material &material = GetMaterial(gfx, handle);
		material.pipelineH = FindPipelineHandle(gfx, material.pipelineName);
	}
}

static void RecompilePipelines(Engine &engine, Arena scratch)
{
	for (u32 i = 0; i < ARRAY_COUNT(pipelineDescs); ++i)
	{
		CompileGraphicsPipeline(engine, scratch, i);
	}

	// Compute pipelines
	for (u32 i = 0; i < ARRAY_COUNT(computeDescs); ++i)
	{
		CompileComputePipeline(engine, scratch, i);
	}
}

static void ResetDynamicBindGroups( Graphics &gfx )
{
	ResetBindGroupAllocator( gfx.device, gfx.dynamicBindGroupAllocator[gfx.device.frameIndex] );
	MemSet(gfx.dynamicBindGroupDescs, gfx.dynamicBindGroupCount * sizeof(BindGroupDesc), 0);
	gfx.dynamicBindGroupCount = 0;
}

static const BindGroup &GetOrCreateDynamicBindGroup(Graphics &gfx, const BindGroupDesc &bindGroupDesc)
{
	for (u32 i = 0; i < gfx.dynamicBindGroupCount; ++i)
	{
		if ( MemCompare( &gfx.dynamicBindGroupDescs[i], &bindGroupDesc, sizeof(BindGroupDesc) ) == 0 )
		{
			return gfx.dynamicBindGroups[i];
		}
	}

	ASSERT( gfx.dynamicBindGroupCount < ARRAY_COUNT(gfx.dynamicBindGroups) );
	const BindGroup bindGroup = CreateFullBindGroup(gfx.device, bindGroupDesc, gfx.dynamicBindGroupAllocator[gfx.device.frameIndex]);
	gfx.dynamicBindGroupDescs[gfx.dynamicBindGroupCount] = bindGroupDesc;
	gfx.dynamicBindGroups[gfx.dynamicBindGroupCount] = bindGroup;
	return gfx.dynamicBindGroups[gfx.dynamicBindGroupCount++];
}

bool InitializeGraphics(Engine &engine, Arena &globalArena)
{
	Scratch scratch;
	Graphics &gfx = engine.gfx;

	if ( !InitializeGraphicsDevice( gfx.device, scratch.arena ) ) {
		return false;
	}

	// Scene render pass
	{
		const Format format = gfx.device.swapchainInfo.format;

		const RenderpassDesc renderpassDesc = {
			.name = "scene_renderpass",
			.colorAttachmentCount = 1,
			.colorAttachments = {
				{ .format = format, .loadOp = LoadOpClear, .storeOp = StoreOpStore, .isSwapchain = false },
			},
			.hasDepthAttachment = true,
			.depthAttachment = {
				.loadOp = LoadOpClear, .storeOp = StoreOpStore,
			}
		};
		gfx.litRenderPassH = CreateRenderPass( gfx.device, renderpassDesc );
	}

	// Display render pass
	{
		const Format format = gfx.device.swapchainInfo.format;

		const RenderpassDesc renderpassDesc = {
			.name = "display_renderpass",
			.colorAttachmentCount = 1,
			.colorAttachments = {
				{ .format = format, .loadOp = LoadOpClear, .storeOp = StoreOpStore, .isSwapchain = true },
			},
			.hasDepthAttachment = false,
		};
		gfx.displayRenderPassH = CreateRenderPass( gfx.device, renderpassDesc );
	}

	// Shadowmap render pass
	{
		const RenderpassDesc renderpassDesc = {
			.name = "shadowmap_renderpass",
			.colorAttachmentCount = 0,
			.hasDepthAttachment = true,
			.depthAttachment = {
				.loadOp = LoadOpClear, .storeOp = StoreOpStore
			}
		};
		gfx.shadowmapRenderPassH = CreateRenderPass( gfx.device, renderpassDesc );
	}

	// ID render pass
	{
		const RenderpassDesc renderpassDesc = {
			.name = "id_renderpass",
			.colorAttachmentCount = 1,
			.colorAttachments = {
				{ .format = FormatUInt, .loadOp = LoadOpClear, .storeOp = StoreOpStore },
			},
			.hasDepthAttachment = true,
			.depthAttachment = {
				.loadOp = LoadOpLoad, .storeOp = StoreOpStore
			}
		};
		gfx.idRenderPassH = CreateRenderPass( gfx.device, renderpassDesc );
	}

	// Create staging buffer
	gfx.stagingBuffer = CreateStagingBuffer(gfx);

	// Create global geometry buffers
	gfx.globalVertexArena = MakeBufferArena( gfx, CreateVertexBuffer(gfx, MB(4)) );
	gfx.globalIndexArena = MakeBufferArena( gfx, CreateIndexBuffer(gfx, MB(4)) );

	// Create debug draw vertex buffers
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		gfx.debugDrawVertexBuffer[i] = CreateBuffer(
			gfx.device,
			sizeof(DebugDrawVertex) * MAX_DEBUG_DRAW_VERTICES,
			BufferUsageVertexBuffer,
			HeapType_Dynamic);

		gfx.debugDrawVertices[i] = (DebugDrawVertex*)GetBufferPtr(gfx.device, gfx.debugDrawVertexBuffer[i]);
	}
	gfx.debugDrawVerticesCPU = PushArray(globalArena, DebugDrawVertex, MAX_DEBUG_DRAW_VERTICES);

	CommandList commandList = BeginUploadCommandList(gfx);

	// Create vertex/index buffers
	gfx.cubeVertices = PushData(gfx, commandList, gfx.globalVertexArena, cubeVertices, sizeof(cubeVertices));
	gfx.cubeIndices = PushData(gfx, commandList, gfx.globalIndexArena, cubeIndices, sizeof(cubeIndices));
	gfx.planeVertices = PushData(gfx, commandList, gfx.globalVertexArena, planeVertices, sizeof(planeVertices));
	gfx.planeIndices = PushData(gfx, commandList, gfx.globalIndexArena, planeIndices, sizeof(planeIndices));
	gfx.quadVertices = PushData(gfx, commandList, gfx.globalVertexArena, quadVertices, sizeof(quadVertices));
	gfx.quadIndices = PushData(gfx, commandList, gfx.globalIndexArena, quadIndices, sizeof(quadIndices));
	gfx.spriteVertices = PushData(gfx, commandList, gfx.globalVertexArena, spriteVertices, sizeof(spriteVertices));
	gfx.spriteIndices = PushData(gfx, commandList, gfx.globalIndexArena, spriteIndices, sizeof(spriteIndices));
	gfx.screenTriangleVertices = PushData(gfx, commandList, gfx.globalVertexArena, screenTriangleVertices, sizeof(screenTriangleVertices));
	gfx.screenTriangleIndices = PushData(gfx, commandList, gfx.globalIndexArena, screenTriangleIndices, sizeof(screenTriangleIndices));

	EndUploadCommandList(gfx, commandList);

	// Create globals buffer
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		const u32 globalsBufferSize = sizeof(Globals);
		gfx.globalsBuffer[i] = CreateBuffer(
			gfx.device,
			globalsBufferSize,
			BufferUsageUniformBuffer,
			HeapType_Dynamic);
	}

	// Create entities buffer
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		const u32 entityBufferSize = MAX_ENTITIES * AlignUp( sizeof(SEntity), gfx.device.alignment.uniformBufferOffset );
		gfx.entityBuffer[i] = CreateBuffer(
			gfx.device,
			entityBufferSize,
			BufferUsageStorageBuffer,
			HeapType_Dynamic);
	}

	// Create sprite data buffer
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		const u32 spriteDataBufferSize = (MAX_SPRITES) * sizeof(SSpriteData);
		gfx.spriteDataBuffer[i] = CreateBuffer(
			gfx.device,
			spriteDataBufferSize,
			BufferUsageStorageBuffer,
			HeapType_Dynamic);
	}

	// Create tile data buffer
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		const u32 tileDataBufferSize = (MAX_TILES) * sizeof(STileData);
		gfx.tileDataBuffer[i] = CreateBuffer(
			gfx.device,
			tileDataBufferSize,
			BufferUsageStorageBuffer,
			HeapType_Dynamic);
	}


	// Create material buffer
	const u32 materialBufferSize = MAX_MATERIALS * AlignUp( sizeof(SMaterial), gfx.device.alignment.uniformBufferOffset );
	gfx.materialBuffer = CreateBuffer(gfx.device, materialBufferSize, BufferUsageUniformBuffer | BufferUsageTransferDst, HeapType_General);

	// Create buffer for computes
	const u32 computeBufferSize = sizeof(float);
	gfx.computeBufferH = CreateBuffer(gfx.device, computeBufferSize, BufferUsageStorageTexelBuffer, HeapType_General);
	gfx.computeBufferViewH = CreateBufferView(gfx.device, gfx.computeBufferH, FormatFloat, 0, 0);

#if USE_EDITOR
	const u32 selectionBufferSize = AlignUp( sizeof(u32), gfx.device.alignment.storageBufferOffset );
	gfx.selectionBufferH = CreateBuffer(gfx.device, selectionBufferSize, BufferUsageStorageTexelBuffer, HeapType_Readback);
	gfx.selectionBufferViewH = CreateBufferView(gfx.device, gfx.selectionBufferH, FormatUInt, 0, 0);
#endif // USE_EDITOR


	// Create Global BindGroup allocator
	{
		const BindGroupAllocatorDesc desc = {
			.name = "GlobalBindGroup",
			.counts = {
				.uniformBufferCount = MAX_FRAMES_IN_FLIGHT,
				.storageBufferCount = MAX_FRAMES_IN_FLIGHT * 3,
				.textureCount = 1000,
				.samplerCount = MAX_FRAMES_IN_FLIGHT * 5,
				.groupCount = MAX_FRAMES_IN_FLIGHT,
			},
		};
		gfx.globalBindGroupAllocator = CreateBindGroupAllocator(gfx.device, desc);
	}

	// Create Material BindGroup allocator
	{
		const BindGroupAllocatorDesc desc = {
			.name = "MaterialBindGroup",
			.counts = {
				.uniformBufferCount = MAX_MATERIALS,
				.textureCount = MAX_MATERIALS,
				.groupCount = MAX_MATERIALS,
			},
		};
		gfx.materialBindGroupAllocator = CreateBindGroupAllocator(gfx.device, desc);
	}

	// Create dynamic per-frame BindGroup allocator
	for (u32 i = 0; i < ARRAY_COUNT(gfx.dynamicBindGroupAllocator); ++i)
	{
		const BindGroupAllocatorDesc desc = {
			.name = "DynamicBindGroup",
			.counts = {
				.uniformBufferCount = 1000,
				.storageBufferCount = 1000,
				.storageTexelBufferCount = 1000,
				.textureCount = 1000,
				.samplerCount = 1000,
				.groupCount = 100,
			},
		};
		gfx.dynamicBindGroupAllocator[i] = CreateBindGroupAllocator(gfx.device, desc);
	}

	// Create global BindGroup layout

	const ShaderBinding globalShaderBindings[] = {
		{ .set = 0, .binding = BINDING_GLOBALS, .type = SpvTypeUniformBuffer, .stageFlags = SpvStageFlagsVertexBit | SpvStageFlagsFragmentBit | SpvStageFlagsComputeBit },
		{ .set = 0, .binding = BINDING_SAMPLER_POINT, .type = SpvTypeSampler, .stageFlags = SpvStageFlagsFragmentBit },
		{ .set = 0, .binding = BINDING_SAMPLER_LINEAR, .type = SpvTypeSampler, .stageFlags = SpvStageFlagsFragmentBit },
		{ .set = 0, .binding = BINDING_ENTITIES, .type = SpvTypeStorageBuffer, .stageFlags = SpvStageFlagsVertexBit },
		{ .set = 0, .binding = BINDING_SHADOWMAP, .type = SpvTypeImage, .stageFlags = SpvStageFlagsFragmentBit },
		{ .set = 0, .binding = BINDING_SHADOWMAP_SAMPLER, .type = SpvTypeSampler, .stageFlags = SpvStageFlagsFragmentBit },
		{ .set = 0, .binding = BINDING_SPRITE_DATA, .type = SpvTypeStorageBuffer, .stageFlags = SpvStageFlagsVertexBit },
		{ .set = 0, .binding = BINDING_TILE_DATA, .type = SpvTypeStorageBuffer, .stageFlags = SpvStageFlagsVertexBit },
	};
	gfx.globalBindGroupLayout = CreateBindGroupLayout(gfx.device, globalShaderBindings, ARRAY_COUNT(globalShaderBindings));

	// Handle managers
	Initialize(gfx.textureHandles, globalArena, MAX_TEXTURES);
	Initialize(gfx.materialHandles, globalArena, MAX_MATERIALS);

	// Graphics pipelines
	ResetArena(scratch.arena);
	RecompilePipelines(engine, scratch.arena);

	// Builtin images
	const byte whiteImagePixels[] = { 255, 255, 255, 255 };
	gfx.whiteImageH = EngineCreateImage(gfx, "whiteImage", 1, 1, 4, false, whiteImagePixels);
	const byte pinkImagePixels[] = { 255, 0, 255, 255 };
	gfx.pinkImageH = EngineCreateImage(gfx, "pinkImage", 1, 1, 4, false, pinkImagePixels);
	const byte grayImagePixels[] = { 127, 127, 127, 255 };
	gfx.grayImageH = EngineCreateImage(gfx, "grayImage", 1, 1, 4, false, grayImagePixels);
	const byte blackImagePixels[] = { 0, 0, 0, 0 };
	gfx.blackImageH = EngineCreateImage(gfx, "blackImage", 1, 1, 4, false, blackImagePixels);

	// Builtin texture
	const TextureDesc textureDesc = {
		.name = InternString("tex_default"),
		.filename = InternString(""),
		.mipmap = 0,
		.flags = AssetFlag_Builtin,
	};
	gfx.defaultTexture = CreateTexture(gfx, textureDesc, gfx.pinkImageH);

	// Builtin material
	const MaterialDesc materialDesc = {
		.name = InternString("mat_default"),
		.textureName = InternString("tex_default"),
		.pipelineName = InternString("pipeline_shading"),
		.uvScale = 1.0,
		.flags = AssetFlag_Builtin,
	};
	gfx.defaultMaterial = CreateMaterial(gfx, materialDesc);

	// Samplers
	const SamplerDesc pointSamplerDesc = {
		.addressMode = AddressModeRepeat,
		.filter = FilterNearest,
	};
	gfx.pointSamplerH = CreateSampler(gfx.device, pointSamplerDesc);
	const SamplerDesc materialSamplerDesc = {
		.addressMode = AddressModeRepeat,
		.filter = FilterLinear,
	};
	gfx.linearSamplerH = CreateSampler(gfx.device, materialSamplerDesc);
	const SamplerDesc screenSamplerDesc = {
		.addressMode = AddressModeClampToEdge,
		.filter = FilterNearest,
	};
	gfx.screenSamplerH = CreateSampler(gfx.device, screenSamplerDesc);
	const SamplerDesc shadowmapSamplerDesc = {
		.addressMode = AddressModeClampToBorder,
		.filter = FilterNearest,
		.borderColor = BorderColorBlackFloat,
		.compareOp = CompareOpGreater,
	};
	gfx.shadowmapSamplerH = CreateSampler(gfx.device, shadowmapSamplerDesc);
	const SamplerDesc skySamplerDesc = {
		.addressMode = AddressModeClampToEdge,
		.filter = FilterLinear,
	};
	gfx.skySamplerH = CreateSampler(gfx.device, skySamplerDesc);

	// BindGroups for globals
	for (u32 i = 0; i < ARRAY_COUNT(gfx.globalBindGroups); ++i)
	{
		gfx.globalBindGroups[i] = CreateBindGroup(gfx.device, gfx.globalBindGroupLayout, gfx.globalBindGroupAllocator);
	}

	gfx.shouldUpdateGlobalBindGroups = true;

	// Timestamp queries
	for (u32 i = 0; i < ARRAY_COUNT(gfx.timestampPools); ++i)
	{
		gfx.timestampPools[i] = CreateTimestampPool(gfx.device, 128);
		//ResetTimestampPool(gfx.device, gfx.timestampPools[i]); // Vulkan 1.2
	}

	LinkHandles(gfx);

#if USE_UI
	UIIcon *icons = nullptr;
	u32 iconCount = 0;
#if USE_EDITOR
	ResetArena(scratch.arena);
	const char *iconFilenames[] = { "editor/play_16.png", "editor/open.png", "editor/save.png", "editor/reload.png" };
	iconCount = ARRAY_COUNT(iconFilenames);
	icons = PushArray(globalArena, UIIcon, iconCount);
	for (u32 i = 0; i < iconCount; ++i) {
		FilePath filepath = MakePath(ProjectDir, iconFilenames[i]);
		ReadImagePixels(scratch.arena, filepath.str, icons[i].image);
	}
#endif
	UI_Initialize(engine.ui, gfx, gfx.device, globalArena, icons, iconCount);
#endif

	gfx.deviceInitialized = true;

	return true;
}

BindGroupDesc GlobalBindGroupDesc(const Graphics &gfx, u32 frameIndex)
{
	const BindGroupDesc bindGroupDesc = {
		.layout = gfx.globalBindGroupLayout,
		.bindings = {
			{ .index = BINDING_GLOBALS, .buffer = gfx.globalsBuffer[frameIndex] },
			{ .index = BINDING_SAMPLER_POINT, .sampler = gfx.pointSamplerH },
			{ .index = BINDING_SAMPLER_LINEAR, .sampler = gfx.linearSamplerH },
			{ .index = BINDING_ENTITIES, .buffer = gfx.entityBuffer[frameIndex] },
			{ .index = BINDING_SHADOWMAP, .image = gfx.renderTargets.shadowmapImage },
			{ .index = BINDING_SHADOWMAP_SAMPLER, .sampler = gfx.shadowmapSamplerH },
			{ .index = BINDING_SPRITE_DATA, .buffer = gfx.spriteDataBuffer[frameIndex] },
			{ .index = BINDING_TILE_DATA, .buffer = gfx.tileDataBuffer[frameIndex] },
		},
	};
	return bindGroupDesc;
}

void UpdateGlobalBindGroups(Graphics &gfx)
{
	for (u32 i = 0; i < ARRAY_COUNT(gfx.globalBindGroups); ++i)
	{
		const BindGroupDesc globalBindGroupDesc = GlobalBindGroupDesc(gfx, i);
		UpdateBindGroup(gfx.device, globalBindGroupDesc, gfx.globalBindGroups[i]);
	}
}

BindGroupDesc MaterialBindGroupDesc(Graphics &gfx, const Material &material)
{
	const Pipeline &pipeline = GetPipeline(gfx.device, material.pipelineH);
	const BindGroupLayout &bindGroupLayout = pipeline.layout.bindGroupLayouts[BIND_GROUP_MATERIAL];
	const ImageH &albedoImage = GetTextureImage(gfx, material.albedoTexture, gfx.pinkImageH );

	const BindGroupDesc bindGroupDesc = {
		.layout = bindGroupLayout,
		.bindings = {
			{ .index = BINDING_MATERIAL, .buffer = gfx.materialBuffer, .offset = material.bufferOffset, .range = sizeof(SMaterial) },
			{ .index = BINDING_ALBEDO, .image = albedoImage },
		},
	};
	return bindGroupDesc;
}

void UpdateMaterialBindGroups(Graphics &gfx)
{
	for (u32 materialIndex = 0; materialIndex < gfx.materialHandles.handleCount; ++materialIndex)
	{
		const MaterialH handle = GetHandleAt(gfx.materialHandles, materialIndex);
		const Material &material = GetMaterial(gfx, handle);
		const BindGroupDesc materialBindGroupDesc = MaterialBindGroupDesc(gfx, material);
		UpdateBindGroup(gfx.device, materialBindGroupDesc, gfx.materialBindGroups[handle.idx]);
	}
}

void UploadMaterialData(Graphics &gfx)
{
	CommandList commandList = BeginUploadCommandList(gfx);

	// Copy material info to buffer
	for (u32 i = 0; i < gfx.materialHandles.handleCount; ++i)
	{
		MaterialH handle = GetHandleAt(gfx.materialHandles, i);
		const Material &material = GetMaterial(gfx, handle);
		SMaterial shaderMaterial = { material.uvScale };
		StagedData staged = StageData(gfx, &shaderMaterial, sizeof(shaderMaterial));

		CopyBufferToBuffer(commandList, staged.buffer, staged.offset, gfx.materialBuffer, material.bufferOffset, sizeof(shaderMaterial));
	}

	EndUploadCommandList(gfx, commandList);
}

void CreateMaterialBindGroup(Graphics &gfx, MaterialH handle)
{
	const Material &material = GetMaterial(gfx, handle);
	const Pipeline &pipeline = GetPipeline(gfx.device, material.pipelineH);
	gfx.materialBindGroups[handle.idx] = CreateBindGroup(gfx.device, pipeline.layout.bindGroupLayouts[1], gfx.materialBindGroupAllocator);
	gfx.shouldUpdateMaterialBindGroups = true;
}

void CreateMaterialBindGroups(Graphics &gfx)
{
	// BindGroups for materials
	for (u32 i = 0; i < gfx.materialHandles.handleCount; ++i)
	{
		MaterialH handle = GetHandleAt(gfx.materialHandles, i);
		CreateMaterialBindGroup(gfx, handle);
	}
}

void EngineWaitDeviceIdle(Graphics &gfx)
{
	WaitDeviceIdle(gfx.device);

	gfx.stagingBufferOffset = 0;
}

void CleanupGraphics(Graphics &gfx)
{
	EngineWaitDeviceIdle( gfx );

	for (u32 i = 0; i < ARRAY_COUNT(gfx.timestampPools); ++i)
	{
		DestroyTimestampPool(gfx.device, gfx.timestampPools[i]);
	}

	DestroyBindGroupAllocator( gfx.device, gfx.globalBindGroupAllocator );
	DestroyBindGroupAllocator( gfx.device, gfx.materialBindGroupAllocator );
	for (u32 i = 0; i < ARRAY_COUNT(gfx.dynamicBindGroupAllocator); ++i)
	{
		DestroyBindGroupAllocator( gfx.device, gfx.dynamicBindGroupAllocator[i] );
	}

	CleanupGraphicsDevice( gfx.device, FrameArena );

	CleanupGraphicsDriver( gfx.device );

	ZeroStruct( &gfx ); // deviceInitialized = false;
}

AssetDescriptors GetAssetDescriptors(Engine &engine, Arena &arena)
{
	static TextureDesc textureDescs[MAX_TEXTURES];
	u32 textureCount = 0;
	for (HandleIter it = BeginIter(engine.gfx.textureHandles); it; it++) {
		textureDescs[textureCount] = GetTextureDesc(engine.gfx, *it);
		if ( !( textureDescs[textureCount].flags & AssetFlag_Builtin ) ) {
			textureCount++;
		}
	}

	static SpriteDesc spriteDescs[MAX_SPRITES];
	static u16 spriteIndexByHandleIdx[MAX_SPRITES];
	u32 spriteCount = 0;
	for (HandleIter it = BeginIter(engine.scene.spriteHandles); it; it++) {
		const SpriteH handle = *it;
		spriteIndexByHandleIdx[handle.idx] = (u16)spriteCount;
		spriteDescs[spriteCount++] = GetSpriteDesc(engine.scene, handle);
	}

	static MaterialDesc materialDescs[MAX_MATERIALS];
	u32 materialCount = 0;
	for (HandleIter it = BeginIter(engine.gfx.materialHandles); it; it++) {
		materialDescs[materialCount] = GetMaterialDesc(engine.gfx, *it);
		if ( !( materialDescs[materialCount].flags & AssetFlag_Builtin ) ) {
			materialCount++;
		}
	}

	static EntityDesc entityDescs[MAX_ENTITIES];
	u32 entityCount = 0;
	for (HandleIter it = BeginIter(engine.scene.entityHandles); it; it++) {
		entityDescs[entityCount++] = GetEntityDesc(engine.scene, *it);
	}

	static RoomDesc roomDescs[MAX_ROOMS];
	u32 roomCount = 0;
	for (HandleIter it = BeginIter(engine.scene.roomHandles); it; it++) {
		const Room &room = GetRoom(engine.scene, *it);
		RoomDesc &desc = roomDescs[roomCount++];
		desc = {};
		desc.name = room.name;
		desc.pos = room.pos;
		for (u32 l = 0; l < ARRAY_COUNT(room.layers); ++l) {
			const Layer &layer = room.layers[l];
			if (!layer.initialized) {
				continue;
			}
			LayerDesc &layerDesc = desc.layers[desc.layerCount++];
			layerDesc.name = layer.name;
			layerDesc.order = layer.order;
			layerDesc.visible = layer.visible;
			layerDesc.isCollider = layer.isCollider;
			layerDesc.size = layer.size;
			layerDesc.tiles = (TileDesc*)(arena.base + arena.used);
			layerDesc.tileCount = 0;
			if (layer.isCollider)
			{
				for (u32 x = 0; x < layer.size.x; ++x) {
					for (u32 y = 0; y < layer.size.y; ++y) {
						const u16 collider = (u16)layer.cells[x][y].collider;
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
						const SpriteH spriteH = layer.cells[x][y].handle;
						if (spriteH == InvalidHandle) continue;
						if (!IsValidHandle(engine.scene.spriteHandles, spriteH)) continue;
						TileDesc &tile = *PushStruct(arena, TileDesc);
						tile.x = (u16)x;
						tile.y = (u16)y;
						tile.spriteIndex = spriteIndexByHandleIdx[spriteH.idx];
						layerDesc.tileCount++;
					}
				}
			}
		}
	}

	static AudioClipDesc audioClipDescs[MAX_AUDIO_CLIPS];
	u32 audioClipCount = 0;
	for (HandleIter it = BeginIter(engine.audio.clipHandles); it; it++) {
		audioClipDescs[audioClipCount] = GetAudioClipDesc(engine.audio, *it);
	}

	const AssetDescriptors assetDescs = {
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
		.roomDescs = roomDescs,
		.roomDescCount = roomCount,
		.audioClipDescs = audioClipDescs,
		.audioClipDescCount = audioClipCount,
	};

	return assetDescs;
}

static bool PushDataArenaState(Engine &engine)
{
	const bool ok = engine.dataArenaStateCount < ARRAY_COUNT(engine.dataArenaStates);
	if (ok)
	{
		engine.dataArenaStates[engine.dataArenaStateCount++] = DataArena;
	}
	return ok;
}

static bool PopDataArenaState(Engine &engine)
{
	const bool ok = engine.dataArenaStateCount > 0;
	if (ok)
	{
		DataArena = engine.dataArenaStates[--engine.dataArenaStateCount];
	}
	return ok;
}

// Defined later in this file (room management)
Handle CreateRoom(Engine &engine, const RoomDesc &desc, const SpriteH *spriteHandles, u32 spriteHandleCount);
Handle CreateRoom(Engine &engine, const BinRoom &binRoom, const SpriteH *spriteHandles, u32 spriteHandleCount);
void RemoveRoom(Engine &engine, Handle handle);

static void CleanRoom(Handle handle, void *data)
{
	Engine &engine = *(Engine*)data;
	RemoveRoom(engine, handle);
}

#if USE_DATA_BUILD
void LoadSceneFromTxt(Engine &engine, const char *filepath)
{
	if ( PushDataArenaState(engine) )
	{
		Arena &dataArena = DataArena;
		AssetDescriptors assetDescriptors = ParseDescriptors(filepath, dataArena);

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

		// Sprites (must be before entities and rooms)
		static SpriteH spriteHandles[MAX_SPRITES];
		for (u32 i = 0; i < assetDescriptors.spriteDescCount; ++i)
		{
			spriteHandles[i] = CreateSprite(engine, assetDescriptors.spriteDescs[i]);
		}

		// Entities
		for (u32 i = 0; i < assetDescriptors.entityDescCount; ++i)
		{
			CreateEntity(engine, assetDescriptors.entityDescs[i]);
		}

		// Rooms (layer cells reference sprites by index into the sprite list)
		if (assetDescriptors.roomDescCount > 0)
		{
			for (u32 i = 0; i < assetDescriptors.roomDescCount; ++i)
			{
				CreateRoom(engine, assetDescriptors.roomDescs[i], spriteHandles, assetDescriptors.spriteDescCount);
			}
		}

		// Audio clips
		for (u32 i = 0; i < assetDescriptors.audioClipDescCount; ++i)
		{
			CreateAudioClip(engine, assetDescriptors.audioClipDescs[i]);
		}

		// Music files
		for (u32 i = 0; i < assetDescriptors.musicFileDescCount; ++i)
		{
			CreateMusicFile(engine, assetDescriptors.musicFileDescs[i]);
		}

		UploadMaterialData(engine.gfx);
		LinkHandles(engine.gfx);
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

		// Sprites (must be before entities and rooms)
		static SpriteH spriteHandles[MAX_SPRITES];
		for (u32 i = 0; i < engine.assets.header.spriteCount; ++i)
		{
			spriteHandles[i] = CreateSprite(engine, *engine.assets.sprites[i].desc);
		}

		// Entities
		for (u32 i = 0; i < engine.assets.header.entityCount; ++i)
		{
			CreateEntity(engine, *engine.assets.entities[i].desc);
		}

		// Rooms (layer cells reference sprites by index into the sprite list)
		if (engine.assets.header.roomCount > 0)
		{
			for (u32 i = 0; i < engine.assets.header.roomCount; ++i)
			{
				CreateRoom(engine, engine.assets.rooms[i], spriteHandles, engine.assets.header.spriteCount);
			}
		}

		// Audio clips
		for (u32 i = 0; i < engine.assets.header.audioClipCount; ++i)
		{
			CreateAudioClip(engine, engine.assets.audioClips[i]);
		}

		// Music files
		for (u32 i = 0; i < engine.assets.header.musicFileCount; ++i)
		{
			CreateMusicFile(engine, engine.assets.musicFiles[i]);
		}

		UploadMaterialData(engine.gfx);
		LinkHandles(engine.gfx);
	}
}

void CleanTexture(Handle handle, void* data)
{
	Engine &engine = *(Engine*)data;
	const TextureDesc &desc = GetTextureDesc( engine.gfx, handle);
	if ( !(desc.flags & AssetFlag_Builtin) ) {
		RemoveTexture(engine.gfx, handle);
	}
}

void CleanMaterial(Handle handle, void* data)
{
	Engine &engine = *(Engine*)data;
	const MaterialDesc &desc = GetMaterialDesc( engine.gfx, handle);
	if ( !(desc.flags & AssetFlag_Builtin) ) {
		RemoveMaterial(engine.gfx, handle);
	}
}

void CleanEntity(Handle handle, void* data)
{
	Engine &engine = *(Engine*)data;
	RemoveEntity(engine, handle);
}

void CleanSprite(Handle handle, void* data)
{
	Engine &engine = *(Engine*)data;
	RemoveSprite(engine.scene, handle);
}


void CleanAudioClip(Handle handle, void* data)
{
	Engine &engine = *(Engine*)data;
	RemoveAudioClip(engine, handle);
}

u32 CreateLayer(Room &room, const LayerDesc &desc)
{
	u32 index = U32_MAX;

	if ( room.layerCount < ARRAY_COUNT(room.layers) )
	{
		for (u32 i = 0; i < ARRAY_COUNT(room.layers); ++i)
		{
			Layer &layer = room.layers[i];
			if (!layer.initialized)
			{
				room.layerCount++;
				layer.initialized = true;
				layer.name = desc.name;
				layer.order = desc.order;
				layer.visible = desc.visible;
				layer.isCollider = desc.isCollider;
				layer.size = desc.size;
				index = i;
				break;
			}
		}
	}

	return index;
}

void RemoveLayer(Room &room, u32 index)
{
	if ( room.layerCount > 1 && index < ARRAY_COUNT(room.layers) )
	{
		Layer &layer = room.layers[index];
		if (layer.initialized)
		{
			layer = {};
			room.layerCount--;
		}
	}
}

float2 LayerSize(const Layer &layer)
{
	const float2 res = Float2(layer.size);
	return res;
}

float2 RoomSize(const Room &room)
{
	const float2 res = LayerSize(room.layers[0]);
	return res;
}

Handle CreateRoom(Engine &engine)
{
	Handle roomH = NewHandle(engine.scene.roomHandles);
	Room &room = engine.scene.rooms[roomH.idx];

	room.name = InternString("Room");
	room.pos = {};

	const LayerDesc desc1 = { .name = "Layer", .visible = true, .isCollider = false, .size {TILE_GRID_SIZE_X, TILE_GRID_SIZE_Y} };
	CreateLayer(room, desc1);
	const LayerDesc desc2 = { .name = "Colliders", .visible = true, .isCollider = true, .size {TILE_GRID_SIZE_X, TILE_GRID_SIZE_Y} };
	CreateLayer(room, desc2);

	return roomH;
}

Handle CreateRoom(Engine &engine, const RoomDesc &desc, const SpriteH *spriteHandles, u32 spriteHandleCount)
{
	Handle roomH = NewHandle(engine.scene.roomHandles);
	Room &room = engine.scene.rooms[roomH.idx];
	room = {};

	room.name = InternString(desc.name);
	room.pos = desc.pos;

	for (u32 l = 0; l < desc.layerCount; ++l)
	{
		LayerDesc layerDesc = desc.layers[l];
		layerDesc.name = InternString(layerDesc.name);

		const u32 index = CreateLayer(room, layerDesc);
		if (index == U32_MAX) {
			continue;
		}

		Layer &layer = room.layers[index];
		if (layerDesc.isCollider) {
			for (u32 t = 0; t < layerDesc.tileCount; ++t) {
				const TileDesc &tile = layerDesc.tiles[t];
				if (tile.x < layer.size.x && tile.y < layer.size.y) {
					layer.cells[tile.x][tile.y].collider = tile.collider;
				}
			}
		}
		else
		{
			for (u32 t = 0; t < layerDesc.tileCount; ++t) {
				const TileDesc &tile = layerDesc.tiles[t];
				if (tile.x < layer.size.x && tile.y < layer.size.y) {
					if (tile.spriteIndex < spriteHandleCount) {
						layer.cells[tile.x][tile.y].handle = spriteHandles[tile.spriteIndex];
					}
				}
			}
		}
	}

	return roomH;
}

Handle CreateRoom(Engine &engine, const BinRoom &binRoom, const SpriteH *spriteHandles, u32 spriteHandleCount)
{
	const BinRoomDesc &bin = *binRoom.desc;

	RoomDesc desc = {};
	desc.name = bin.name;
	desc.pos = bin.pos;

	for (u32 l = 0; l < bin.layerCount && l < ARRAY_COUNT(desc.layers); ++l)
	{
		const BinLayerDesc &ld = bin.layers[l];
		desc.layers[desc.layerCount++] = {
			.name = ld.name,
			.order = ld.order,
			.visible = ld.visible != 0,
			.isCollider = ld.isCollider != 0,
			.size = ld.size,
			.tiles = binRoom.tiles[l],
			.tileCount = ld.tiles.size / sizeof(TileDesc),
		};
	}

	return CreateRoom(engine, desc, spriteHandles, spriteHandleCount);
}

void RemoveRoom(Engine &engine, Handle handle)
{
	Room &room = GetRoom(engine.scene, handle);
	room = {};
	FreeHandle(engine.scene.roomHandles, handle);
}

void CreateScene(Engine &engine)
{
	CreateRoom(engine);
}

void CleanScene(Engine &engine)
{
	WaitDeviceIdle(engine.gfx.device);

	if (PopDataArenaState(engine))
	{
	}

	ForAllHandles(engine.gfx.textureHandles, CleanTexture, &engine);
	ForAllHandles(engine.gfx.materialHandles, CleanMaterial, &engine);
	ForAllHandles(engine.scene.roomHandles, CleanRoom, &engine);
	ForAllHandles(engine.scene.entityHandles, CleanEntity, &engine);
	ForAllHandles(engine.scene.spriteHandles, CleanSprite, &engine);
	ForAllHandles(engine.audio.clipHandles, CleanAudioClip, &engine);
	CloseAssets(engine.assets);

	engine.gfx.shouldUpdateMaterials = true;
	engine.gfx.shouldUpdateMaterialBindGroups = true;
}

#if USE_DATA_BUILD
void BuildShaders(Engine &engine, const char *outBinFilepath)
{
	CompileShaders();

	Arena scratch = MakeSubArena(DataArena, "Scratch - BuildShaders");
	AssetDescriptors assetDescriptors = {};
	assetDescriptors.shaderDescs = shaderSourceDescs;
	assetDescriptors.shaderDescCount = ARRAY_COUNT(shaderSourceDescs);
	BuildAssets(assetDescriptors, outBinFilepath, scratch);
}

void BuildAssetsFromTxt(Engine &engine, const char *inTxtFilepath, const char *outBinFilepath)
{
	Arena scratch = MakeSubArena(DataArena, "Scratch - BuildAssetsFromTxt");
	AssetDescriptors assetDescriptors = ParseDescriptors(inTxtFilepath, scratch);
	BuildAssets(assetDescriptors, outBinFilepath, scratch);
}
#endif // USE_DATA_BUILD

float3 UpDirectionFromAngles(const float2 &angles)
{
	const f32 yaw = angles.x;
	const f32 pitch = angles.y;
	const float3 up = { Sin(yaw)*Sin(pitch), Cos(pitch), Cos(yaw)*Sin(pitch) };
	return up;
}

float3 ForwardDirectionFromAngles(const float2 &angles)
{
	const f32 yaw = angles.x;
	const f32 pitch = angles.y;
	const float3 forward = { -Sin(yaw)*Cos(pitch), Sin(pitch), -Cos(yaw)*Cos(pitch) };
	return forward;
}

float3 RightDirectionFromAngles(const float2 &angles)
{
	const f32 yaw = angles.x;
	const float3 right = { Cos(yaw), 0.0f, -Sin(yaw) };
	return right;
}

float4x4 ViewMatrixFromCamera(const Camera &camera)
{
	const float3 forward = ForwardDirectionFromAngles(camera.orientation);
	const float3 vrp = Add(camera.position, forward);
	constexpr float3 up = {0, 1, 0};
	const float4x4 res = LookAt(vrp, camera.position, up);
	return res;
}

uint2 GetFramebufferSize(const Framebuffer &framebuffer)
{
	const uint2 size = { framebuffer.extent.width, framebuffer.extent.height };
	return size;
}

const ImageH GetDisplayImageH(const Graphics &gfx)
{
	const u32 imageIndex = gfx.device.swapchain.currentImageIndex;
	const ImageH displayImageH = gfx.device.swapchain.imageHandles[imageIndex];
	return displayImageH;
}

const Image &GetDisplayImage(const Graphics &gfx)
{
	const ImageH displayImageH = GetDisplayImageH(gfx);
	const Image &displayImage = GetImageConst(gfx.device, displayImageH);
	return displayImage;
}

Framebuffer GetDisplayFramebuffer(const Graphics &gfx)
{
	const u32 imageIndex = gfx.device.swapchain.currentImageIndex;
	const Framebuffer framebuffer = gfx.renderTargets.displayFramebuffers[imageIndex];
	return framebuffer;
}

Framebuffer GetShadowmapFramebuffer(const Graphics &gfx)
{
	const Framebuffer &framebuffer = gfx.renderTargets.shadowmapFramebuffer;
	return framebuffer;
}


struct Plane
{
	float3 normal; // Orientation of the plane
	float3 point; // Point in the plane
	//float distance; // Distance from the origin to the nearest point in the plane
};

struct FrustumPlanes
{
	Plane planes[6];
};

static FrustumPlanes FrustumPlanesFromCamera(float3 cameraPosition, float3 cameraForward, float zNear, float zFar, float fovy, float aspect)
{
	const float3 up = Float3(0.0f, 1.0f, 0.0f);
	const float3 cameraRight = Normalize(Cross(cameraForward, up));
	const float3 cameraUp = Normalize(Cross(cameraRight, cameraForward));

    const float farHalfY = zFar * Tan(0.5f * fovy * ToRadians);
    const float farHalfX = farHalfY * aspect;
	const float3 farRight = Mul(cameraRight, farHalfX);
	const float3 farLeft = Negate(farRight);
	const float3 farTop = Mul(cameraUp, farHalfY);
	const float3 farBot = Negate(farTop);
	const float3 farForward = Mul(cameraForward, zFar);

	const float3 topLeft = Add(Add(farForward, farLeft), farTop);
	const float3 topRight = Add(Add(farForward, farRight), farTop);
	const float3 botLeft = Add(Add(farForward, farLeft), farBot);
	const float3 botRight = Add(Add(farForward, farRight), farBot);

	const float3 topFaceNormal = Normalize(Cross(topLeft, topRight));
	const float3 botFaceNormal = Normalize(Cross(botRight, botLeft));
	const float3 rightFaceNormal = Normalize(Cross(topRight, botRight));
	const float3 leftFaceNormal = Normalize(Cross(botLeft, topLeft));
	const float3 farFaceNormal = Negate(cameraForward);
	const float3 nearFaceNormal = cameraForward;

	const float3 topFacePoint = cameraPosition;
	const float3 botFacePoint = cameraPosition;
	const float3 rightFacePoint = cameraPosition;
	const float3 leftFacePoint = cameraPosition;
	const float3 farFacePoint = Add(cameraPosition, Mul(cameraForward, zFar));
	const float3 nearFacePoint = Add(cameraPosition, Mul(cameraForward, zNear));

	const FrustumPlanes frustumPlanes = {
		.planes = {
			{ .normal = rightFaceNormal, .point = rightFacePoint },
			{ .normal = leftFaceNormal, .point = leftFacePoint },
			{ .normal = topFaceNormal, .point = topFacePoint },
			{ .normal = botFaceNormal, .point = botFacePoint },
			{ .normal = nearFaceNormal, .point = nearFacePoint },
			{ .normal = farFaceNormal, .point = farFacePoint },
		},
	};
    return frustumPlanes;
}


static bool PointIsInFrontOfPlane(float3 point, const Plane &plane)
{
	const float3 dir = FromTo(plane.point, point);
	const float dotResult = Dot(plane.normal, dir);
	return dotResult >= 0.0f;
}

static bool PointsAreInFrustum(const float3 *points, u32 pointCount, const FrustumPlanes &frustum)
{
	bool entityIsBehindPlane = false;
	for (u32 j = 0; j < ARRAY_COUNT(frustum.planes); ++j)
	{
		entityIsBehindPlane = true;
		for (u32 i = 0; i < pointCount; ++i)
		{
			if (PointIsInFrontOfPlane(points[i], frustum.planes[j]))
			{
				entityIsBehindPlane = false;
				break;
			}
		}
		if (entityIsBehindPlane)
		{
			break;
		}
	}
	return !entityIsBehindPlane;
}

static void GetSpriteBounds(const Sprite &sprite, float2 bounds[4])
{
	const float2 size = float2{(f32)sprite.size.x, (f32)sprite.size.y} / PIXELS_PER_METER;
	bounds[0] = { 0, 0 };
	bounds[1] = { (f32)sprite.size.x, 0 };
	bounds[3] = { 0, (f32)sprite.size.y };
	bounds[2] = { (f32)sprite.size.x, (f32)sprite.size.y };
}

static bool EntityIsInFrustum3D(Scene &scene, const Entity &entity, const FrustumPlanes &frustum)
{
	bool entityIsInFrustum = false;

	float3 points[8] = {};
	u32 pointCount = 0;

	if (IsValidHandle(scene.spriteHandles, entity.spriteH))
	{
		const Sprite &sprite = GetSprite(scene, entity.spriteH);
		float2 sbounds[4] = {};
		GetSpriteBounds(sprite, sbounds);

		for (u32 i = 0; i < ARRAY_COUNT(sbounds); ++i) {
			points[pointCount++] = Float3(sbounds[i], 0.0f);
		}
	}
	else
	{
		float x = 0.5f * entity.scale;
		float y = 0.5f * entity.scale;
		const float z = 0.5f * entity.scale;

		points[pointCount++] = Add(entity.position, Float3(-x,-y,-z));
		points[pointCount++] = Add(entity.position, Float3( x,-y,-z));
		points[pointCount++] = Add(entity.position, Float3(-x, y,-z));
		points[pointCount++] = Add(entity.position, Float3( x, y,-z));
		points[pointCount++] = Add(entity.position, Float3(-x,-y, z));
		points[pointCount++] = Add(entity.position, Float3( x,-y, z));
		points[pointCount++] = Add(entity.position, Float3(-x, y, z));
		points[pointCount++] = Add(entity.position, Float3( x, y, z));
	}

	entityIsInFrustum = PointsAreInFrustum(points, pointCount, frustum);

	return entityIsInFrustum;
}

static float4 GetOrthographicCameraMinMaxRect(const Camera &camera, f32 ar)
{
	const f32 height = camera.height;
	const float2 halfExtent = { height*ar, height };
	const float2 rectMin = { camera.position.x - halfExtent.x, camera.position.y - halfExtent.y };
	const float2 rectMax = { camera.position.x + halfExtent.x, camera.position.y + halfExtent.y };
	const float4 minMaxRect = { .xy = rectMin, .zw =  rectMax };
	return minMaxRect;
}

static bool Intersects(float2 aMin, float2 aMax, float2 bMin, float2 bMax)
{
	const bool outsideX = aMax.x < bMin.x || aMin.x > bMax.x;
	const bool outsideY = aMax.y < bMin.y || aMin.y > bMax.y;
	return !(outsideX || outsideY);
}

static bool EntityIsInFrustum2D(Scene &scene, const Entity &entity, float2 rectMin, float2 rectMax)
{
	float2 halfSize = { 0.5f * entity.scale, 0.5f * entity.scale };

	if (IsValidHandle(scene.spriteHandles, entity.spriteH))
	{
		const Sprite &sprite = GetSprite(scene, entity.spriteH);
		halfSize = 0.5f * float2{(f32)sprite.size.x, (f32)sprite.size.y} / PIXELS_PER_METER;
	}

	const float2 entityMin = entity.position.xy;// { entity.position.x - halfSize.x, entity.position.y - halfSize.y };
	const float2 entityMax = entity.position.xy + 2.0f * halfSize;//{ entity.position.x + halfSize.x, entity.position.y + halfSize.y };

	return Intersects(entityMin, entityMax, rectMin, rectMax);
}


static f32 GetSceneAspectRatio(const Graphics &gfx)
{
	const uint2 sceneSize = gfx.renderTargets.sceneSize;
	const f32 preRotationDegrees = gfx.device.swapchain.preRotationDegrees;
	ASSERT(preRotationDegrees == 0 || preRotationDegrees == 90 || preRotationDegrees == 180 || preRotationDegrees == 270);
	const bool isLandscapeRotation = preRotationDegrees == 0 || preRotationDegrees == 180;
	const f32 ar = isLandscapeRotation ? (f32) sceneSize.x / sceneSize.y : (f32) sceneSize.y / sceneSize.x;
	return ar;
}

static float2 GetRoomScrollRatio(const float2 &cameraPos, const float2 &roomPos, const float2 &roomSizeWorld, const float2 &viewportSizeWorld)
{
	const float2 viewMin = cameraPos - 0.5f * viewportSizeWorld - roomPos;
	const float2 travel = roomSizeWorld - viewportSizeWorld;
	float2 ratio = { 0.5f, 0.5f };
	if (travel.x > 0.0f) ratio.x = Clamp(viewMin.x / travel.x, 0.0f, 1.0f);
	if (travel.y > 0.0f) ratio.y = Clamp(viewMin.y / travel.y, 0.0f, 1.0f);
	return ratio;
}

static float2 GetParallaxOffset(const float2 &scrollRatio, const uint2 &baseLayerSize, const uint2 &layerSize)
{
	const float2 slack = Max(Float2(baseLayerSize) - Float2(layerSize), float2{0.0f, 0.0f});
	return scrollRatio * slack;
}

bool RenderGraphics(Engine &engine)
{
	PROFILE_BLOCK(RenderGraphics);

	Scene &scene = engine.scene;
	Graphics &gfx = engine.gfx;
	Window &window = *sPlatform->window;
#if USE_EDITOR
	Editor &editor = engine.editor;
#endif

	static f32 totalSeconds = 0.0f;
	totalSeconds += gfx.deltaSeconds;

	u32 frameIndex = gfx.device.frameIndex;

	{
		PROFILE_BLOCK(BeginFrame);
		BeginFrame(gfx.device);
	}

	// We can query for previous (MAX_FRAMES_IN_FLIGHT before) frame fences
	static u32 frameCount = 0;
	if ( frameCount++ >= MAX_FRAMES_IN_FLIGHT )
	{
		const TimestampPool &timestampPool = gfx.timestampPools[frameIndex];
		const Timestamp t0 = ReadTimestamp(timestampPool, 0);
		const Timestamp t1 = ReadTimestamp(timestampPool, 1);
		ASSERT(t1.millis >= t0.millis);
		AddTimeSample(gfx.gpuFrameTimes, t1.millis - t0.millis);
	}

#if USE_UI
	UI_FinalizeDrawData(engine.ui);
#endif

	// Display size
	const i32 displayWidth = gfx.device.swapchain.extent.width;
	const i32 displayHeight = gfx.device.swapchain.extent.height;

	// Camera setup
	float4x4 viewMatrix = Eye();
	float4x4 inverseViewMatrix = Eye();
	float4x4 viewportRotationMatrix = Eye();
	float4x4 projectionMatrix = Eye();
	float4 frustumTopLeft = {};
	float4 frustumBottomRight = {};

	// In game mode the scene renders to a low-res target; snap the camera and
	// entity positions to the pixel grid so sprites don't shimmer at sub-pixel
	// offsets. The game keeps its own unsnapped camera, so smooth movements
	// (e.g. lerps) are not affected by this.
	const bool snapToPixelGrid = engine.game.state == GameStateRunning;
	constexpr f32 pixelSize = 1.0f / PIXELS_PER_METER;

	Camera camera = engine.gfx.camera;
	if (snapToPixelGrid && camera.projectionType == ProjectionOrthographic)
	{
		camera.position.x = Round(camera.position.x / pixelSize) * pixelSize;
		camera.position.y = Round(camera.position.y / pixelSize) * pixelSize;
	}

	const f32 preRotationDegrees = gfx.device.swapchain.preRotationDegrees;
	const f32 ar = GetSceneAspectRatio(gfx);
	const float4 cameraMinMaxRect = GetOrthographicCameraMinMaxRect(camera, ar);

	if (camera.projectionType == ProjectionPerspective)
	{
		// Calculate camera matrices
		viewMatrix = ViewMatrixFromCamera(camera);
		inverseViewMatrix = Float4x4(Transpose(Float3x3(viewMatrix)));
		const float fovy = camera.fovy;
		const float znear = camera.znear;
		const float zfar = camera.zfar;
		viewportRotationMatrix = Rotate(float3{0.0, 0.0, 1.0}, preRotationDegrees);
		//const float4x4 preTransformMatrixInv = Float4x4(Transpose(Float3x3(viewportRotationMatrix)));
		const float4x4 perspectiveMatrix = Perspective(fovy, ar, znear, zfar);
		projectionMatrix = Mul(viewportRotationMatrix, perspectiveMatrix);

		// Frustum vectors
		const float hypotenuse  = znear / Cos( 0.5f * fovy * ToRadians );
		const float top = hypotenuse * Sin( 0.5f * fovy * ToRadians );
		const float bottom = -top;
		const float right = top * ar;
		const float left = -right;
		frustumTopLeft = Float4( Float3(left, top, -znear), 0.0f );
		frustumBottomRight = Float4( Float3(right, bottom, -znear), 0.0f );


		// CPU Frustum culling
		const float3 cameraForward = ForwardDirectionFromAngles(camera.orientation);
		const FrustumPlanes frustumPlanes = FrustumPlanesFromCamera(camera.position, cameraForward, znear, zfar, fovy, ar);
		for (HandleIter it = BeginIter(scene.entityHandles); it; it++)
		{
			Entity &entity = GetEntity(scene, *it);
			entity.culled = !EntityIsInFrustum3D(scene, entity, frustumPlanes);
		}
	}
	else
	{
		const f32 height = camera.height;
		// Calculate camera matrices
		viewMatrix = ViewMatrixFromCamera(camera);
		//inverseViewMatrix = Inverse2D(viewMatrix);
		viewportRotationMatrix = Rotate(float3{0.0, 0.0, 1.0}, preRotationDegrees);
		//const float4x4 preTransformMatrixInv = Float4x4(Transpose(Float3x3(viewportRotationMatrix)));
		const float4x4 orthographicMatrix = Orthogonal(-height*ar, height*ar, -height, height, camera.znear, camera.zfar);
		projectionMatrix = Mul(viewportRotationMatrix, orthographicMatrix);

		// Frustum vectors
		frustumTopLeft = Float4( Float3(-height*ar, height, 0), 0.0f );
		frustumBottomRight = Float4( Float3(height*ar, -height, 0), 0.0f );

		// CPU Frustum culling
		for (HandleIter it = BeginIter(scene.entityHandles); it; it++)
		{
			Entity &entity = GetEntity(scene, *it);
			entity.culled = !EntityIsInFrustum2D(scene, entity, cameraMinMaxRect.xy, cameraMinMaxRect.zw);
		}
	}

	// Sun matrices
	const float3 sunDirUnnormalized =
		(camera.projectionType == ProjectionPerspective) ?
		Float3(-2.0f, 2.0f, -1.0f) : Float3(0.0f, 0.0f, -1.0f);

	const float4x4 sunRotationMatrix = Rotate(float3{0.0, 1.0, 0.0}, 180.0f);
	const float3 sunDir = Normalize(MulVector(sunRotationMatrix, sunDirUnnormalized));
	const float3 sunPos = Float3(0.0f, 0.0f, 0.0f);
	const float3 sunVrp = Sub(sunPos, sunDir);
	const float3 sunUp = Float3(0.0f, 1.0f, 0.0f);
	const float4x4 sunViewMatrix = LookAt(sunVrp, sunPos, sunUp);
	const float4x4 sunProjMatrix = Orthogonal(-5.0f, 5.0f, -10.0f, 5.0f, -5.0f, 10.0f);

	// Camera UI 2D
	const f32 l = 0.0f;
	const f32 r = (f32) displayWidth;
	const f32 t = 0.0f;
	const f32 b = (f32) displayHeight;
	const f32 n = 0.0f;
	const f32 f = 1.0f;
	const float4x4 camera2dProjection = Orthogonal(l, r, b, t, n, f);

	Handle selectedEntity = EditorGetSelectedEntity(editor);

	// Update globals struct
	const Globals globals = {
		.cameraView = viewMatrix,
		.cameraViewInv = inverseViewMatrix,
		.cameraProj = projectionMatrix,
		.camera2dProj = camera2dProjection, // UI
		.viewportRotationMatrix = viewportRotationMatrix,
		.cameraFrustumTopLeft = frustumTopLeft,
		.cameraFrustumBottomRight = frustumBottomRight,
		.sunView = sunViewMatrix,
		.sunProj = sunProjMatrix,
		.sunDir = Float4(sunDir, 0.0f),
		.eyePosition = Float4(camera.position, 1.0f),
		.shadowmapDepthBias = 0.005,
		.time = totalSeconds,
		.mousePosition = window.mouse.pos,
#if USE_EDITOR
		.selectedEntity = selectedEntity.num,
#endif
	};

	// Update globals buffer
	Globals *globalsBufferPtr = (Globals*)GetBufferPtr(gfx.device, gfx.globalsBuffer[frameIndex]);
	*globalsBufferPtr = globals;

	// Advance animation states for animated sprites (frameCount > 1)
	for (u32 i = 0; i < scene.spriteHandles.handleCount; ++i)
	{
		const Handle handle = GetHandleAt(scene.spriteHandles, i);
		const Sprite &sprite = scene.sprites[handle.idx];
		if (sprite.frameCount <= 1) continue;
		SpriteAnimState &state = scene.spriteAnimStates[handle.idx];
		state.elapsedTime += gfx.deltaSeconds;
		const f32 totalDuration = (f32)sprite.frameCount / (f32)sprite.fps;
		if (sprite.loop && state.elapsedTime >= totalDuration)
			state.elapsedTime = fmodf(state.elapsedTime, totalDuration);
		state.currentFrame = (u32)(state.elapsedTime * (f32)sprite.fps);
		if (state.currentFrame >= sprite.frameCount)
			state.currentFrame = sprite.frameCount - 1;
	}

	// Update sprite data buffer (UV per sprite)
	SSpriteData *spriteDataPtr = (SSpriteData*)GetBufferPtr(gfx.device, gfx.spriteDataBuffer[frameIndex]);
	for (u32 i = 0; i < scene.spriteHandles.handleCount; ++i)
	{
		const Handle handle = GetHandleAt(scene.spriteHandles, i);
		const Sprite &sprite = scene.sprites[handle.idx];
		const Texture &texture = GetTexture(gfx, sprite.textureH);
		{
			const float2 frameUvPos  = { (f32)sprite.pos.x / texture.size.x, (f32)sprite.pos.y / texture.size.y };
			const float2 frameUvSize = { (f32)sprite.size.x / texture.size.x, (f32)sprite.size.y / texture.size.y };
			const float frameOffsetU = sprite.frameCount > 1
				? scene.spriteAnimStates[handle.idx].currentFrame * frameUvSize.x
				: 0.0f;
			spriteDataPtr[handle.idx].uvOffset  = {frameUvPos.x + frameOffsetU, frameUvPos.y};
			spriteDataPtr[handle.idx].uvSize    = frameUvSize;
			spriteDataPtr[handle.idx].worldSize = float2{(f32)sprite.size.x, (f32)sprite.size.y} / PIXELS_PER_METER;
		}
	}

	// Layer: Update tile data buffer
	const u32 tileScratchSize = MAX_TILES * (sizeof(STileData) + sizeof(Handle));
	Scratch tileScratch(tileScratchSize);
	STileData *tileDataPtr = PushArray(tileScratch.arena, STileData, MAX_TILES);
	Handle *tileSpriteHandles = PushArray(tileScratch.arena, Handle, MAX_TILES);


	const float2 viewportSizeWorld = Float2(GetFramebufferSize(gfx.renderTargets.sceneFramebuffer)) / PIXELS_PER_METER;
	const bool parallaxEnabled = engine.game.state == GameStateRunning; // The editor shows all layers unshifted

	u32 tileCount = 0;
	for (HandleIter it = BeginIter(scene.roomHandles); it; it++)
	{
		const Room &room = GetRoom(scene, *it);

		// TODO: Skip room if not in camera

		// Layer 0 is the room's base layer: it never scrolls, the rest parallax against it
		const uint2 baseLayerSize = room.layers[0].size;
		const float2 roomPos = Float2(room.pos);
		const float2 scrollRatio = GetRoomScrollRatio(camera.position.xy, roomPos, Float2(baseLayerSize), viewportSizeWorld);

		for (i32 i = ARRAY_COUNT(room.layers) - 1; i >= 0; --i)
		{
			const Layer &layer = room.layers[i];

			if (layer.initialized && layer.visible && !layer.isCollider)
			{
				const float2 parallax = parallaxEnabled ? GetParallaxOffset(scrollRatio, baseLayerSize, layer.size) : float2{0.0f, 0.0f};

				for (i32 y = 0; y < layer.size.y; ++y)
				{
					for (i32 x = 0; x < layer.size.x; ++x)
					{
						// TODO: Skip cell if not in camera

						const Handle spriteH = layer.cells[x][y].handle;
						if (IsValidHandle(scene.spriteHandles, spriteH) && tileCount < MAX_TILES)
						{
							tileDataPtr[tileCount].pos = roomPos + Float2(int2{x, y}) + parallax;
							tileDataPtr[tileCount].spriteIndex = spriteH.idx;
							tileSpriteHandles[tileCount] = spriteH;
							tileCount++;
						}
					}
				}
			}
		}
	}

	STileData *gpuTileDataPtr = (STileData*)GetBufferPtr(gfx.device, gfx.tileDataBuffer[frameIndex]);
	MemCopy(gpuTileDataPtr, tileDataPtr, tileCount * sizeof(STileData));

	// Update entity data
	SEntity *entities = (SEntity*)GetBufferPtr(gfx.device, gfx.entityBuffer[frameIndex]);
	for (u32 i = 0; i < scene.entityHandles.handleCount; ++i)
	{
		const Handle handle = GetHandleAt(scene.entityHandles, i);
		const Entity &entity = GetEntity(scene, handle);
		float3 entityScale = Float3(entity.scale);
		float3 entityPosition = entity.position;
		if (snapToPixelGrid)
		{
			entityPosition.x = Round(entityPosition.x / pixelSize) * pixelSize;
			entityPosition.y = Round(entityPosition.y / pixelSize) * pixelSize;
		}
		const float4x4 worldMatrix = Mul(Translate(entityPosition), Scale(entityScale)); // TODO: Apply also rotation
		entities[handle.idx].world = worldMatrix;

		const u32 spriteIndex = IsValidHandle(scene.spriteHandles, entity.spriteH) ? entity.spriteH.idx : 0;
		entities[handle.idx].spriteIndex = spriteIndex;
	}

	// Update materials
	if (gfx.shouldUpdateMaterials)
	{
		gfx.shouldUpdateMaterials = false;
		UploadMaterialData(gfx);
	}

	// Update bind groups
	if (gfx.shouldUpdateGlobalBindGroups)
	{
		gfx.shouldUpdateGlobalBindGroups = false;
		UpdateGlobalBindGroups(gfx);
	}

	if (gfx.shouldUpdateMaterialBindGroups)
	{
		gfx.shouldUpdateMaterialBindGroups = false;
		UpdateMaterialBindGroups(gfx);
	}

	// Reset per-frame bind group allocators
	ResetDynamicBindGroups( gfx );

	// Record commands
	CommandList commandList = BeginCommandList(gfx.device);

	// Timestamp
	ResetTimestampPool(commandList, gfx.timestampPools[frameIndex]);
	WriteTimestamp(commandList, gfx.timestampPools[frameIndex], PipelineStageTop);

	#if USE_COMPUTE_TEST
	{
		const Pipeline &pipeline = GetPipeline(gfx.device, gfx.computeClearH);

		SetPipeline(commandList, gfx.computeClearH);

		const BindGroupDesc bindGroupDesc = {
			.layout = pipeline.layout.bindGroupLayouts[0],
			.bindings = {
				{ .index = 0, .bufferView = gfx.computeBufferViewH },
			},
		};
		const BindGroup bindGroup = CreateFullBindGroup(gfx.device, bindGroupDesc, gfx.dynamicBindGroupAllocator[frameIndex]);

		SetBindGroup(commandList, 0, bindGroup);

		Dispatch(commandList, 1, 1, 1);
	}
	#endif // USE_COMPUTE_TEST

	const BufferH globalsBuffer = gfx.globalsBuffer[frameIndex];
	const BufferH entityBuffer = gfx.entityBuffer[frameIndex];
	const BufferH vertexBuffer = gfx.globalVertexArena.buffer;
	const BufferH indexBuffer = gfx.globalIndexArena.buffer;

	// Shadow map
	if (camera.projectionType == ProjectionPerspective)
	{
		PROFILE_BLOCK(ShadowMap);

		BeginDebugGroup(commandList, "Shadow map", ColorBlack);

		SetClearDepth(commandList, 0, 0.0f);

		const Framebuffer shadowmapFramebuffer = GetShadowmapFramebuffer(gfx);
		BeginRenderPass(commandList, shadowmapFramebuffer);

		const uint2 shadowmapSize = GetFramebufferSize(shadowmapFramebuffer);
		SetViewportAndScissor(commandList, shadowmapSize);

		SetPipeline(commandList, gfx.shadowmapPipelineH);

		SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);

		for (u32 entityIndex = 0; entityIndex < scene.entityHandles.handleCount; ++entityIndex)
		{
			const Handle handle = GetHandleAt(scene.entityHandles, entityIndex);
			const Entity &entity = GetEntity(scene, handle);

			if ( !entity.visible ) continue;

			// Geometry
			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);

			// Draw!!!
			const uint32_t indexCount = entity.indices.size/sizeof(Index);
			const uint32_t firstIndex = entity.indices.offset/sizeof(Index);
			const int32_t firstVertex = entity.vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same
			DrawIndexed(commandList, indexCount, firstIndex, firstVertex, handle.num);
		}

		EndRenderPass(commandList);

		EndDebugGroup(commandList);
	}

	const Format depthFormat = gfx.device.defaultDepthFormat;
	const ImageH shadowmapImage = gfx.renderTargets.shadowmapImage;
	TransitionImageLayout(commandList, shadowmapImage, ImageStateRenderTarget, ImageStateShaderInput, 0, 1);

	// Scene
	{
		PROFILE_BLOCK(Scene);

		BeginDebugGroup(commandList, "Scene", ColorBlack);

		SetClearColorFloat4(commandList, 0, { 0.0f, 0.0f, 0.0f, 0.0f } );
		SetClearDepth(commandList, 1, 0.0f);

		const Framebuffer &sceneFramebuffer = gfx.renderTargets.sceneFramebuffer;
		BeginRenderPass(commandList, sceneFramebuffer);

		const uint2 displaySize = GetFramebufferSize(sceneFramebuffer);
		SetViewportAndScissor(commandList, displaySize);

		// Entities
		for (HandleIter it = BeginIter(scene.entityHandles); it; it++)
		{
			const Handle handle = *it;
			const Entity &entity = GetEntity(scene, handle);

			if ( !entity.visible || entity.culled ) continue;
			if ( entity.materialH == InvalidHandle ) continue;

			const MaterialH materialH = entity.materialH;
			const Material &material = GetMaterial(gfx, materialH);

			BeginDebugGroup(commandList, material.name, ColorBlack);

			// Pipeline
			SetPipeline(commandList, material.pipelineH);

			// Bind groups
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
			SetBindGroup(commandList, 1, gfx.materialBindGroups[materialH.idx]);

			// Geometry
			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);

			// Draw!!!
			const uint32_t indexCount = entity.indices.size/sizeof(Index);
			const uint32_t firstIndex = entity.indices.offset/sizeof(Index);
			const int32_t firstVertex = entity.vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same
			DrawIndexed(commandList, indexCount, firstIndex, firstVertex, handle.num);

			EndDebugGroup(commandList);
		}

		// Layer tiles
		{
			PROFILE_BLOCK(LayerTiles);

			BeginDebugGroup(commandList, "Tiles", ColorBlack);

			const Pipeline &tilePipeline = GetPipeline(gfx.device, gfx.tilePipelineH);
			const uint32_t tileIndexCount = gfx.spriteIndices.size / sizeof(Index);
			const uint32_t tileFirstIndex = gfx.spriteIndices.offset / sizeof(Index);
			const int32_t tileFirstVertex = gfx.spriteVertices.offset / sizeof(Vertex);

			SetPipeline(commandList, gfx.tilePipelineH);
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);

			for (u32 i = 0; i < tileCount; ++i)
			{
				const Handle spriteH = tileSpriteHandles[i];
				const Sprite &sprite = GetSprite(scene, spriteH);
				const ImageH imageH = GetTextureImage(gfx, sprite.textureH, gfx.pinkImageH);
				const BindGroupDesc textureBindGroupDesc = {
					.layout = tilePipeline.layout.bindGroupLayouts[2],
					.bindings = {
						{ .index = 0, .image = imageH },
					},
				};
				const BindGroup textureBindGroup = GetOrCreateDynamicBindGroup(gfx, textureBindGroupDesc);

				SetBindGroup(commandList, 2, textureBindGroup);
				DrawIndexed(commandList, tileIndexCount, tileFirstIndex, tileFirstVertex, i);
			}

			EndDebugGroup(commandList);
		}

		{
			PROFILE_BLOCK(Sprites);
			// Sprite entities
			const Pipeline &spritePipeline = GetPipeline(gfx.device, gfx.spritePipelineH);
			const uint32_t spriteIndexCount = gfx.spriteIndices.size / sizeof(Index);
			const uint32_t spriteFirstIndex = gfx.spriteIndices.offset / sizeof(Index);
			const int32_t spriteFirstVertex = gfx.spriteVertices.offset / sizeof(Vertex);

			SetPipeline(commandList, gfx.spritePipelineH);
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);

			for (HandleIter it = BeginIter(scene.entityHandles); it; it++)
			{
				const Handle handle = *it;
				const Entity &entity = GetEntity(scene, handle);

				if (!entity.visible || entity.culled) continue;

				TextureH textureH = InvalidHandle;
				if (IsValidHandle(scene.spriteHandles, entity.spriteH))
					textureH = GetSprite(scene, entity.spriteH).textureH;
				else
					continue;

				const ImageH imageH = GetTextureImage(gfx, textureH, gfx.pinkImageH);
				const BindGroupDesc textureBindGroupDesc = {
					.layout = spritePipeline.layout.bindGroupLayouts[2],
					.bindings = {
						{ .index = 0, .image = imageH },
					},
				};
				const BindGroup textureBindGroup = GetOrCreateDynamicBindGroup(gfx, textureBindGroupDesc);

				BeginDebugGroup(commandList, entity.name ? entity.name : "sprite", ColorBlack);
				SetBindGroup(commandList, 2, textureBindGroup);
				DrawIndexed(commandList, spriteIndexCount, spriteFirstIndex, spriteFirstVertex, handle.num);
				EndDebugGroup(commandList);
			}
		}

		// Sky
		if (camera.projectionType == ProjectionPerspective)
		{
			PROFILE_BLOCK(Sky);

			const ImageH &skyImage = GetTextureImage(gfx, gfx.skyTextureH, gfx.grayImageH);
			const Pipeline &pipeline = GetPipeline(gfx.device, gfx.skyPipelineH);
			const BufferChunk indices = GetIndicesForGeometryType(gfx, GeometryTypeScreen);
			const BufferChunk vertices = GetVerticesForGeometryType(gfx, GeometryTypeScreen);
			const uint32_t indexCount = indices.size/sizeof(Index);
			const uint32_t firstIndex = indices.offset/sizeof(Index);
			const int32_t firstVertex = vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same

			const BindGroupDesc bindGroupDesc = {
				.layout = pipeline.layout.bindGroupLayouts[3],
				.bindings = {
					{ .index = 0, .sampler = gfx.skySamplerH },
					{ .index = 1, .image = skyImage },
				},
			};
			const BindGroup bindGroup = CreateFullBindGroup(gfx.device, bindGroupDesc, gfx.dynamicBindGroupAllocator[frameIndex]);

			BeginDebugGroup(commandList, "sky", ColorBlack);

			SetPipeline(commandList, gfx.skyPipelineH);
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
			SetBindGroup(commandList, 3, bindGroup);
			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);
			DrawIndexed(commandList, indexCount, firstIndex, firstVertex, 0);

			EndDebugGroup(commandList);
		}

#if USE_EDITOR
		// Editor grid
		if (editor.showGrid && engine.game.state == GameStateStopped)
		{
			PROFILE_BLOCK(EditorGrid);

			if (camera.projectionType == ProjectionPerspective)
			{
				const BufferChunk indices = GetIndicesForGeometryType(gfx, GeometryTypeScreen);
				const BufferChunk vertices = GetVerticesForGeometryType(gfx, GeometryTypeScreen);
				const uint32_t indexCount = indices.size/sizeof(Index);
				const uint32_t firstIndex = indices.offset/sizeof(Index);
				const int32_t firstVertex = vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same

				BeginDebugGroup(commandList, "grid_3d", ColorBlack);

				SetPipeline(commandList, gfx.grid3dPipelineH);
				SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
				SetVertexBuffer(commandList, vertexBuffer);
				SetIndexBuffer(commandList, indexBuffer);
				DrawIndexed(commandList, indexCount, firstIndex, firstVertex, 0);

				EndDebugGroup(commandList);
			}
			else // if (IsEngineMode2D(engine.mode))
			{
				const BufferChunk indices = GetIndicesForGeometryType(gfx, GeometryTypeScreen);
				const BufferChunk vertices = GetVerticesForGeometryType(gfx, GeometryTypeScreen);
				const uint32_t indexCount = indices.size/sizeof(Index);
				const uint32_t firstIndex = indices.offset/sizeof(Index);
				const int32_t firstVertex = vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same

				BeginDebugGroup(commandList, "grid_2d", ColorBlack);

				SetPipeline(commandList, gfx.grid2dPipelineH);
				SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
				SetVertexBuffer(commandList, vertexBuffer);
				SetIndexBuffer(commandList, indexBuffer);
				DrawIndexed(commandList, indexCount, firstIndex, firstVertex, 0);

				EndDebugGroup(commandList);
			}
		}
#endif

		{ // Debug draw
			PROFILE_BLOCK(DebugDraw);

			MemCopy(gfx.debugDrawVertices[frameIndex], gfx.debugDrawVerticesCPU, gfx.debugDrawVertexCount * sizeof(DebugDrawVertex));

			BeginDebugGroup(commandList, "DebugDraw", ColorBlack);

			const Pipeline &debugDrawPipeline = GetPipeline(gfx.device, gfx.debugDrawPipelineH);
			const BindGroupLayout &bindGroupLayout = debugDrawPipeline.layout.bindGroupLayouts[3];

			SetPipeline(commandList, gfx.debugDrawPipelineH);
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
			SetVertexBuffer(commandList, gfx.debugDrawVertexBuffer[frameIndex]);

			for (u32 i = 0; i < gfx.debugDrawBatchCount; ++i)
			{
				const DebugDrawBatch &batch = gfx.debugDrawBatches[i];

				const BindGroupDesc bindGroupDesc = {
					.layout = bindGroupLayout,
					.bindings = {
						{ .index = 0, .sampler = gfx.pointSamplerH },
						{ .index = 1, .image = batch.imageH },
					},
				};
				const BindGroup bindGroup = GetOrCreateDynamicBindGroup(gfx, bindGroupDesc);
				SetBindGroup(commandList, 3, bindGroup);

				Draw(commandList, batch.vertexCount, batch.vertexIndex);
			}

			gfx.debugDrawVertexCount = 0;
			gfx.debugDrawBatchCount = 0;

			EndDebugGroup(commandList);
		}

		EndRenderPass(commandList);

		EndDebugGroup(commandList);
	}

	// Display
	{
		PROFILE_BLOCK(Display);

		BeginDebugGroup(commandList, "Display", ColorBlack);

		TransitionImageLayout(commandList, gfx.renderTargets.sceneImage, ImageStateRenderTarget, ImageStateShaderInput, 0, 1);

		const Framebuffer displayFramebuffer = GetDisplayFramebuffer(gfx);
		BeginRenderPass(commandList, displayFramebuffer);

		const uint2 displaySize = GetFramebufferSize(displayFramebuffer);
		SetViewportAndScissor(commandList, displaySize);

		{ // Scene blit
			PROFILE_BLOCK(Blit);

			BeginDebugGroup(commandList, "Blit", ColorBlack);

			const uint2 sceneSize = gfx.renderTargets.sceneSize;
			const u32 multiplier = Min(displaySize.x / sceneSize.x, displaySize.y / sceneSize.y);
			const uint2 scaledSceneSize = multiplier * sceneSize;
			const rect viewport = {
				displaySize.x > scaledSceneSize.x ? (i32)(displaySize.x - scaledSceneSize.x) / 2 : 0,
				displaySize.y > scaledSceneSize.y ? (i32)(displaySize.y - scaledSceneSize.y) / 2 : 0,
				scaledSceneSize.x,
				scaledSceneSize.y,
			};
			SetViewport(commandList, viewport);
			SetScissor(commandList, viewport);

			const Pipeline &pipeline = GetPipeline(gfx.device, gfx.blitPipelineH);
			const BindGroupLayout &bindGroupLayout = pipeline.layout.bindGroupLayouts[3];

			const BufferChunk indices = GetIndicesForGeometryType(gfx, GeometryTypeScreen);
			const BufferChunk vertices = GetVerticesForGeometryType(gfx, GeometryTypeScreen);
			const uint32_t indexCount = indices.size/sizeof(Index);
			const uint32_t firstIndex = indices.offset/sizeof(Index);
			const int32_t firstVertex = vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same

			SetPipeline(commandList, gfx.blitPipelineH);
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);

			ImageH sceneImage = gfx.renderTargets.sceneImage;
			const BindGroupDesc bindGroupDesc = {
				.layout = bindGroupLayout,
				.bindings = {
					{ .index = 0, .sampler = gfx.screenSamplerH },
					{ .index = 1, .image = sceneImage },
				},
			};
			const BindGroup textureBindGroup = CreateFullBindGroup(gfx.device, bindGroupDesc, gfx.dynamicBindGroupAllocator[frameIndex]);
			SetBindGroup(commandList, 3, textureBindGroup);

			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);
			DrawIndexed(commandList, indexCount, firstIndex, firstVertex, 0);

			SetViewportAndScissor(commandList, displaySize);

			EndDebugGroup(commandList);
		}

#if USE_UI
		{ // GUI
			PROFILE_BLOCK(GUI);

			BeginDebugGroup(commandList, "GUI", ColorBlack);

			const UI &ui = engine.ui;

			const Pipeline &pipeline = GetPipeline(gfx.device, gfx.guiPipelineH);
			const BindGroupLayout &bindGroupLayout = pipeline.layout.bindGroupLayouts[3];

			SetPipeline(commandList, gfx.guiPipelineH);
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
			SetVertexBuffer(commandList, UI_GetVertexBuffer(ui));

			for (u32 i = 0; i < UI_DrawListCount(ui); ++i)
			{
				const UIDrawList &drawList = UI_GetDrawListAt(ui, i);
				SetScissor(commandList, drawList.scissorRect);

				const BindGroupDesc textureBindGroupDesc = {
					.layout = bindGroupLayout,
					.bindings = {
						{ .index = 0, .sampler = gfx.pointSamplerH },
						{ .index = 1, .image = drawList.imageHandle },
					},
				};
				const BindGroup textureBindGroup = GetOrCreateDynamicBindGroup(gfx, textureBindGroupDesc);
				SetBindGroup(commandList, 3, textureBindGroup);

				for (u32 i = 0; i < drawList.vertexRangeCount; ++i)
				{
					const UIVertexRange &range = drawList.vertexRanges[i];
					Draw(commandList, range.count, range.index);
				}
			}

			EndDebugGroup(commandList);
		}
#endif

		EndRenderPass(commandList);

		EndDebugGroup(commandList);
	}

#if USE_EDITOR
	EditorRender(engine, commandList);
#endif // USE_EDITOR


	TransitionImageLayout(commandList, shadowmapImage, ImageStateShaderInput, ImageStateRenderTarget, 0, 1);

	WriteTimestamp(commandList, gfx.timestampPools[frameIndex], PipelineStageBottom);

	EndCommandList(commandList);

	SubmitResult submitRes;

	{
		PROFILE_BLOCK(Submit);
		submitRes = Submit(gfx.device, commandList);
	}

	{
		PROFILE_BLOCK(Present);
		if ( !Present(gfx.device, submitRes) ) {
			return false;
		}
	}

	// TODO: Check if this should be executed even if Present failed...
	PROFILE_BLOCK(EndFrame);
	EndFrame(gfx.device);

	return true;
}


#if USE_UI

void UIBeginFrameRecording(Engine &engine)
{
	UI &ui = GetUI(engine);
	const Window &window = *sPlatform->window;
	Graphics &gfx = engine.gfx;

	UI_SetInputState(ui, window.keyboard, window.mouse, window.chars);
	UI_SetViewportSize(ui, uint2{window.width, window.height});

	UI_BeginFrame(ui);
}

void UIEndFrameRecording(Engine &engine)
{
	UI &ui = GetUI(engine);
	UI_EndFrame(ui);
}

#endif

static bool sKeyPendingRelease[K_COUNT];
static bool sGamepadButtonPendingRelease[ARRAY_COUNT(Gamepad::buttons)];

struct PlatformInput
{
	Gamepad gamepad;
	Keyboard keyboard;
	Mouse mouse;
};

static void InputAccumulate(PlatformInput &input, const PlatformInput &newInput)
{
	for (u32 i = 0; i < K_COUNT; ++i)
	{
		const KeyState newState = newInput.keyboard.keys[i];
		KeyState &state = input.keyboard.keys[i];

		if (newState == KEY_STATE_PRESS)
		{
			// Latch the edge until a fixed step consumes it
			state = KEY_STATE_PRESS;
			sKeyPendingRelease[i] = false;
		}
		else if (newState == KEY_STATE_RELEASE)
		{
			if (state == KEY_STATE_PRESS) {
				// The press was not consumed yet: keep it and release right after
				sKeyPendingRelease[i] = true;
			} else {
				state = KEY_STATE_RELEASE;
			}
		}
	}
	for (u32 i = 0; i < ARRAY_COUNT(input.gamepad.buttons); ++i)
	{
		const ButtonState newState = newInput.gamepad.buttons[i];
		ButtonState &state = input.gamepad.buttons[i];

		if (newState == BUTTON_STATE_PRESS)
		{
			// Latch the edge until a fixed step consumes it
			state = BUTTON_STATE_PRESS;
			sGamepadButtonPendingRelease[i] = false;
		}
		else if (newState == BUTTON_STATE_RELEASE)
		{
			if (state == BUTTON_STATE_PRESS) {
				// The press was not consumed yet: keep it and release right after
				sGamepadButtonPendingRelease[i] = true;
			} else {
				state = BUTTON_STATE_RELEASE;
			}
		}
	}
	input.gamepad.leftTrigger = newInput.gamepad.leftTrigger;
	input.gamepad.rightTrigger = newInput.gamepad.rightTrigger;
	input.gamepad.leftAxis = newInput.gamepad.leftAxis;
	input.gamepad.rightAxis = newInput.gamepad.rightAxis;
}

static void InputConsume(PlatformInput &input)
{
	for (u32 i = 0; i < K_COUNT; ++i)
	{
		KeyState &state = input.keyboard.keys[i];

		if (state == KEY_STATE_PRESS)
		{
			state = sKeyPendingRelease[i] ? KEY_STATE_RELEASE : KEY_STATE_PRESSED;
			sKeyPendingRelease[i] = false;
		}
		else if (state == KEY_STATE_RELEASE)
		{
			state = KEY_STATE_IDLE;
		}
	}
	for (u32 i = 0; i < ARRAY_COUNT(input.gamepad.buttons); ++i)
	{
		ButtonState &state = input.gamepad.buttons[i];

		if (state == BUTTON_STATE_PRESS)
		{
			state = sGamepadButtonPendingRelease[i] ? BUTTON_STATE_RELEASE : BUTTON_STATE_PRESSED;
			sGamepadButtonPendingRelease[i] = false;
		}
		else if (state == BUTTON_STATE_RELEASE)
		{
			state = BUTTON_STATE_IDLE;
		}
	}
}

void GameUpdate(Engine &engine, const Plat &platform)
{
	Game &game = engine.game;

	static PlatformInput accumulatedInput = {};
	static f32 accumulatedSeconds = 0.0f;

	if (game.state == GameStateStarting)
	{
		EngineWaitDeviceIdle(engine.gfx);
		DestroyRenderTargets(engine.gfx, engine.gfx.renderTargets);
		CreateRenderTargets(engine.gfx, SCENE_WIDTH, SCENE_HEIGHT);

		accumulatedInput = {};
		accumulatedSeconds = 0.0f;
		MemSet(sKeyPendingRelease, sizeof(sKeyPendingRelease), 0);

		GameStart(game);
		game.state = GameStateRunning;
	}

	if (game.state == GameStateRunning)
	{
		constexpr f32 fixedStepSeconds = 1.0f / 60.0f;
		constexpr f32 maxFrameSeconds = 0.25f; // avoid catch-up bursts after stalls (hot-reload, shader compiles...)

		const PlatformInput platformInput = {
			.gamepad = *platform.gamepad,
			.keyboard = platform.window->keyboard,
			.mouse = platform.window->mouse,
		};

		InputAccumulate(accumulatedInput, platformInput);

		accumulatedSeconds += Min(engine.gfx.deltaSeconds, maxFrameSeconds);

		while (accumulatedSeconds >= fixedStepSeconds)
		{
			game.deltaSeconds = fixedStepSeconds;
			GameSetInput(game, accumulatedInput.keyboard, accumulatedInput.mouse, accumulatedInput.gamepad);
			GameSimulate(game);
			InputConsume(accumulatedInput);
			accumulatedSeconds -= fixedStepSeconds;
		}

		GameUpdate(game);
	}

	if (game.state == GameStateStopping)
	{
		GameStop(game);

		AudioStopAll(engine);

		EngineWaitDeviceIdle(engine.gfx);
		DestroyRenderTargets(engine.gfx, engine.gfx.renderTargets);
		CreateRenderTargets(engine.gfx);

		game.state = GameStateStopped;
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Dynamic library interface

#if PLATFORM_WINDOWS
#define ENGINE_API extern "C" __declspec(dllexport)
#else
#define ENGINE_API extern "C"
#endif

// Bumped by hand whenever the layout of the retained engine state changes in a way its size
// does not capture, for instance reordering fields that happen to have the same size.
#define ENGINE_STATE_VERSION 1

// Signature to let the platform know if the memory layout of engine data changed
ENGINE_API u32 OnPlatformGetStateSignature()
{
	const u32 layout[] = {
		ENGINE_STATE_VERSION,
		sizeof(Engine),
		sizeof(Graphics),
		sizeof(Scene),
		sizeof(Audio),
		sizeof(Game),
#if USE_UI
		sizeof(UI),
#endif
#if USE_EDITOR
		sizeof(Editor),
#endif
	};

	const u32 signature = HashFNV(layout, sizeof(layout));
	return signature;
}

ENGINE_API void OnPlatformSetupAPI(Plat &platform)
{
	SetPlatformAPI(platform);
	SetGraphicsAPI(&platform.graphicsAPI);
	ProfileInit();

	if ( platform.engine )
	{
		UI_ResetStyle(platform.engine->ui);
	}
}

ENGINE_API bool OnPlatformPreInit(Plat &platform)
{
	::engine = PushZeroStruct(GlobalArena, Engine);
	platform.engine = ::engine;

	Engine &engine = *::engine;

#if USE_DATA_BUILD
	bool buildAssets = false;
	bool exitAfterBuild = false;
	for ( u32 i = 0; i < platform.argc; ++i ) {
		if ( StrEq(platform.argv[i], "--build-assets") ) {
			buildAssets = true;
			exitAfterBuild = true;
		}
	}

	const FilePath assetsFilepath = MakePath(DataDir, "assets.dat");
	if ( !ExistsFile(assetsFilepath.str) ) {
		buildAssets = true;
	}

	if ( buildAssets ) {
		const FilePath shadersFilepath = MakePath(DataDir, "shaders.dat");
		BuildShaders(engine, shadersFilepath.str);
		const FilePath descriptorsFilepath = MakePath(AssetDir, "assets.txt");
		BuildAssetsFromTxt(engine, descriptorsFilepath.str, assetsFilepath.str);
		if (exitAfterBuild) {
			PlatformQuit();
		}
	}
#endif // USE_DATA_BUILD

	return true;
}

ENGINE_API bool OnPlatformInit(Plat &platform)
{
	SetGraphicsStringInterning(platform.stringInterning);

	Engine &engine = GetEngine(platform);

	Graphics &gfx = engine.gfx;
	Game &game = engine.game;

#if USE_EDITOR
	CompileModifiedShaders();
#endif

	// Initialize graphics
	if ( !InitializeGraphicsDriver(gfx.device, GlobalArena) )
	{
		// TODO: Actually we could throw a system error and exit...
		LOG(Error, "InitializeGraphicsDriver failed!\n");
		return false;
	}

	// Initialize sound system
	if ( !InitializeAudio(engine.audio, GlobalArena) )
	{
		LOG(Error, "InitializeAudio failed!\n");
		return false;
	}

	return true;
}

ENGINE_API bool OnPlatformWindowInit(Plat &platform)
{
	Engine &engine = GetEngine(platform);
	Graphics &gfx = engine.gfx;

	if ( !InitializeGraphicsSurface(gfx.device, *platform.window) )
	{
		// TODO: Actually we could throw a system error and exit...
		LOG(Error, "InitializeGraphicsSurface failed!\n");
		return false;
	}

	if ( gfx.deviceInitialized )
	{
		// TODO: Check the current device still supports the new surface
	}
	else
	{
		if (!sLoadShadersFromText) {
			LoadShadersFromBin(engine);
		}

		if ( !InitializeGraphics(engine, GlobalArena) )
		{
			// TODO: Actually we could throw a system error and exit...
			LOG(Error, "InitializeGraphics failed!\n");
			return false;
		}

		gfx.camera = {
			.projectionType = ProjectionOrthographic,
			.znear = -10.0f,
			.zfar = 10.0f,
			.height = 8.0f,
		};

		Initialize(engine.scene.roomHandles, GlobalArena, MAX_ROOMS);
		Initialize(engine.scene.entityHandles, GlobalArena, MAX_ENTITIES);
		Initialize(engine.scene.spriteHandles, GlobalArena, MAX_SPRITES);

#if USE_EDITOR
		EditorInitialize(engine);
#else
		engine.mode = EngineModeGame3D;
		LoadSceneFromBin(engine);
#endif
	}

	return true;
}

ENGINE_API void OnPlatformUpdate(Plat &platform)
{
	ProfileRegisterThread("UpdateAndRender");
	PROFILE_FRAME();
	PROFILE_BLOCK(Update);

	Engine &engine = GetEngine(platform);
	Graphics &gfx = engine.gfx;

	::engine = &engine;

	const Clock begin = GetClock();

#if PLATFORM_ANDROID
	// TODO(jesus): This is test code
	static bool firstUpdate = true;
	if ( firstUpdate )
	{
		firstUpdate = false;
		//LoadSceneFromBin(engine);
		MusicPlay(engine, 0);
	}
#endif

#if USE_UI
	UIBeginFrameRecording(engine);
#endif

#if USE_EDITOR
	static Clock lastClock = GetClock();
	const Clock currentClock = GetClock();
	const f32 secondsSinceLastCheck  = GetSecondsElapsed(lastClock, currentClock);

	if ( secondsSinceLastCheck > 0.2 )
	{
		lastClock = currentClock;

		if ( CompileModifiedShaders() )
		{
			// NOTE(jesus): Recompiling all pipelines here even if likely only a shader was recompiled :-S
			WaitDeviceIdle(gfx.device);
			Scratch scratch;
			RecompilePipelines(engine, scratch.arena);
		}

		RecreateModifiedTextures(engine);
	}

	EditorUpdate(engine);
#endif

	{
		PROFILE_BLOCK(GameUpdate);
		GameUpdate(engine, platform);
	}

#if USE_UI
	UIEndFrameRecording(engine);
#endif

	const Clock end = GetClock();
	const f32 updateMillis = GetSecondsElapsed(begin, end) * 1000.0f;
	AddTimeSample( gfx.cpuFrameTimes, updateMillis );
}

ENGINE_API void OnPlatformRenderGraphics(Plat &platform)
{
	PROFILE_BLOCK(RenderGraphics);

	Engine &engine = GetEngine(platform);
	Graphics &gfx = engine.gfx;

	static Clock lastClock = GetClock();
	Clock currentClock = GetClock();
	gfx.deltaSeconds = GetSecondsElapsed(lastClock, currentClock);
	lastClock = currentClock;

	if ( !gfx.deviceInitialized )
	{
		return;
	}

	if ( !IsValidSwapchain(gfx.device) )
	{
		EngineWaitDeviceIdle(gfx);
		DestroyRenderTargets(gfx, gfx.renderTargets);
		if ( platform.window->width != 0 && platform.window->height != 0 )
		{
			RecreateSwapchain(gfx.device, *platform.window);

			char debugName[16];
			for (u32 i = 0; i < ARRAY_COUNT(gfx.device.swapchain.imageHandles); ++i) {
				SPrintf(debugName, "swapchain_%u", i);
				SetObjectNameImage(gfx.device, gfx.device.swapchain.imageHandles[i], debugName);
			}

			u32 sceneWidth = 0;
			u32 sceneHeight = 0;
			if ( engine.game.state != GameStateStopped ) {
				sceneWidth = SCENE_WIDTH;
				sceneHeight = SCENE_HEIGHT;
			}
			CreateRenderTargets(gfx, sceneWidth, sceneHeight);
		}
	}

	if ( IsValidSwapchain(gfx.device) )
	{
		RenderGraphics(engine);

#if USE_EDITOR
		EditorPostRender(engine);
#endif
	}
}

ENGINE_API void OnPlatformPreRenderAudio(Plat &platform)
{
	ProfileRegisterThread("Audio");
	ProfileFlush();
	PROFILE_BLOCK(PreRenderAudio);
	Engine &engine = GetEngine(platform);
	PreRenderAudio(engine);
}

ENGINE_API void OnPlatformRenderAudio(Plat &platform, SoundBuffer &soundBuffer)
{
	ProfileFlush();
	PROFILE_BLOCK(RenderAudio);
	Engine &engine = GetEngine(platform);
	RenderAudio(engine, soundBuffer);
}

ENGINE_API void OnPlatformWindowCleanup(Plat &platform)
{
	Engine &engine = GetEngine(platform);
	Graphics &gfx = engine.gfx;

	EngineWaitDeviceIdle(gfx);
	DestroyRenderTargets(gfx, gfx.renderTargets);
	DestroySwapchain(gfx.device);
	CleanupGraphicsSurface(gfx.device);
}

ENGINE_API void OnPlatformCleanup(Plat &platform)
{
	Engine &engine = GetEngine(platform);
	Graphics &gfx = engine.gfx;
	Game &game = engine.game;

	EngineWaitDeviceIdle(gfx);

#if USE_UI
	UI_Cleanup(engine.ui);
#endif

	CleanupGraphics(gfx);
}



////////////////////////////////////////////////////////////////////////
// Game API

void SetCamera(const Camera &camera)
{
	engine->gfx.camera = camera;
}

Room *GetRoom(const char *name)
{
	static Room nullRoom = {};
	Room *roomPtr = &nullRoom;
	for ( HandleIter it = BeginIter(engine->scene.roomHandles); it; it++)
	{
		Room &room = GetRoom(engine->scene, *it);
		if ( StrEq(room.name, name) ) {
			roomPtr = &room;
			break;
		}
	}
	return roomPtr;
}

Entity *GetEntity(const char *name)
{
	static Entity nullEntity = {};
	Entity *ent = &nullEntity;
	for ( HandleIter it = BeginIter(engine->scene.entityHandles); it; it++)
	{
		Entity &entity = GetEntity(engine->scene, *it);
		if ( StrEq(entity.name, name) ) {
			ent = &entity;
			break;
		}
	}
	return ent;
}

AudioClipH GetAudioClip(const char *name)
{
	Handle handle = InvalidHandle;
	for ( HandleIter it = BeginIter(engine->audio.clipHandles); it; it++)
	{
		AudioClipDesc &desc = GetAudioClipDesc(engine->audio, *it);
		if ( StrEq(desc.name, name) ) {
			handle = *it;
			break;
		}
	}
	return handle;
}

u32 PlayAudioClip(Handle handle)
{
	u32 ret = PlayAudioClip(*engine, handle);
	return ret;
}

Handle GetMusic(const char *name)
{
	Handle handle = InvalidHandle;
	for ( HandleIter it = BeginIter(engine->audio.musicHandles); it; it++)
	{
		MusicFileDesc &desc = GetMusicFileDesc(engine->audio, *it);
		if ( StrEq(desc.name, name) ) {
			handle = *it;
			break;
		}
	}
	return handle;
}

void PlayMusic(Handle handle)
{
	MusicPlay(*engine, handle);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementations

#define PLATFORM_API_IMPLEMENTATION
#include "platform.h"

#include "data.cpp"
#include "audio.cpp"

#define ILU_PROFILE_IMPLEMENTATION
#include "ilu_profile.h"

#if USE_EDITOR
#include "editor.cpp"
#endif

#include "libs/ibxm/ibxm.c"

#include "game.cpp"

// TODO:
// - [ ] Instead of binding descriptors per entity, group entities by material and perform a multi draw call for each material group.
// - [ ] GPU culling: Modify the compute to perform frustum culling and save the result in the buffer.
// - [ ] Text rendering
// - [ ] Include directive in the C AST parsing code.
//
// DONE:
// - [X] Avoid using push constants and put transformation matrices in buffers instead.
// - [X] Investigate how to write descriptors in a more elegant manner (avoid hardcoding).
// - [X] Put all the geometry in the same buffer.
// - [X] GPU culling: Add a "hello world" compute shader that writes some numbers into a buffer.
// - [X] GPU time queries
// - [X] GPU culling: As a first step, perform frustum culling in the CPU.
// - [X] Avoid duplicated global descriptor sets.
// - [X] Have a single descripor set for global info that only changes once per frame

