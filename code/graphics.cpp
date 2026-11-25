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

static const Vertex screenTriangleVertices90[] = {
	{{ 1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	{{-3.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 2.0f}},
	{{ 1.0f, -3.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {2.0f, 0.0f}},
};

static const Vertex screenTriangleVertices180[] = {
	{{ 1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	{{ 1.0f,  3.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 2.0f}},
	{{-3.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {2.0f, 0.0f}},
};

static const Vertex screenTriangleVertices270[] = {
	{{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	{{ 3.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 2.0f}},
	{{-1.0f,  3.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {2.0f, 0.0f}},
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
	{ .type = ShaderTypeCompute,  .filename = "light_binning.hlsl",  .entryPoint = "CSMain",      .name = "light_binning" },
	{ .type = ShaderTypeCompute,  .filename = "compute.hlsl",        .entryPoint = "main_clear",  .name = "compute_clear" },
	{ .type = ShaderTypeCompute,  .filename = "compute.hlsl",        .entryPoint = "main_update", .name = "compute_update" },
	{ .type = ShaderTypeVertex,   .filename = "debug_draw.hlsl",     .entryPoint = "VSMain",      .name = "vs_debug_draw" },
	{ .type = ShaderTypeFragment, .filename = "debug_draw.hlsl",     .entryPoint = "PSMain",      .name = "fs_debug_draw" },
	{ .type = ShaderTypeVertex,   .filename = "particles.hlsl",      .entryPoint = "VSMain",      .name = "vs_particles" },
	{ .type = ShaderTypeFragment, .filename = "particles.hlsl",      .entryPoint = "PSMain",      .name = "fs_particles" },
	{ .type = ShaderTypeVertex,   .filename = "fog.hlsl",            .entryPoint = "VSMain",      .name = "vs_fog" },
	{ .type = ShaderTypeFragment, .filename = "fog.hlsl",            .entryPoint = "PSMain",      .name = "fs_fog" },
	{ .type = ShaderTypeVertex,   .filename = "distortion_map.hlsl", .entryPoint = "VSMain",      .name = "vs_distortion_map" },
	{ .type = ShaderTypeFragment, .filename = "distortion_map.hlsl", .entryPoint = "PSMain",      .name = "fs_distortion_map" },
	{ .type = ShaderTypeFragment, .filename = "distortion.hlsl",     .entryPoint = "PSMain",      .name = "fs_distortion" },
	{ .type = ShaderTypeFragment, .filename = "bloom.hlsl",          .entryPoint = "PSPrefilter", .name = "fs_bloom_prefilter" },
	{ .type = ShaderTypeFragment, .filename = "bloom.hlsl",          .entryPoint = "PSDownsample", .name = "fs_bloom_downsample" },
	{ .type = ShaderTypeFragment, .filename = "bloom.hlsl",          .entryPoint = "PSUpsample",  .name = "fs_bloom_upsample" },
	{ .type = ShaderTypeFragment, .filename = "bloom.hlsl",          .entryPoint = "PSComposite", .name = "fs_bloom_composite" },
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

RenderPass FindRenderPassHandle(const Graphics &gfx, const char *name)
{
	const RenderPass handle = FindRenderPass(gpu, name);
	if ( handle ) {
		return handle;
	}
	LOG(Warning, "Could not find render <%s> handle.\n", name);
	INVALID_CODE_PATH();
	return {};
}

struct StagedData
{
	Buffer buffer;
	u32 offset;
};

static CommandList BeginUploadCommandList(Graphics &gfx)
{
	ASSERT(!gfx.inUploadContext && "Cannot nest calls to BeginUploadCommandList");
	gfx.inUploadContext = true;

	gfx.stagingBufferOffset = 0;

	CommandList commandList = BeginTransientCommandList(gpu);
	return commandList;
}

static void EndUploadCommandList(Graphics &gfx, CommandList commandList)
{
	EndTransientCommandList(gpu, commandList);

	ASSERT(gfx.inUploadContext && "BeginUploadCommandList must have been called first");
	gfx.inUploadContext = false;
}

static StagedData StageData(Graphics &gfx, const void *data, u32 size, u32 alignment = 0)
{
	ASSERT(gfx.inUploadContext && "StageData must be called between calls to Begin/EndUploadCommandList");


	const u32 finalAlignment = Max(alignment, GetDeviceCaps(gpu).alignment.optimalBufferCopyOffset);
	const u32 alignedOffset = AlignUp(gfx.stagingBufferOffset, finalAlignment);

	ASSERT(alignedOffset + size <= GetBufferSize(gpu, gfx.stagingBuffer));

	StagedData staging = {};
	staging.buffer = gfx.stagingBuffer;
	staging.offset = alignedOffset;

	byte *stagingData = (byte*)GetBufferPtr(gpu, gfx.stagingBuffer) + staging.offset;
	MemCopy(stagingData, data, size);

	gfx.stagingBufferOffset = staging.offset + size;

	return staging;
}

Buffer CreateStagingBuffer(Graphics &gfx)
{
	const u32 stagingHeapSize = GetHeapSize(gpu, HeapType_Staging);
	Buffer stagingBufferHandle = CreateBuffer(gpu, stagingHeapSize, BufferUsageTransferSrc, HeapType_Staging);
	return stagingBufferHandle;
}

Buffer CreateVertexBuffer(Graphics &gfx, u32 size)
{
	Buffer vertexBufferHandle = CreateBuffer(
			gpu,
			size,
			BufferUsageVertexBuffer | BufferUsageTransferDst,
			HeapType_General);

	return vertexBufferHandle;
}

Buffer CreateIndexBuffer(Graphics &gfx, u32 size)
{
	Buffer indexBufferHandle = CreateBuffer(
			gpu,
			size,
			BufferUsageIndexBuffer | BufferUsageTransferDst,
			HeapType_General);

	return indexBufferHandle;
}

BufferArena MakeBufferArena(Graphics &gfx, Buffer bufferHandle)
{
	const BufferArena arena = {
		.buffer = bufferHandle,
		.used = 0,
	};
	return arena;
}

void UploadData(Graphics &gfx, const CommandList &commandList, const void *data, u32 size, Buffer destBuffer, u32 destOffset, u32 alignment)
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

void GenerateMipmaps(const GraphicsDevice &device, const CommandList &commandList, Image image)
{
	const Format format = GetImageFormat(device, image);
	const uint2 size = GetImageSize(device, image);
	const u32 mipLevels = GetImageMipLevels(device, image);

	if (!GetDeviceCaps(device).formatSupport[format].linearFilteredSampling) {
		LOG(Error, "GenerateMipmaps() - Linear filtering not supported for format %s.\n", FormatName(format));
		QUIT_ABNORMALLY();
	}

	for (u32 i = 1; i < mipLevels; ++i)
	{
		TransitionImageLayout(commandList, image, ImageStateTransferDst, ImageStateTransferSrc, i - 1, 1);

		const i32 srcWidth = Max(size.x >> (i-1), 1u);
		const i32 srcHeight = Max(size.y >> (i-1), 1u);
		const i32 dstWidth = Max(size.x >> i, 1u);
		const i32 dstHeight = Max(size.y >> i, 1u);
		const BlitRegion srcRegion = { .x = 0, .y = 0, .width = srcWidth, .height = srcHeight, .mipLevel = i - 1 };
		const BlitRegion dstRegion = { .x = 0, .y = 0, .width = dstWidth, .height = dstHeight, .mipLevel = i };

		Blit(commandList, image, srcRegion, image, dstRegion);

		TransitionImageLayout(commandList, image, ImageStateTransferSrc, ImageStateShaderInput, i - 1, 1);
	}

	TransitionImageLayout(commandList, image, ImageStateTransferDst, ImageStateShaderInput, mipLevels - 1, 1);
}

Image GfxCreateImage(Graphics &gfx, const char *name, int width, int height, int channels, bool mipmap, const byte *pixels)
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

	Image image = CreateImage(gpu,
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
		GenerateMipmaps(gpu, commandList, image);
	}
	else
	{
		TransitionImageLayout(commandList, image, ImageStateTransferDst, ImageStateShaderInput, 0, 1);
	}

	EndUploadCommandList(gfx, commandList);

	SetObjectNameImage(gpu, image, name);

	return image;
}

Image GfxCreateImage(Graphics &gfx, const ImagePixels &img, const char *name, bool createMipmaps)
{
	const Image imageHandle = GfxCreateImage(gfx, name, img.width, img.height, img.channelCount, createMipmaps, img.pixels);
	return imageHandle;
}

////////////////////////////////////////////////////////////////////////
// Texture management

Texture &GetTexture(ID id)
{
	if ( !id ) {
		Graphics &gfx = GetEngine().gfx;
		id = gfx.defaultTexture;
	}
	Texture &texture = *((Texture*)GetObject(id));
	return texture;
}

// Appends a texture and gives it its ID. Null when the array is full.
static Texture *PushTexture(Graphics &gfx, const Texture &desc)
{
	if ( gfx.textureCount == MAX_TEXTURES )
	{
		LOG(Warning, "Could not create texture, the texture array is full.\n");
		return nullptr;
	}

	if ( desc.image )
	{
		LOG(Warning, "Could not create texture <%s>, duplicating an existing texture is not supported.\n", desc.name);
		return nullptr;
	}

	Texture &texture = gfx.textures[gfx.textureCount++];
	texture = desc;

	BindID(&texture.id, &texture, ReflexID_Texture);

	return &texture;
}

ID CreateTexture(Graphics &gfx, const Texture &desc, Image image)
{
	Texture *texture = PushTexture(gfx, desc);
	if ( !texture ) {
		return {};
	}

	texture->image = image;
	texture->ownsImage = false;
	texture->ts = 0;

	texture->size = GetImageSize(gpu, image);

	return texture->id;
}

ID CreateTexture(Graphics &gfx, const Texture &desc)
{
	ID id = {};

	// Not the exact size: a scratch slot too small for a request is never grown, so asking for
	// every texture's own size would take a new slot for each larger one until they run out
	Scratch scratch(Max(desc.location.size, (u32)MB(16)));

	if ( IsAssetFromBin(desc.flags) )
	{
		const byte *pixels = PushDataFromFile(scratch.arena, gfx.assetsFile, desc.location.offset, desc.location.size);

		Image imageHandle = GfxCreateImage(gfx, desc.name, desc.width, desc.height, desc.channels, desc.mipmap, pixels);

		id = CreateTexture(gfx, desc, imageHandle);

		if ( id ) {
			GetTexture(id).ownsImage = true;
		} else {
			DestroyImage(gpu, imageHandle);
		}

	}
	else
	{
		const FilePath imagePath = MakePath(AssetDir, desc.filename);
		ImagePixels img;
		if ( ReadImagePixels(scratch.arena, imagePath.str, img) )
		{
			Image imageHandle = GfxCreateImage(gfx, img, desc.name, desc.mipmap);

			id = CreateTexture(gfx, desc, imageHandle);

			if ( id )
			{
				Texture &texture = GetTexture(id);
				texture.ownsImage = true;
				GetFileLastWriteTimestamp(imagePath.str, texture.ts);
			}
			else
			{
				DestroyImage(gpu, imageHandle);
			}
		}
	}

	return id;
}

ID GetOrCreateTexture(Graphics &gfx, const Texture &desc)
{
	const FilePath imagePath = MakePath(AssetDir, desc.filename);

	ID id = {};
	for (u16 i = 0; i < gfx.textureCount; ++i)
	{
		const Texture &texture = gfx.textures[i];
		const Texture &desc = texture;
		if ( IsAssetFromBin(desc.flags) ) { continue; }
		const FilePath imagePath2 = MakePath(AssetDir, desc.filename);
		if ( !( desc.flags & AssetFlag_Ghost ) && StrEq(imagePath.str, imagePath2.str)) {
			id = desc.id;
			break;
		}
	}

	if ( !id )
	{
		id = CreateTexture(gfx, desc);
	}
	return id;
}

Image GetTextureImage(Graphics &gfx, ID id, Image image)
{
	Image res = image;

	if ( id ) {
		const Texture &texture = GetTexture(id);
		res = texture.image;
	}

	return res;
}

void RemoveTexture(Graphics &gfx, ID id)
{
	if ( IsBuiltin(id) )
	{
		LOG(Warning, "Ignoring an attempt to remove builtin texture <%s>.\n", GetTexture(id).name);
		return;
	}

	if (id)
	{
		// Marks only. The texture keeps its image until CompactTextures, so anything
		// still drawing with it this frame has something valid to sample.
		GetTexture(id).id = {};
		Invalidate(id);
	}
}

static COMPACT_REMOVE(RemoveTextureImage)
{
	Graphics &gfx = *(Graphics*)data;
	Texture &texture = gfx.textures[index];

	if ( texture.ownsImage && texture.image )
	{
		WaitDeviceIdle(gpu);
		DestroyImage(gpu, texture.image);
	}
}

void CompactTextures(Graphics &gfx)
{
	if ( COMPACT_ARRAY_BY_ID(Texture, gfx.textures, gfx.textureCount, id, nullptr, RemoveTextureImage, &gfx) )
	{
		gfx.shouldUpdateMaterialBindGroups = true;
	}
}

static void RecreateTextureIfModifed(Graphics &gfx, Texture &texture)
{
	const Texture &desc = texture;

	if ( !desc.id ) { return; }

	if ( IsAssetFromBin(desc.flags) ) { return; }
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
			WaitDeviceIdle(gpu);

			texture.ts = ts;

			if ( texture.ownsImage ) {
				DestroyImage(gpu, texture.image);
			}

			texture.image = GfxCreateImage(gfx, img, desc.name, desc.mipmap);
			texture.ownsImage = true;

			GetFileLastWriteTimestamp(imagePath.str, texture.ts);

			gfx.shouldUpdateMaterialBindGroups = true;
		}
	}
}

void RecreateModifiedTextures(Engine &engine)
{
	Graphics &gfx = engine.gfx;
	for (u16 i = 0; i < gfx.textureCount; ++i)
	{
		RecreateTextureIfModifed(gfx, gfx.textures[i]);
	}
}


////////////////////////////////////////////////////////////////////////
// Material management

Material &GetMaterial(ID id)
{
	if ( !id ) {
		Graphics &gfx = GetEngine().gfx;
		id = gfx.defaultMaterial;
	}
	Material &material = *((Material*)GetObject(id));
	return material;
}

// Materials keep a dense index, unlike textures: the uniform buffer offset and the
// parallel bind group array are both addressed by it.
u16 GetMaterialIndex(const Graphics &gfx, ID id)
{
	const Material &material = GetMaterial(id);
	const u16 index = (u16)(&material - gfx.materials);
	ASSERT( index < gfx.materialCount );
	return index;
}

static u32 MaterialBufferOffset(const Graphics &gfx, u16 index)
{
	const u32 offset = index * AlignUp(sizeof(SMaterial), GetDeviceCaps(gpu).alignment.uniformBufferOffset);
	return offset;
}

// Appends a material and gives it its ID. Null when the array is full.
static Material *PushMaterial(Graphics &gfx, const Material &desc)
{
	if ( gfx.materialCount == MAX_MATERIALS )
	{
		LOG(Warning, "Could not create material, the material array is full.\n");
		return nullptr;
	}

	const u16 index = (u16)gfx.materialCount++;
	Material &material = gfx.materials[index];
	material = desc;
	material.bufferOffset = MaterialBufferOffset(gfx, index);

	BindID(&material.id, &material, ReflexID_Material);

	return &material;
}

void ResolveMaterial(Graphics &gfx, Material &material)
{
	const PipelineIndex pipelineIndex = FindPipelineIndex(material.pipelineName);
	if ( pipelineIndex == Pipeline_Count )
	{
		LOG(Warning, "Material <%s> refers to pipeline <%s>, which does not exist.\n",
				material.name, material.pipelineName ? material.pipelineName : "");
		material.pipelineIndex = (u16)Pipeline_Shading;
		// The name is what gets saved, so put the fallback there too rather than write
		// the broken one back out
		material.pipelineName = GetPipelineName(material.pipelineIndex);
	}
	else
	{
		material.pipelineIndex = (u16)pipelineIndex;
	}

	gfx.shouldUpdateMaterials = true;
}

ID CreateMaterial(Graphics &gfx, const Material &desc)
{
	Material *material = PushMaterial(gfx, desc);
	if ( !material ) {
		return {};
	}

	ResolveMaterial(gfx, *material);

	CreateMaterialBindGroup(gfx, material->id);

	return material->id;
}

ID GetOrCreateMaterial(Graphics &gfx, const Material &desc)
{
	ID id = {};
	for (u32 i = 0; i < gfx.materialCount; ++i)
	{
		const Material &materialDesc = gfx.materials[i];
		if ( !( desc.flags & AssetFlag_Ghost ) && StrEq(desc.name, materialDesc.name)) {
			id = materialDesc.id;
			break;
		}
	}

	if ( !id )
	{
		id = CreateMaterial(gfx, desc);
	}
	return id;
}

void RemoveMaterial(Graphics &gfx, ID id)
{
	if ( IsBuiltin(id) )
	{
		LOG(Warning, "Ignoring an attempt to remove builtin material <%s>.\n", GetMaterial(id).name);
		return;
	}

	if (id)
	{
		// Marks only. The material keeps its slot in the uniform buffer and its bind
		// group until CompactMaterials, so anything still drawing with it this frame
		// has something valid to bind.
		GetMaterial(id).id = {};
		Invalidate(id);
	}
}

static COMPACT_MOVE(MoveMaterial)
{
	Graphics &gfx = *(Graphics*)data;

	// Offsets into the uniform buffer follow the index, and this one just changed. Both
	// this and the material itself are CPU state until the re-upload below, so nothing
	// the device can see has moved yet.
	gfx.materials[dstIndex].bufferOffset = MaterialBufferOffset(gfx, (u16)dstIndex);

	// The bind group about to be overwritten may still be bound by the in-flight frames,
	// and removing a material is rare enough that stalling here costs nothing. The
	// device is already idle by the time a second move gets here.
	WaitDeviceIdle(gpu);
	gfx.materialBindGroups[dstIndex] = gfx.materialBindGroups[srcIndex];
}

// Runs before survivors are moved over this index
static COMPACT_REMOVE(RemoveMaterial)
{
	Graphics &gfx = *(Graphics*)data;

	WaitDeviceIdle(gpu);
	DestroyBindGroup(gpu, gfx.materialBindGroups[index]);
}

void CompactMaterials(Graphics &gfx)
{
	if ( COMPACT_ARRAY_BY_ID(Material, gfx.materials, gfx.materialCount, id, MoveMaterial, RemoveMaterial, &gfx) )
	{
		gfx.shouldUpdateMaterials = true;          // re-upload at the new offsets
		gfx.shouldUpdateMaterialBindGroups = true; // re-point the bind groups at them
	}
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

static Format HdrColorFormat(const GraphicsDevice &device)
{
	const FormatSupport &support = GetDeviceCaps(device).formatSupport[FormatRG11B10F];
	const bool supported = support.colorAttachmentBlend && support.linearFilteredSampling;
	return supported ? FormatRG11B10F : FormatRGBA16F;
}

void CreateRenderTargets(Graphics &gfx, u32 sceneWidth, u32 sceneHeight)
{
	RenderTargets renderTargets = {};

	CommandList commandList = BeginTransientCommandList(gpu);

	const Format depthFormat = GetDeviceCaps(gpu).defaultDepthFormat;
	const float preRotationDegrees = GetSwapchainPreRotation(gpu);
	const uint2 swapchainSize = GetSwapchainSize(gpu);
	const bool isPortraitRotation = preRotationDegrees == 90.0f || preRotationDegrees == 270.0f;
	const u32 swapchainWidth = isPortraitRotation ? swapchainSize.y : swapchainSize.x;
	const u32 swapchainHeight = isPortraitRotation ? swapchainSize.x : swapchainSize.y;
	f32 scenePixelsPerMeter = ART_PIXELS_PER_METER;

	// Editor
	if (sceneWidth == 0 || sceneHeight == 0)
	{
		renderTargets.scaleSceneToDisplay = true;

		// Capped to the light grid's maximum, aspect kept
		const u32 maxWidth = MAX_LIGHT_GRID_WIDTH * LIGHT_TILE_SIZE;
		const u32 maxHeight = MAX_LIGHT_GRID_HEIGHT * LIGHT_TILE_SIZE;
		const f32 scale = Min(1.0f, Min((f32)maxWidth / swapchainWidth, (f32)maxHeight / swapchainHeight));
		sceneWidth = Min(maxWidth, (u32)(scale * swapchainWidth));
		sceneHeight = Min(maxHeight, (u32)(scale * swapchainHeight));
	}
	// Game
	else
	{
		renderTargets.hasBloom = true;

		// Keep scene size at a multiple of art resolution
		u32 xmul = sceneWidth / ART_WIDTH;
		u32 ymul = sceneHeight / ART_HEIGHT;
		sceneWidth = ART_WIDTH * xmul--;
		sceneHeight = ART_HEIGHT * ymul--;
		while ( sceneWidth > swapchainWidth || sceneHeight > swapchainHeight )
		{
			sceneWidth = ART_WIDTH * xmul--;
			sceneHeight = ART_HEIGHT * ymul--;
		}
		if ( sceneWidth < ART_WIDTH || sceneHeight < ART_HEIGHT )
		{
			sceneWidth = ART_WIDTH;
			sceneHeight = ART_HEIGHT;
		}
		scenePixelsPerMeter = (f32)ART_PIXELS_PER_METER * sceneHeight / ART_HEIGHT;
	}

	renderTargets.sceneSize = { sceneWidth, sceneHeight };
	renderTargets.scenePixelsPerMeter = scenePixelsPerMeter;

	// Depth buffer
	renderTargets.depthImage = CreateImage(gpu,
			sceneWidth, sceneHeight, 1,
			depthFormat,
			ImageUsageDepthStencilAttachment,
			HeapType_RTs);
	TransitionImageLayout(commandList, renderTargets.depthImage, ImageStateInitial, ImageStateRenderTarget, 0, 1);
	SetObjectNameImage(gpu, renderTargets.depthImage, "scene_depth");

	// Scene color buffer
	renderTargets.sceneImage = CreateImage(gpu,
		sceneWidth, sceneHeight, 1,
		HdrColorFormat(gpu),
		ImageUsageColorAttachment | ImageUsageSampled,
		HeapType_RTs);
	TransitionImageLayout(commandList, renderTargets.sceneImage, ImageStateInitial, ImageStateRenderTarget, 0, 1);
	SetObjectNameImage(gpu, renderTargets.sceneImage, "scene_image");

	// Scene framebuffer
	{
		const FramebufferDesc desc = {
			.renderPass = gfx.litRenderPass,
			.attachments = {
				renderTargets.sceneImage,
				renderTargets.depthImage,
			},
			.attachmentCount = 2,
		};

		renderTargets.sceneFramebuffer = CreateFramebuffer(gpu, desc);
	}

	// Distortion map
	renderTargets.distortionImage = CreateImage(gpu,
		sceneWidth, sceneHeight, 1,
		FormatRG16F,
		ImageUsageColorAttachment | ImageUsageSampled,
		HeapType_RTs);
	TransitionImageLayout(commandList, renderTargets.distortionImage, ImageStateInitial, ImageStateRenderTarget, 0, 1);
	SetObjectNameImage(gpu, renderTargets.distortionImage, "distortion_image");

	// Distortion framebuffer
	{
		const FramebufferDesc desc = {
			.renderPass = gfx.distortionRenderPass,
			.attachments = {
				renderTargets.distortionImage,
				renderTargets.depthImage,
			},
			.attachmentCount = 2,
		};

		renderTargets.distortionFramebuffer = CreateFramebuffer(gpu, desc);
	}

	// Effects color buffer
	renderTargets.effectsImage = CreateImage(gpu,
		sceneWidth, sceneHeight, 1,
		HdrColorFormat(gpu),
		ImageUsageColorAttachment | ImageUsageSampled,
		HeapType_RTs);
	TransitionImageLayout(commandList, renderTargets.effectsImage, ImageStateInitial, ImageStateRenderTarget, 0, 1);
	SetObjectNameImage(gpu, renderTargets.effectsImage, "effects_image");

	// Effects framebuffer
	{
		const FramebufferDesc desc = {
			.renderPass = gfx.effectsRenderPass,
			.attachments = {
				renderTargets.effectsImage,
				renderTargets.depthImage,
			},
			.attachmentCount = 2,
		};

		renderTargets.effectsFramebuffer = CreateFramebuffer(gpu, desc);
	}

	// Bloom pyramid
	if ( renderTargets.hasBloom )
	{
		for ( u32 i = 0; i < BLOOM_LEVEL_COUNT; ++i )
		{
			char debugName[16];
			SPrintf(debugName, "bloom_%u", i);

			const Image image = CreateImage(gpu,
				Max(sceneWidth >> (i + 1), 1u), Max(sceneHeight >> (i + 1), 1u), 1,
				HdrColorFormat(gpu),
				ImageUsageColorAttachment | ImageUsageSampled,
				HeapType_RTs);
			TransitionImageLayout(commandList, image, ImageStateInitial, ImageStateRenderTarget, 0, 1);
			SetObjectNameImage(gpu, image, debugName);
			renderTargets.bloomImages[i] = image;

			const FramebufferDesc downsampleDesc = {
				.renderPass = gfx.bloomDownsampleRenderPass,
				.attachments = { image },
				.attachmentCount = 1,
			};
			renderTargets.bloomDownsampleFramebuffers[i] = CreateFramebuffer(gpu, downsampleDesc);

			if ( i < BLOOM_LEVEL_COUNT - 1 )
			{
				const FramebufferDesc upsampleDesc = {
					.renderPass = gfx.bloomUpsampleRenderPass,
					.attachments = { image },
					.attachmentCount = 1,
				};
				renderTargets.bloomUpsampleFramebuffers[i] = CreateFramebuffer(gpu, upsampleDesc);
			}
		}

		const FramebufferDesc compositeDesc = {
			.renderPass = gfx.bloomUpsampleRenderPass,
			.attachments = { renderTargets.effectsImage },
			.attachmentCount = 1,
		};
		renderTargets.bloomCompositeFramebuffer = CreateFramebuffer(gpu, compositeDesc);
	}

	// Display framebuffer
	for ( u32 i = 0; i < GetSwapchainImageCount(gpu); ++i )
	{
		const FramebufferDesc desc = {
			.renderPass = gfx.displayRenderPass,
			.attachments = {
				GetSwapchainImage(gpu, i),
			},
			.attachmentCount = 1,
		};

		renderTargets.displayFramebuffers[i] = CreateFramebuffer(gpu, desc);
	}

	// Shadowmap
	{
		renderTargets.shadowmapImage = CreateImage(gpu,
				1024, 1024, 1,
				depthFormat,
				ImageUsageDepthStencilAttachment | ImageUsageSampled,
				HeapType_RTs);
		TransitionImageLayout(commandList, renderTargets.shadowmapImage, ImageStateInitial, ImageStateRenderTarget, 0, 1);
		SetObjectNameImage(gpu, renderTargets.shadowmapImage, "scene_shadowmap");

		const FramebufferDesc desc = {
			.renderPass = gfx.shadowmapRenderPass,
			.attachments = { renderTargets.shadowmapImage },
			.attachmentCount = 1,
		};

		renderTargets.shadowmapFramebuffer = CreateFramebuffer( gpu, desc );
	}

#if USE_EDITOR
	// ID buffer
	{
		renderTargets.idImage = CreateImage(gpu,
			 sceneWidth, sceneHeight, 1,
			 FormatUInt,
			 ImageUsageColorAttachment | ImageUsageSampled,
			 HeapType_RTs);
		TransitionImageLayout(commandList, renderTargets.idImage, ImageStateInitial, ImageStateRenderTarget, 0, 1);
		SetObjectNameImage(gpu, renderTargets.idImage, "scene_id");

		const FramebufferDesc desc = {
			.renderPass = gfx.idRenderPass,
			.attachments = { renderTargets.idImage, renderTargets.depthImage },
			.attachmentCount = 2,
		};

		renderTargets.idFramebuffer = CreateFramebuffer( gpu, desc );
	}
#endif

	EndTransientCommandList(gpu, commandList);

	renderTargets.initialized = true;

	gfx.renderTargets = renderTargets;
	gfx.globalBindGroupUpdateMask = (1u << MAX_FRAMES_IN_FLIGHT) - 1;
}

void DestroyRenderTargets(Graphics &gfx, RenderTargets &renderTargets)
{
	if ( !renderTargets.initialized )
	{
		return;
	}

	DestroyImage(gpu, renderTargets.depthImage);
	DestroyImage(gpu, renderTargets.sceneImage);
	DestroyImage(gpu, renderTargets.distortionImage);
	DestroyImage(gpu, renderTargets.effectsImage);

	if ( renderTargets.hasBloom )
	{
		for ( u32 i = 0; i < BLOOM_LEVEL_COUNT; ++i )
		{
			DestroyImage(gpu, renderTargets.bloomImages[i]);
			DestroyFramebuffer( gpu, renderTargets.bloomDownsampleFramebuffers[i] );
			if ( i < BLOOM_LEVEL_COUNT - 1 )
			{
				DestroyFramebuffer( gpu, renderTargets.bloomUpsampleFramebuffers[i] );
			}
		}
		DestroyFramebuffer( gpu, renderTargets.bloomCompositeFramebuffer );
	}

	// Reset the heap used for render targets
	ResetHeap(gpu, HeapType_RTs);

	DestroyFramebuffer( gpu, renderTargets.sceneFramebuffer );
	DestroyFramebuffer( gpu, renderTargets.distortionFramebuffer );
	DestroyFramebuffer( gpu, renderTargets.effectsFramebuffer );

	for ( u32 i = 0; i < GetSwapchainImageCount(gpu); ++i )
	{
		DestroyFramebuffer( gpu, renderTargets.displayFramebuffers[i] );
	}

	DestroyImage(gpu, renderTargets.shadowmapImage);
	DestroyFramebuffer( gpu, renderTargets.shadowmapFramebuffer );

#if USE_EDITOR
	DestroyImage(gpu, renderTargets.idImage);
	DestroyFramebuffer( gpu, renderTargets.idFramebuffer );
#endif

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

static const ShaderAndPipelineDesc pipelineDescs[] =
{
	{
		.vsName = "vs_shading_3d",
		.fsName = "fs_shading_3d",
		.renderPass = "scene_renderpass",
		.index = Pipeline_Shading,
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
		},
	},
	{
		.vsName = "vs_shading_2d",
		.fsName = "fs_shading_2d",
		.renderPass = "scene_renderpass",
		.index = Pipeline_Shading2D,
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
		},
	},
	{
		.vsName = "vs_shading_2d_tile",
		.fsName = "fs_shading_2d_tile",
		.renderPass = "scene_renderpass",
		.index = Pipeline_Shading2DTile,
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
		.index = Pipeline_Shadowmap,
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
		.index = Pipeline_Sky,
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
		.index = Pipeline_Grid2D,
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
		.index = Pipeline_Grid3D,
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
		.index = Pipeline_Blit,
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
		.vsName = "vs_blit",
		.fsName = "fs_distortion",
		.renderPass = "effects_renderpass",
		.index = Pipeline_Distortion,
		.desc = {
			.name = "pipeline_distortion",
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
		.index = Pipeline_UI,
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
		.index = Pipeline_ModelId,
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
		.index = Pipeline_SpriteId,
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
		.index = Pipeline_DebugDraw,
		.desc = {
			.name = "pipeline_debug_draw",
			.vsFunction = "VSMain",
			.fsFunction = "PSMain",
			.vertexBufferCount = 1,
			.vertexBuffers = { { .stride = 24 }, },
			.vertexAttributeCount = 3,
			.vertexAttributes = {
				{ .bufferIndex = 0, .location = 0, .offset = 0,  .format = FormatFloat3, },
				{ .bufferIndex = 0, .location = 1, .offset = 12, .format = FormatFloat2, },
				{ .bufferIndex = 0, .location = 2, .offset = 20, .format = FormatRGBA8, },
			},
			.depthTest = false,
			.blending = true,
		}
	},
	{
		.vsName = "vs_particles",
		.fsName = "fs_particles",
		.renderPass = "scene_renderpass",
		.index = Pipeline_Particles,
		.desc = {
			.name = "pipeline_particles",
			.vsFunction = "VSMain",
			.fsFunction = "PSMain",
			.vertexBufferCount = 1,
			.vertexBuffers = { { .stride = 28 }, },
			.vertexAttributeCount = 4,
			.vertexAttributes = {
				{ .bufferIndex = 0, .location = 0, .offset = 0,  .format = FormatFloat3, },
				{ .bufferIndex = 0, .location = 1, .offset = 12, .format = FormatFloat2, },
				{ .bufferIndex = 0, .location = 2, .offset = 20, .format = FormatRGBA8, },
				{ .bufferIndex = 0, .location = 3, .offset = 24, .format = FormatFloat, },
			},
			.depthTest = true,
			.depthWrite = false,
			.depthCompareOp = CompareOpGreaterOrEqual,
			.blending = true,
		}
	},
	{
		.vsName = "vs_distortion_map",
		.fsName = "fs_distortion_map",
		.renderPass = "distortion_renderpass",
		.index = Pipeline_DistortionMap,
		.desc = {
			.name = "pipeline_distortion_map",
			.vsFunction = "VSMain",
			.fsFunction = "PSMain",
			.vertexBufferCount = 1,
			.vertexBuffers = { { .stride = 24 }, },
			.vertexAttributeCount = 3,
			.vertexAttributes = {
				{ .bufferIndex = 0, .location = 0, .offset = 0,  .format = FormatFloat3, },
				{ .bufferIndex = 0, .location = 1, .offset = 12, .format = FormatFloat2, },
				{ .bufferIndex = 0, .location = 2, .offset = 20, .format = FormatFloat, },
			},
			.depthTest = true,
			.depthWrite = false,
			.depthCompareOp = CompareOpGreaterOrEqual,
			.additiveBlending = true,
		}
	},
	{
		.vsName = "vs_fog",
		.fsName = "fs_fog",
		.renderPass = "scene_renderpass",
		.index = Pipeline_Fog,
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
			.depthTest = true,
			.depthWrite = false,
			.depthCompareOp = CompareOpGreaterOrEqual,
			.blending = true,
		}
	},
	{
		.vsName = "vs_blit",
		.fsName = "fs_bloom_prefilter",
		.renderPass = "bloom_downsample_renderpass",
		.index = Pipeline_BloomPrefilter,
		.desc = {
			.name = "pipeline_bloom_prefilter",
			.vsFunction = "VSMain",
			.fsFunction = "PSPrefilter",
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
		.vsName = "vs_blit",
		.fsName = "fs_bloom_downsample",
		.renderPass = "bloom_downsample_renderpass",
		.index = Pipeline_BloomDownsample,
		.desc = {
			.name = "pipeline_bloom_downsample",
			.vsFunction = "VSMain",
			.fsFunction = "PSDownsample",
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
		.vsName = "vs_blit",
		.fsName = "fs_bloom_upsample",
		.renderPass = "bloom_upsample_renderpass",
		.index = Pipeline_BloomUpsample,
		.desc = {
			.name = "pipeline_bloom_upsample",
			.vsFunction = "VSMain",
			.fsFunction = "PSUpsample",
			.vertexBufferCount = 1,
			.vertexBuffers = { { .stride = 32 }, },
			.vertexAttributeCount = 2,
			.vertexAttributes = {
				{ .bufferIndex = 0, .location = 0, .offset = 0,  .format = FormatFloat3, },
				{ .bufferIndex = 0, .location = 1, .offset = 24, .format = FormatFloat2, },
			},
			.depthTest = false,
			.additiveBlending = true,
		}
	},
	{
		.vsName = "vs_blit",
		.fsName = "fs_bloom_composite",
		.renderPass = "bloom_upsample_renderpass",
		.index = Pipeline_BloomComposite,
		.desc = {
			.name = "pipeline_bloom_composite",
			.vsFunction = "VSMain",
			.fsFunction = "PSComposite",
			.vertexBufferCount = 1,
			.vertexBuffers = { { .stride = 32 }, },
			.vertexAttributeCount = 2,
			.vertexAttributes = {
				{ .bufferIndex = 0, .location = 0, .offset = 0,  .format = FormatFloat3, },
				{ .bufferIndex = 0, .location = 1, .offset = 24, .format = FormatFloat2, },
			},
			.depthTest = false,
			.additiveBlending = true,
		}
	},
};

static const ShaderAndComputeDesc computeDescs[] =
{
	{
		.csName = "compute_select",
		.index = Pipeline_ComputeSelect,
		.desc = {
			.name = "compute_select",
			.function = "CSMain"
		},
	},
	{
		.csName = "light_binning",
		.index = Pipeline_LightBinning,
		.desc = {
			.name = "light_binning",
			.function = "CSMain"
		},
	},
};

PipelineIndex FindPipelineIndex(const char *name)
{
	if ( name )
	{
		for (u32 i = 0; i < ARRAY_COUNT(pipelineDescs); ++i)
		{
			if ( StrEq(pipelineDescs[i].desc.name, name) ) {
				return pipelineDescs[i].index;
			}
		}
	}
	return Pipeline_Count; // Not found
}

const char *GetPipelineName(u16 index)
{
	for (u32 i = 0; i < ARRAY_COUNT(pipelineDescs); ++i)
	{
		if ( pipelineDescs[i].index == index ) {
			return pipelineDescs[i].desc.name;
		}
	}
	return "";
}

void CompileGraphicsPipeline(Engine &engine, Arena scratch, const ShaderAndPipelineDesc &shaderPipeDesc)
{
	Graphics &gfx = engine.gfx;

	const RenderPass renderPass = FindRenderPassHandle(gfx, shaderPipeDesc.renderPass);

	PipelineDesc desc = shaderPipeDesc.desc;
	desc.renderPass = renderPass;

	if ( sLoadShadersFromText )
	{
		desc.vertexShaderSource = GetShaderSource(scratch, shaderPipeDesc.vsName);
		desc.fragmentShaderSource = GetShaderSource(scratch, shaderPipeDesc.fsName);
	}
	else
	{
		desc.vertexShaderSource = GetShaderSource(engine.shaderAssets, shaderPipeDesc.vsName);
		desc.fragmentShaderSource = GetShaderSource(engine.shaderAssets, shaderPipeDesc.fsName);
	}

	LOG(Info, "Creating Graphics Pipeline: %s\n", desc.name);
	// Written in place, so everything reading through gfx.pipelines picks up the new one
	Pipeline &pipeline = gfx.pipelines[shaderPipeDesc.index];
	if ( pipeline ) {
		DestroyPipeline( gpu, pipeline );
	}
	pipeline = CreateGraphicsPipeline(gpu, scratch, desc, gfx.globalBindGroupLayout);
	SetObjectNamePipeline(gpu, pipeline, desc.name);
}

void CompileComputePipeline(Engine &engine, Arena scratch, const ShaderAndComputeDesc &shaderComputeDesc)
{
	Graphics &gfx = engine.gfx;

	ComputeDesc desc = shaderComputeDesc.desc;

	if ( sLoadShadersFromText )
	{
		desc.computeShaderSource = GetShaderSource(scratch, shaderComputeDesc.csName);
	}
	else
	{
		desc.computeShaderSource = GetShaderSource(engine.shaderAssets, shaderComputeDesc.csName);
	}

	LOG(Info, "Creating Compute Pipeline: %s\n", desc.name);
	Pipeline &pipeline = gfx.pipelines[shaderComputeDesc.index];
	if ( pipeline ) {
		DestroyPipeline( gpu, pipeline );
	}
	pipeline = CreateComputePipeline(gpu, scratch, desc, gfx.globalBindGroupLayout);
	SetObjectNamePipeline(gpu, pipeline, desc.name);
}


// Rebuilds every pipeline in place. Everything reads through gfx.pipelines, so nothing
// has to be re-linked afterwards.
void RecompilePipelines(Engine &engine, Arena scratch)
{
	for (u32 i = 0; i < ARRAY_COUNT(pipelineDescs); ++i)
	{
		CompileGraphicsPipeline(engine, scratch, pipelineDescs[i]);
	}

	// Compute pipelines
	for (u32 i = 0; i < ARRAY_COUNT(computeDescs); ++i)
	{
		CompileComputePipeline(engine, scratch, computeDescs[i]);
	}
}

void ResetDynamicBindGroups( Graphics &gfx )
{
	ResetBindGroupAllocator( gpu, gfx.dynamicBindGroupAllocator[GetFrameSlot(gpu)] );
	MemSet(gfx.dynamicBindGroupDescs, gfx.dynamicBindGroupCount * sizeof(BindGroupDesc), 0);
	gfx.dynamicBindGroupCount = 0;
}

BindGroup GetOrCreateDynamicBindGroup(Graphics &gfx, const BindGroupDesc &bindGroupDesc)
{
	for (u32 i = 0; i < gfx.dynamicBindGroupCount; ++i)
	{
		if ( MemCompare( &gfx.dynamicBindGroupDescs[i], &bindGroupDesc, sizeof(BindGroupDesc) ) == 0 )
		{
			return gfx.dynamicBindGroups[i];
		}
	}

	ASSERT( gfx.dynamicBindGroupCount < ARRAY_COUNT(gfx.dynamicBindGroups) );
	const BindGroup bindGroup = CreateFullBindGroup(gpu, bindGroupDesc, gfx.dynamicBindGroupAllocator[GetFrameSlot(gpu)]);
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

	if ( !InitializeGraphicsDevice( gpu, scratch.arena ) ) {
		return false;
	}

	LOG(Info, "HDR color format: %s\n", FormatName(HdrColorFormat(gpu)));

	// Scene render pass
	{
		const RenderpassDesc renderpassDesc = {
			.name = "scene_renderpass",
			.colorAttachmentCount = 1,
			.colorAttachments = {
				{ .format = HdrColorFormat(gpu), .loadOp = LoadOpClear, .storeOp = StoreOpStore, .isSwapchain = false },
			},
			.hasDepthAttachment = true,
			.depthAttachment = {
				.loadOp = LoadOpClear, .storeOp = StoreOpStore,
			}
		};
		gfx.litRenderPass = CreateRenderPass( gpu, renderpassDesc );
	}

	// Distortion render pass
	// Additive offsets on a cleared map, depth tested against the scene
	{
		const RenderpassDesc renderpassDesc = {
			.name = "distortion_renderpass",
			.colorAttachmentCount = 1,
			.colorAttachments = {
				{ .format = FormatRG16F, .loadOp = LoadOpClear, .storeOp = StoreOpStore, .isSwapchain = false },
			},
			.hasDepthAttachment = true,
			.depthAttachment = {
				.loadOp = LoadOpLoad, .storeOp = StoreOpStore,
			}
		};
		gfx.distortionRenderPass = CreateRenderPass( gpu, renderpassDesc );
	}

	// Effects render pass (continue after scene render pass)
	// Color is overwritten by the scene copy, depth is kept from the scene
	{
		const RenderpassDesc renderpassDesc = {
			.name = "effects_renderpass",
			.colorAttachmentCount = 1,
			.colorAttachments = {
				{ .format = HdrColorFormat(gpu), .loadOp = LoadOpDontCare, .storeOp = StoreOpStore, .isSwapchain = false },
			},
			.hasDepthAttachment = true,
			.depthAttachment = {
				.loadOp = LoadOpLoad, .storeOp = StoreOpStore,
			}
		};
		gfx.effectsRenderPass = CreateRenderPass( gpu, renderpassDesc );
	}

	// Bloom render passes
	{
		const RenderpassDesc downsampleDesc = {
			.name = "bloom_downsample_renderpass",
			.colorAttachmentCount = 1,
			.colorAttachments = {
				{ .format = HdrColorFormat(gpu), .loadOp = LoadOpDontCare, .storeOp = StoreOpStore, .isSwapchain = false },
			},
		};
		gfx.bloomDownsampleRenderPass = CreateRenderPass( gpu, downsampleDesc );

		const RenderpassDesc upsampleDesc = {
			.name = "bloom_upsample_renderpass",
			.colorAttachmentCount = 1,
			.colorAttachments = {
				{ .format = HdrColorFormat(gpu), .loadOp = LoadOpLoad, .storeOp = StoreOpStore, .isSwapchain = false },
			},
		};
		gfx.bloomUpsampleRenderPass = CreateRenderPass( gpu, upsampleDesc );
	}

	// Display render pass
	{
		const Format format = GetSwapchainFormat(gpu);

		const RenderpassDesc renderpassDesc = {
			.name = "display_renderpass",
			.colorAttachmentCount = 1,
			.colorAttachments = {
				{ .format = format, .loadOp = LoadOpClear, .storeOp = StoreOpStore, .isSwapchain = true },
			},
			.hasDepthAttachment = false,
		};
		gfx.displayRenderPass = CreateRenderPass( gpu, renderpassDesc );
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
		gfx.shadowmapRenderPass = CreateRenderPass( gpu, renderpassDesc );
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
		gfx.idRenderPass = CreateRenderPass( gpu, renderpassDesc );
	}

	// Create staging buffer
	gfx.stagingBuffer = CreateStagingBuffer(gfx);

	// Create global geometry buffers
	gfx.globalVertexArena = MakeBufferArena( gfx, CreateVertexBuffer(gfx, MB(4)) );
	gfx.globalIndexArena = MakeBufferArena( gfx, CreateIndexBuffer(gfx, MB(4)) );

#define MAX_DEBUG_DRAW_VERTICES (6 * 16384)

	// Create debug draw vertex buffers
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		gfx.debugDrawVertexBuffer[i] = CreateBuffer(
			gpu,
			sizeof(DebugDrawVertex) * MAX_DEBUG_DRAW_VERTICES,
			BufferUsageVertexBuffer,
			HeapType_Dynamic);

		gfx.debugDrawVertices[i] = (DebugDrawVertex*)GetBufferPtr(gpu, gfx.debugDrawVertexBuffer[i]);
	}
	gfx.debugDrawVerticesCPU = PushArray(globalArena, DebugDrawVertex, MAX_DEBUG_DRAW_VERTICES);

	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		gfx.particleVertexBuffer[i] = CreateBuffer(
			gpu,
			sizeof(ParticleVertex) * MAX_PARTICLES * 6,
			BufferUsageVertexBuffer,
			HeapType_Dynamic);

		gfx.distortionVertexBuffer[i] = CreateBuffer(
			gpu,
			sizeof(DistortionVertex) * MAX_PARTICLES * 6,
			BufferUsageVertexBuffer,
			HeapType_Dynamic);
	}

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
	gfx.screenTriangleVertices90 = PushData(gfx, commandList, gfx.globalVertexArena, screenTriangleVertices90, sizeof(screenTriangleVertices90));
	gfx.screenTriangleVertices180 = PushData(gfx, commandList, gfx.globalVertexArena, screenTriangleVertices180, sizeof(screenTriangleVertices180));
	gfx.screenTriangleVertices270 = PushData(gfx, commandList, gfx.globalVertexArena, screenTriangleVertices270, sizeof(screenTriangleVertices270));
	gfx.screenTriangleIndices = PushData(gfx, commandList, gfx.globalIndexArena, screenTriangleIndices, sizeof(screenTriangleIndices));

	EndUploadCommandList(gfx, commandList);

	// Create globals buffer
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		const u32 globalsBufferSize = sizeof(Globals);
		gfx.globalsBuffer[i] = CreateBuffer(
			gpu,
			globalsBufferSize,
			BufferUsageUniformBuffer,
			HeapType_Dynamic);
	}

	// Create entities buffer
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		const u32 entityBufferSize = MAX_ENTITIES * AlignUp( sizeof(SEntity), GetDeviceCaps(gpu).alignment.uniformBufferOffset );
		gfx.entityBuffer[i] = CreateBuffer(
			gpu,
			entityBufferSize,
			BufferUsageStorageBuffer,
			HeapType_Dynamic);
	}

	// Create sprite data buffer
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		const u32 spriteDataBufferSize = (MAX_SPRITES) * sizeof(SSpriteData);
		gfx.spriteDataBuffer[i] = CreateBuffer(
			gpu,
			spriteDataBufferSize,
			BufferUsageStorageBuffer,
			HeapType_Dynamic);
	}

	// Create tile data buffer
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		const u32 tileDataBufferSize = MAX_TILES * sizeof(STileData);
		gfx.tileDataBuffer[i] = CreateBuffer(
			gpu,
			tileDataBufferSize,
			BufferUsageStorageBuffer,
			HeapType_Dynamic);
	}

	// Create light buffers
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		const u32 lightBufferSize = MAX_VISIBLE_LIGHTS * sizeof(SLight);
		gfx.lightBuffer[i] = CreateBuffer(
			gpu,
			lightBufferSize,
			BufferUsageStorageBuffer,
			HeapType_Dynamic);
	}

	// Create light grid buffers. GPU only: the culling compute writes them, shading reads them.
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		const u32 lightGridBufferSize = MAX_LIGHT_GRID_CELLS * LIGHT_CELL_STRIDE;
		gfx.lightGridBuffer[i] = CreateBuffer(
			gpu,
			lightGridBufferSize,
			BufferUsageStorageBuffer,
			HeapType_General);
	}


	// Create material buffer
	const u32 materialBufferSize = MAX_MATERIALS * AlignUp( sizeof(SMaterial), GetDeviceCaps(gpu).alignment.uniformBufferOffset );
	gfx.materialBuffer = CreateBuffer(gpu, materialBufferSize, BufferUsageUniformBuffer | BufferUsageTransferDst, HeapType_General);

	// Create buffer for computes
	const u32 computeBufferSize = sizeof(float);
	gfx.computeBuffer = CreateBuffer(gpu, computeBufferSize, BufferUsageStorageTexelBuffer, HeapType_General);
	gfx.computeBufferView = CreateBufferView(gpu, gfx.computeBuffer, FormatFloat, 0, 0);

#if USE_EDITOR
	const u32 selectionBufferSize = AlignUp( sizeof(u32), GetDeviceCaps(gpu).alignment.storageBufferOffset );
	gfx.selectionBuffer = CreateBuffer(gpu, selectionBufferSize, BufferUsageStorageTexelBuffer, HeapType_Readback);
	gfx.selectionBufferView = CreateBufferView(gpu, gfx.selectionBuffer, FormatUInt, 0, 0);
#endif // USE_EDITOR


	// Create Global BindGroup allocator
	{
		const BindGroupAllocatorDesc desc = {
			.name = "GlobalBindGroup",
			.counts = {
				.uniformBufferCount = MAX_FRAMES_IN_FLIGHT,
				.storageBufferCount = MAX_FRAMES_IN_FLIGHT * 5,
				.textureCount = 1000,
				.samplerCount = MAX_FRAMES_IN_FLIGHT * 5,
				.groupCount = MAX_FRAMES_IN_FLIGHT,
			},
		};
		gfx.globalBindGroupAllocator = CreateBindGroupAllocator(gpu, desc);
	}

	// Create Material BindGroup allocator
	{
		const BindGroupAllocatorDesc desc = {
			.name = "MaterialBindGroup",
			.counts = {
				.uniformBufferCount = MAX_MATERIALS,
				.textureCount = MAX_MATERIALS,
				.groupCount = MAX_MATERIALS,
				.allowIndividualFrees = true,
			},
		};
		gfx.materialBindGroupAllocator = CreateBindGroupAllocator(gpu, desc);
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
		gfx.dynamicBindGroupAllocator[i] = CreateBindGroupAllocator(gpu, desc);
	}

	// Create global BindGroup layout

	const ShaderBinding globalShaderBindings[] = {
		{ .set = 0, .binding = BINDING_GLOBALS, .type = ShaderBindingTypeUniformBuffer, .stageFlags = ShaderStageVertex | ShaderStageFragment | ShaderStageCompute },
		{ .set = 0, .binding = BINDING_SAMPLER_POINT, .type = ShaderBindingTypeSampler, .stageFlags = ShaderStageFragment },
		{ .set = 0, .binding = BINDING_SAMPLER_LINEAR, .type = ShaderBindingTypeSampler, .stageFlags = ShaderStageFragment },
		{ .set = 0, .binding = BINDING_ENTITIES, .type = ShaderBindingTypeStorageBuffer, .stageFlags = ShaderStageVertex },
		{ .set = 0, .binding = BINDING_SHADOWMAP, .type = ShaderBindingTypeImage, .stageFlags = ShaderStageFragment },
		{ .set = 0, .binding = BINDING_SHADOWMAP_SAMPLER, .type = ShaderBindingTypeSampler, .stageFlags = ShaderStageFragment },
		{ .set = 0, .binding = BINDING_SPRITE_DATA, .type = ShaderBindingTypeStorageBuffer, .stageFlags = ShaderStageVertex },
		{ .set = 0, .binding = BINDING_TILE_DATA, .type = ShaderBindingTypeStorageBuffer, .stageFlags = ShaderStageVertex },
		{ .set = 0, .binding = BINDING_NOISE2D, .type = ShaderBindingTypeImage, .stageFlags = ShaderStageFragment },
		{ .set = 0, .binding = BINDING_LIGHTS, .type = ShaderBindingTypeStorageBuffer, .stageFlags = ShaderStageFragment | ShaderStageCompute },
		{ .set = 0, .binding = BINDING_LIGHT_GRID, .type = ShaderBindingTypeStorageBuffer, .stageFlags = ShaderStageFragment },
	};
	gfx.globalBindGroupLayout = CreateBindGroupLayout(gpu, globalShaderBindings, ARRAY_COUNT(globalShaderBindings));

	// Graphics pipelines
	ResetArena(scratch.arena);
	RecompilePipelines(engine, scratch.arena);

	// Builtin images
	const byte whiteImagePixels[] = { 255, 255, 255, 255 };
	gfx.whiteImage = GfxCreateImage(gfx, "whiteImage", 1, 1, 4, false, whiteImagePixels);
	const byte pinkImagePixels[] = { 255, 0, 255, 255 };
	gfx.pinkImage = GfxCreateImage(gfx, "pinkImage", 1, 1, 4, false, pinkImagePixels);
	const byte grayImagePixels[] = { 127, 127, 127, 255 };
	gfx.grayImage = GfxCreateImage(gfx, "grayImage", 1, 1, 4, false, grayImagePixels);
	const byte blackImagePixels[] = { 0, 0, 0, 0 };
	gfx.blackImage = GfxCreateImage(gfx, "blackImage", 1, 1, 4, false, blackImagePixels);
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
	gfx.noiseImage = GfxCreateImage(gfx, "noiseImage", noiseWidth, noiseHeight, 4, false, (byte*)noiseImagePixels);

	// Builtin texture
	const Texture defaultTextureDesc = {
		.id = { BuiltinID_DefaultTexture },
		.name = InternString("tex_default"),
		.filename = InternString(""),
		.mipmap = 0,
		.flags = AssetFlag_Ghost | AssetFlag_Builtin,
	};
	gfx.defaultTexture = CreateTexture(gfx, defaultTextureDesc, gfx.pinkImage);

	// Builtin noise texture
	const Texture noiseTextureDesc = {
		.id = { BuiltinID_NoiseTexture },
		.name = InternString("tex_noise"),
		.filename = InternString(""),
		.mipmap = 0,
		.flags = AssetFlag_Ghost | AssetFlag_Builtin,
	};
	gfx.noiseTexture = CreateTexture(gfx, noiseTextureDesc, gfx.noiseImage);

	// Builtin material
	const Material materialDesc = {
		.id = { BuiltinID_DefaultMaterial },
		.name = InternString("mat_default"),
		.textureId = gfx.defaultTexture,
		.pipelineName = InternString("pipeline_shading"),
		.uvScale = 1.0,
		.flags = AssetFlag_Ghost | AssetFlag_Builtin,
	};
	gfx.defaultMaterial = CreateMaterial(gfx, materialDesc);

	// Samplers
	const SamplerDesc pointSamplerDesc = {
		.addressMode = AddressModeRepeat,
		.filter = FilterNearest,
	};
	gfx.pointSampler = CreateSampler(gpu, pointSamplerDesc);
	const SamplerDesc materialSamplerDesc = {
		.addressMode = AddressModeRepeat,
		.filter = FilterLinear,
	};
	gfx.linearSampler = CreateSampler(gpu, materialSamplerDesc);
	const SamplerDesc screenSamplerDesc = {
		.addressMode = AddressModeClampToEdge,
		.filter = FilterNearest,
	};
	gfx.screenSampler = CreateSampler(gpu, screenSamplerDesc);
	const SamplerDesc bloomSamplerDesc = {
		.addressMode = AddressModeClampToEdge,
		.filter = FilterLinear,
	};
	gfx.bloomSampler = CreateSampler(gpu, bloomSamplerDesc);
	const SamplerDesc shadowmapSamplerDesc = {
		.addressMode = AddressModeClampToBorder,
		.filter = FilterNearest,
		.borderColor = BorderColorBlackFloat,
		.compareOp = CompareOpGreater,
	};
	gfx.shadowmapSampler = CreateSampler(gpu, shadowmapSamplerDesc);
	const SamplerDesc skySamplerDesc = {
		.addressMode = AddressModeClampToEdge,
		.filter = FilterLinear,
	};
	gfx.skySampler = CreateSampler(gpu, skySamplerDesc);

	// BindGroups for globals
	for (u32 i = 0; i < ARRAY_COUNT(gfx.globalBindGroups); ++i)
	{
		gfx.globalBindGroups[i] = CreateBindGroup(gpu, gfx.globalBindGroupLayout, gfx.globalBindGroupAllocator);
	}

	gfx.globalBindGroupUpdateMask = (1u << MAX_FRAMES_IN_FLIGHT) - 1;

	// Timestamp queries
	PROFILE_GPU_INIT(gpu);

#if USE_UI
	UIIcon *icons = nullptr;
	u32 iconCount = 0;
#if USE_EDITOR
	ResetArena(scratch.arena);
	iconCount = ARRAY_COUNT(sEditorIconFilenames);
	icons = PushArray(globalArena, UIIcon, iconCount);
	for (u32 i = 0; i < iconCount; ++i) {
		FilePath filepath = MakePath(ProjectDir, sEditorIconFilenames[i]);
		ReadImagePixels(scratch.arena, filepath.str, icons[i].image);
	}
#endif
	UI_Initialize(engine.ui, gfx, gpu, globalArena, icons, iconCount);
#endif

	gfx.camera = {
		.projectionType = ProjectionOrthographic,
		.znear = -10.0f,
		.zfar = 10.0f,
		.height = 8.0f,
	};


	gfx.deviceInitialized = true;

	return true;
}

BindGroupDesc GlobalBindGroupDesc(const Graphics &gfx, u32 frameSlot)
{
	const BindGroupDesc bindGroupDesc = {
		.layout = gfx.globalBindGroupLayout,
		.bindings = {
			{ .index = BINDING_GLOBALS, .buffer = gfx.globalsBuffer[frameSlot] },
			{ .index = BINDING_SAMPLER_POINT, .sampler = gfx.pointSampler },
			{ .index = BINDING_SAMPLER_LINEAR, .sampler = gfx.linearSampler },
			{ .index = BINDING_ENTITIES, .buffer = gfx.entityBuffer[frameSlot] },
			{ .index = BINDING_SHADOWMAP, .image = gfx.renderTargets.shadowmapImage },
			{ .index = BINDING_SHADOWMAP_SAMPLER, .sampler = gfx.shadowmapSampler },
			{ .index = BINDING_SPRITE_DATA, .buffer = gfx.spriteDataBuffer[frameSlot] },
			{ .index = BINDING_TILE_DATA, .buffer = gfx.tileDataBuffer[frameSlot] },
			{ .index = BINDING_NOISE2D, .image = gfx.noiseImage },
			{ .index = BINDING_LIGHTS, .buffer = gfx.lightBuffer[frameSlot] },
			{ .index = BINDING_LIGHT_GRID, .buffer = gfx.lightGridBuffer[frameSlot] },
		},
	};
	return bindGroupDesc;
}

void UpdateGlobalBindGroup(Graphics &gfx, u32 frameSlot)
{
	const BindGroupDesc globalBindGroupDesc = GlobalBindGroupDesc(gfx, frameSlot);
	UpdateBindGroup(gpu, globalBindGroupDesc, gfx.globalBindGroups[frameSlot]);
}

BindGroupDesc MaterialBindGroupDesc(Graphics &gfx, const Material &material)
{
	const BindGroupLayout bindGroupLayout = GetPipelineBindGroupLayout(gpu, gfx.pipelines[material.pipelineIndex], BIND_GROUP_MATERIAL);
	const Image &albedoImage = GetTextureImage(gfx, material.textureId, gfx.pinkImage );

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
	for (u32 i = 0; i < gfx.materialCount; ++i)
	{
		const Material &material = gfx.materials[i];
		const BindGroupDesc materialBindGroupDesc = MaterialBindGroupDesc(gfx, material);
		UpdateBindGroup(gpu, materialBindGroupDesc, gfx.materialBindGroups[i]);
	}
}

void UploadMaterialData(Graphics &gfx)
{
	CommandList commandList = BeginUploadCommandList(gfx);

	// Copy material info to buffer
	for (u32 i = 0; i < gfx.materialCount; ++i)
	{
		const Material &material = gfx.materials[i];
		SMaterial shaderMaterial = { material.uvScale };
		StagedData staged = StageData(gfx, &shaderMaterial, sizeof(shaderMaterial));

		CopyBufferToBuffer(commandList, staged.buffer, staged.offset, gfx.materialBuffer, material.bufferOffset, sizeof(shaderMaterial));
	}

	EndUploadCommandList(gfx, commandList);
}

void CreateMaterialBindGroup(Graphics &gfx, ID materialId)
{
	const Material &material = GetMaterial(materialId);
	gfx.materialBindGroups[GetMaterialIndex(gfx, materialId)] = CreateBindGroup(gpu, GetPipelineBindGroupLayout(gpu, gfx.pipelines[material.pipelineIndex], 1), gfx.materialBindGroupAllocator);
	gfx.shouldUpdateMaterialBindGroups = true;
}

void CreateMaterialBindGroups(Graphics &gfx)
{
	// BindGroups for materials
	for (u32 i = 0; i < gfx.materialCount; ++i)
	{
		CreateMaterialBindGroup(gfx, gfx.materials[i].id);
	}
}

void GfxWaitDeviceIdle(Graphics &gfx)
{
	WaitDeviceIdle(gpu);

	gfx.stagingBufferOffset = 0;
}

void CleanupGraphics(Graphics &gfx)
{
	GfxWaitDeviceIdle( gfx );

	PROFILE_GPU_CLEANUP(gpu);

	DestroyBindGroupAllocator( gpu, gfx.globalBindGroupAllocator );
	DestroyBindGroupAllocator( gpu, gfx.materialBindGroupAllocator );
	for (u32 i = 0; i < ARRAY_COUNT(gfx.dynamicBindGroupAllocator); ++i)
	{
		DestroyBindGroupAllocator( gpu, gfx.dynamicBindGroupAllocator[i] );
	}

	CleanupGraphicsDevice( gpu, FrameArena );

	CleanupGraphicsDriver( gpu );

	ZeroStruct( &gfx ); // deviceInitialized = false;
}


const Image GetDisplayImage(const Graphics &gfx)
{
	const u32 imageIndex = GetSwapchainImageIndex(gpu);
	const Image displayImage = GetSwapchainImage(gpu, imageIndex);
	return displayImage;
}

Framebuffer GetDisplayFramebuffer(const Graphics &gfx)
{
	const u32 imageIndex = GetSwapchainImageIndex(gpu);
	return gfx.renderTargets.displayFramebuffers[imageIndex];
}

Framebuffer GetShadowmapFramebuffer(const Graphics &gfx)
{
	return gfx.renderTargets.shadowmapFramebuffer;
}
