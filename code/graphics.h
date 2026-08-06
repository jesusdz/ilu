#ifndef GRAPHICS_H
#define GRAPHICS_H

// NOTE: ImagePixels is defined in engine.cpp before ilu_ui.h, as the UI needs it there.

struct Engine;

typedef u16 Index;

struct Vertex
{
	float3 pos;
	float3 normal;
	float2 texCoord;
};

struct DebugDrawVertex
{
	float2 pos;
	float2 texCoord;
	rgba color;
};

struct DebugDrawBatch
{
	ImageH imageH;
	u32 vertexIndex;
	u32 vertexCount;
};

struct Texture
{
	TextureDesc desc;
	ImageH image;
	bool ownsImage;
	uint2 size;
	u64 ts;
};

struct Material
{
	MaterialDesc desc;
	PipelineH pipelineH; // Resolved from desc.pipelineName
	u32 bufferOffset;    // Derived from the element index, so it moves with compaction
};

struct RenderTargets
{
	uint2 sceneSize;
	ImageH depthImage;
	ImageH sceneImage;
	Framebuffer sceneFramebuffer;

	Framebuffer displayFramebuffers[MAX_SWAPCHAIN_IMAGE_COUNT];

	ImageH shadowmapImage;
	Framebuffer shadowmapFramebuffer;

	ImageH idImage;
	Framebuffer idFramebuffer;

	bool initialized;
};

// NOTE: Camera lives here rather than in render.h because Graphics embeds one.
enum ProjectionType
{
	ProjectionPerspective,
	ProjectionOrthographic,
	ProjectionTypeCount,
};

struct Camera
{
	ProjectionType projectionType;
	float3 position;
	float2 orientation; // yaw and pitch
	f32 znear;
	f32 zfar;
	f32 height; // orthographic only: half the vertical size of the view volume
	f32 fovy;   // perspective only: vertical field of view, in degrees
};

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

#define MAX_TEXTURES 4092
#define MAX_MATERIALS 4092
#define MAX_DYNAMIC_BIND_GROUPS 4092
#define MAX_DEBUG_DRAW_BATCHES 64

struct Graphics
{
	GraphicsDevice device;

	RenderTargets renderTargets;

	BufferH stagingBuffer;
	u32 stagingBufferOffset;
	bool inUploadContext;

	BufferArena globalVertexArena;
	BufferArena globalIndexArena;

	BufferChunk cubeVertices;
	BufferChunk cubeIndices;
	BufferChunk planeVertices;
	BufferChunk planeIndices;
	BufferChunk quadVertices;
	BufferChunk quadIndices;
	BufferChunk spriteVertices;
	BufferChunk spriteIndices;
	BufferChunk screenTriangleVertices;
	BufferChunk screenTriangleIndices;

	BufferH globalsBuffer[MAX_FRAMES_IN_FLIGHT];
	BufferH entityBuffer[MAX_FRAMES_IN_FLIGHT];
	BufferH materialBuffer;
	BufferH computeBufferH;
	BufferViewH computeBufferViewH;
#if USE_EDITOR
	BufferH selectionBufferH;
	BufferViewH selectionBufferViewH;
#endif

	BufferH debugDrawVertexBuffer[MAX_FRAMES_IN_FLIGHT];
	DebugDrawVertex *debugDrawVertices[MAX_FRAMES_IN_FLIGHT];
	DebugDrawVertex *debugDrawVerticesCPU;
	u32 debugDrawVertexCount;
	DebugDrawBatch debugDrawBatches[MAX_DEBUG_DRAW_BATCHES];
	u32 debugDrawBatchCount;

	BufferH spriteDataBuffer[MAX_FRAMES_IN_FLIGHT];
	BufferH tileDataBuffer[MAX_FRAMES_IN_FLIGHT];

	SamplerH pointSamplerH;
	SamplerH linearSamplerH;
	SamplerH shadowmapSamplerH;
	SamplerH skySamplerH;
	SamplerH screenSamplerH;

