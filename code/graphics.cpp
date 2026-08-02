#define INVALID_HANDLE -1

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
	{ .type = ShaderTypeVertex,   .filename = "fog.hlsl",            .entryPoint = "VSMain",      .name = "vs_fog" },
	{ .type = ShaderTypeFragment, .filename = "fog.hlsl",            .entryPoint = "PSMain",      .name = "fs_fog" },
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
	{
		.vsName = "vs_fog",
		.fsName = "fs_fog",
		.renderPass = "scene_renderpass",
		.desc = {
			.name = "pipeline_fog",
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

ShaderSourceDesc *GetShaderSourceDescs()
{
	return shaderSourceDescs;
}

u32 GetShaderSourceDescCount()
{
	return ARRAY_COUNT(shaderSourceDescs);
}


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

void UploadData(Graphics &gfx, const CommandList &commandList, const void *data, u32 size, BufferH destBuffer, u32 destOffset, u32 alignment)
{
	StagedData staged = StageData(gfx, data, size, alignment);

	// Copy contents from the staging to the final buffer
	CopyBufferToBuffer(commandList, staged.buffer, staged.offset, destBuffer, destOffset, size);
}

BufferChunk PushData(Graphics &gfx, const CommandList &commandList, BufferArena &arena, const void *data, u32 size, u32 alignment)
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
// Render targets

void CreateRenderTargets(Graphics &gfx, u32 sceneWidth, u32 sceneHeight)
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

void CompileGraphicsPipeline(Engine &engine, Arena scratch, u32 pipelineIndex)
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

void CompileComputePipeline(Engine &engine, Arena scratch, u32 pipelineIndex)
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
	gfx.noiseTexture = FindTextureHandle(gfx, "tex_noise");

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
	gfx.fogPipelineH = FindPipelineHandle(gfx, "pipeline_fog");

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

void RecompilePipelines(Engine &engine, Arena scratch)
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

void ResetDynamicBindGroups( Graphics &gfx )
{
	ResetBindGroupAllocator( gfx.device, gfx.dynamicBindGroupAllocator[gfx.device.frameIndex] );
	MemSet(gfx.dynamicBindGroupDescs, gfx.dynamicBindGroupCount * sizeof(BindGroupDesc), 0);
	gfx.dynamicBindGroupCount = 0;
}

const BindGroup &GetOrCreateDynamicBindGroup(Graphics &gfx, const BindGroupDesc &bindGroupDesc)
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
	// Larger than the default MB(1): this scratch has to hold the whole pipeline cache file,
	// which grows with the number of pipelines and already exceeds 1 MB.
	Scratch scratch(MB(8));
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

#define MAX_DEBUG_DRAW_VERTICES 4092

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
		{ .set = 0, .binding = BINDING_NOISE2D, .type = SpvTypeImage, .stageFlags = SpvStageFlagsFragmentBit },
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
	const u32 noiseWidth = 32;
	const u32 noiseHeight = 32;
	ResetArena(scratch.arena);
	rgba *noiseImagePixels = PushArray(scratch.arena, rgba, noiseWidth * noiseHeight);
	for (u32 y = 0; y < noiseHeight; ++y) {
		for (u32 x = 0; x < noiseWidth; ++x) {
			byte r = rand();
			byte g = rand();
			byte b = rand();
			byte a = rand();
			noiseImagePixels[y * noiseWidth + x] = {r, g, b, a};
		}
	}
	gfx.noiseImageH = EngineCreateImage(gfx, "noiseImage", noiseWidth, noiseHeight, 4, false, (byte*)noiseImagePixels);

	// Builtin texture
	const TextureDesc defaultTextureDesc = {
		.name = InternString("tex_default"),
		.filename = InternString(""),
		.mipmap = 0,
		.flags = AssetFlag_Builtin,
	};
	gfx.defaultTexture = CreateTexture(gfx, defaultTextureDesc, gfx.pinkImageH);

	// Builtin noise texture
	const TextureDesc noiseTextureDesc = {
		.name = InternString("tex_noise"),
		.filename = InternString(""),
		.mipmap = 0,
		.flags = AssetFlag_Builtin,
	};
	gfx.noiseTexture = CreateTexture(gfx, noiseTextureDesc, gfx.noiseImageH);

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
	PROFILE_GPU_INIT(gfx.device);

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
			{ .index = BINDING_NOISE2D, .image = gfx.noiseImageH },
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

	PROFILE_GPU_CLEANUP(gfx.device);

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