	RenderPassH litRenderPassH;
	RenderPassH shadowmapRenderPassH;
	RenderPassH idRenderPassH;
	RenderPassH displayRenderPassH;

	u32 textureCount;
	Texture textures[MAX_TEXTURES];

	u32 materialCount;
	Material materials[MAX_MATERIALS];
	bool shouldUpdateMaterials;

	BindGroupAllocator globalBindGroupAllocator;
	BindGroupAllocator materialBindGroupAllocator;
	BindGroupAllocator dynamicBindGroupAllocator[MAX_FRAMES_IN_FLIGHT];

	BindGroupLayout globalBindGroupLayout;

	// Updated each frame so we need MAX_FRAMES_IN_FLIGHT elements
	BindGroup globalBindGroups[MAX_FRAMES_IN_FLIGHT];
	bool shouldUpdateGlobalBindGroups;

	BindGroup materialBindGroups[MAX_MATERIALS]; // Parallel to materials
	bool shouldUpdateMaterialBindGroups;

	BindGroupDesc dynamicBindGroupDescs[MAX_DYNAMIC_BIND_GROUPS];
	BindGroup dynamicBindGroups[MAX_DYNAMIC_BIND_GROUPS];
	u32 dynamicBindGroupCount;

	ImageH whiteImageH;
	ImageH pinkImageH;
	ImageH grayImageH;
	ImageH blackImageH;
	ImageH noiseImageH;

	ID skyTexture;
	ID defaultTexture;
	ID noiseTexture;

	ID defaultMaterial;

	PipelineH shadowmapPipelineH;
	PipelineH skyPipelineH;
	PipelineH spritePipelineH;
	PipelineH tilePipelineH;
	PipelineH blitPipelineH;
	PipelineH guiPipelineH;
#if USE_EDITOR
	PipelineH grid2dPipelineH;
	PipelineH grid3dPipelineH;
	PipelineH modelIdPipelineH;
	PipelineH spriteIdPipelineH;
#endif
	PipelineH debugDrawPipelineH;
	PipelineH fogPipelineH;

#define USE_COMPUTE_TEST 0
#if USE_COMPUTE_TEST
	PipelineH computeClearH;
	PipelineH computeUpdateH;
#endif
	PipelineH computeSelectH;

	bool deviceInitialized;

	f32 deltaSeconds;

	Camera camera;
};


////////////////////////////////////////////////////////////////////////
// Image loading

bool ReadImagePixels(Arena &arena, const char *filepath, ImagePixels &image);
ImagePixels ResizeImagePixels(Arena &arena, ImagePixels inputImagePixels, i32 w, i32 h);


////////////////////////////////////////////////////////////////////////
// Shader source and pipeline descriptor tables

ShaderSourceDesc *GetShaderSourceDescs();
u32 GetShaderSourceDescCount();

const u32 FindShaderSourceDescIndex(const char *name);
RenderPassH FindRenderPassHandle(const Graphics &gfx, const char *name);
PipelineH FindPipelineHandle(const Graphics &gfx, const char *name);


////////////////////////////////////////////////////////////////////////
// Buffers and data upload

BufferH CreateStagingBuffer(Graphics &gfx);
BufferH CreateVertexBuffer(Graphics &gfx, u32 size);
BufferH CreateIndexBuffer(Graphics &gfx, u32 size);
BufferArena MakeBufferArena(Graphics &gfx, BufferH bufferHandle);
void UploadData(Graphics &gfx, const CommandList &commandList, const void *data, u32 size, BufferH destBuffer, u32 destOffset, u32 alignment = 0);
BufferChunk PushData(Graphics &gfx, const CommandList &commandList, BufferArena &arena, const void *data, u32 size, u32 alignment = 0);


////////////////////////////////////////////////////////////////////////
// Image management

void GenerateMipmaps(const GraphicsDevice &device, const CommandList &commandList, ImageH imageH);
ImageH EngineCreateImage(Graphics &gfx, const char *name, int width, int height, int channels, bool mipmap, const byte *pixels);
ImageH EngineCreateImage(Graphics &gfx, const ImagePixels &img, const char *name, bool createMipmaps);


////////////////////////////////////////////////////////////////////////
// Texture management

Texture &GetTexture(ID id);
Texture &GetTextureAt(Graphics &gfx, u32 index);
ID CreateTexture(Graphics &gfx, const TextureDesc &desc, ImageH imageH);
ID CreateTexture(Graphics &gfx, const TextureDesc &desc);
ID GetOrCreateTexture(Graphics &gfx, const TextureDesc &desc);
ID CreateTexture(Graphics &gfx, const BinImage &binImage);
ImageH GetTextureImage(Graphics &gfx, ID textureId, ImageH imageH);
void RemoveTexture(Graphics &gfx, ID textureId);
void CompactTextures(Graphics &gfx);
void RecreateModifiedTextures(Engine &engine);


////////////////////////////////////////////////////////////////////////
// Material management

Material &GetMaterial(ID id);
u16 GetMaterialIndex(const Graphics &gfx, ID materialId);
ID CreateMaterial(Graphics &gfx, const MaterialDesc &desc);
ID GetOrCreateMaterial(Graphics &gfx, const MaterialDesc &desc);
ID CreateMaterial(Graphics &gfx, const BinMaterialDesc &desc);
void RemoveMaterial(Graphics &gfx, ID materialId);
void CompactMaterials(Graphics &gfx);


////////////////////////////////////////////////////////////////////////
// Builtin geometry

BufferChunk GetVerticesForGeometryType(Graphics &gfx, GeometryType geometryType);
BufferChunk GetIndicesForGeometryType(Graphics &gfx, GeometryType geometryType);


////////////////////////////////////////////////////////////////////////
// Pipeline compilation

void CompileGraphicsPipeline(Engine &engine, Arena scratch, u32 pipelineIndex);
void CompileComputePipeline(Engine &engine, Arena scratch, u32 pipelineIndex);
void RecompilePipelines(Engine &engine, Arena scratch);


////////////////////////////////////////////////////////////////////////
// Dynamic bind groups

void ResetDynamicBindGroups(Graphics &gfx);
const BindGroup &GetOrCreateDynamicBindGroup(Graphics &gfx, const BindGroupDesc &bindGroupDesc);


////////////////////////////////////////////////////////////////////////
// Render targets

void CreateRenderTargets(Graphics &gfx, u32 sceneWidth = 0, u32 sceneHeight = 0);
void DestroyRenderTargets(Graphics &gfx, RenderTargets &renderTargets);


////////////////////////////////////////////////////////////////////////
// Device lifetime and bind groups

void LinkHandles(Graphics &gfx);
bool InitializeGraphics(Engine &engine, Arena &globalArena);
BindGroupDesc GlobalBindGroupDesc(const Graphics &gfx, u32 frameIndex);
void UpdateGlobalBindGroups(Graphics &gfx);
BindGroupDesc MaterialBindGroupDesc(Graphics &gfx, const Material &material);
void UpdateMaterialBindGroups(Graphics &gfx);
void UploadMaterialData(Graphics &gfx);
void CreateMaterialBindGroup(Graphics &gfx, ID materialId);
void CreateMaterialBindGroups(Graphics &gfx);
void EngineWaitDeviceIdle(Graphics &gfx);
void CleanupGraphics(Graphics &gfx);


////////////////////////////////////////////////////////////////////////
// Framebuffer and swapchain queries

uint2 GetFramebufferSize(const Framebuffer &framebuffer);
const ImageH GetDisplayImageH(const Graphics &gfx);
const Image &GetDisplayImage(const Graphics &gfx);
Framebuffer GetDisplayFramebuffer(const Graphics &gfx);
Framebuffer GetShadowmapFramebuffer(const Graphics &gfx);

#endif // GRAPHICS_H
