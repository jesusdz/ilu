/*
 * ilu_gfx.h
 * Author: Jesus Diaz Garcia
 *
 * Graphics API abstraction made on top of Vulkan.
 *
 * These are the functions abstracted from the low-level API.
 *
 * Device initialization:
 * - (Create/Destroy)Swapchain
 * - (Initialize/Cleanup)GraphicsDriver
 * - (Initialize/Cleanup)GraphicsSurface
 * - (Initialize/Cleanup)GraphicsDevice
 *
 * Resource management:
 * - (Create/Destroy/Reset)BindGroupAllocator
 * - (Create/Destroy)BindGroupLayout
 * - (Create/Update)BindGroup
 * - (Create/Destroy)Buffer
 * - (Create/Destroy)BufferView
 * - (Create/Destroy)Image
 * - (Create/Destroy)Sampler
 * - CreateGraphicsPipeline
 * - CreateComputePipeline
 * - DestroyPipeline
 * - (Create/Destroy)RenderPass
 * - (Create/Destroy)Framebuffer
 *
 * Command lists
 * - (Begin/End)CommandList
 * - (Begin/End)TransientCommandList
 *
 * Commands:
 * - CopyBufferToBuffer
 * - CopyBufferToImage
 * - Blit
 * - TransitionImageLayout
 * - TransitionBufferState
 * - (Begin/End)RenderPass
 * - SetViewport
 * - SetScissor
 * - SetViewportAndScissor
 * - SetClear(Color/Depth/Stencil)
 * - SetPipeline
 * - SetBindGroup
 * - SetVertexBuffer
 * - SetIndexBuffer
 * - Draw
 * - DrawIndexed
 * - Dispatch
 *
 * Timestamp queries:
 * - CreateTimestampPool
 * - DestroyTimestampPool
 * - ResetTimestampPool
 * - WriteTimestamp
 * - ReadTimestamp
 * - ReadTimestamps
 *
 * Work submission and synchronization:
 * - Submit
 * - Present
 * - (Begin/End)Frame
 * - WaitQueueIdle
 * - WaitDeviceIdle
 *
 * Debug utils:
 * - BeginDebugGroup / EndDebugGroup
 * - InsertDebugLabel
 * - SetObjectName
 */

#ifndef ILU_GFX_H
#define ILU_GFX_H


////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////

#include "ilu_core.h"
#include "handle_pool.h"

//#define SPV_PRINT_FUNCTIONS
#include "ilu_spirv.h"


////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////

#define MAX_BIND_GROUPS SPV_MAX_DESCRIPTOR_SETS
#define MAX_SHADER_BINDINGS SPV_MAX_DESCRIPTORS_PER_SET
#define MAX_BIND_GROUP_LAYOUTS 256
#define MAX_FENCES 128
#define MAX_RENDER_TARGETS 4
#define MAX_BUFFERS 64
#define MAX_IMAGES 128
#define MAX_SAMPLERS 64
#define MAX_PIPELINES 64
#define MAX_RENDERPASSES 8
#define MAX_FRAMEBUFFERS 64
#define MAX_BIND_GROUP_ALLOCATORS 8
#define MAX_ALLOCATED_BIND_GROUPS 8192 // Across all allocators
#define MAX_TIMESTAMP_POOLS 4
#define MAX_COMMAND_LISTS 8
#define MAX_COLOR_ATTACHMENTS 3
#define MAX_DEPTH_ATTACHMENTS 1
#if PLATFORM_ANDROID
#define MAX_SWAPCHAIN_IMAGE_COUNT 5
#else
#define MAX_SWAPCHAIN_IMAGE_COUNT 4
#endif
#define FIRST_SWAPCHAIN_IMAGE_INDEX MAX_IMAGES
#define MAX_FRAMES_IN_FLIGHT 2


////////////////////////////////////////////////////////////////////////
// Types
////////////////////////////////////////////////////////////////////////

enum HeapType
{
	HeapType_General,
	HeapType_RTs,
	HeapType_Staging,
	HeapType_Dynamic,
	HeapType_Readback,
	HeapType_COUNT,
};

constexpr u32 HeapSize_General = MB(64);

enum BorderColor
{
	BorderColorBlackInt,
	BorderColorWhiteInt,
	BorderColorBlackFloat,
	BorderColorWhiteFloat,
	BorderColorCount,
};

enum AddressMode
{
	AddressModeRepeat,
	AddressModeClampToBorder,
	AddressModeClampToEdge,
	AddressModeCount,
};

enum Filter
{
	FilterNearest,
	FilterLinear,
	FilterCount,
};

enum BufferUsageFlagBits
{
	BufferUsageTransferSrc = 1<<0,
	BufferUsageTransferDst = 1<<1,
	BufferUsageUniformBuffer = 1<<2,
	BufferUsageUniformTexelBuffer = 1<<3,
	BufferUsageStorageBuffer = 1<<4,
	BufferUsageStorageTexelBuffer = 1<<5,
	BufferUsageVertexBuffer = 1<<6,
	BufferUsageIndexBuffer = 1<<7,
};

typedef u32 BufferUsageFlags;

enum ImageUsageFlagBits
{
  ImageUsageTransferSrc = 1<<0,
  ImageUsageTransferDst = 1<<1,
  ImageUsageSampled = 1<<2,
  ImageUsageStorage = 1<<3,
  ImageUsageColorAttachment = 1<<4,
  ImageUsageDepthStencilAttachment = 1<<5,
  ImageUsageTransient = 1<<6,
  ImageUsageInputAttachment = 1<<7,
};

typedef u32 ImageUsageFlags;

enum ImageState
{
	ImageStateInitial,
	ImageStateTransferSrc,
	ImageStateTransferDst,
	ImageStateShaderInput,
	ImageStateRenderTarget,
};

enum BufferState
{
	BufferStateInitial,
	BufferStateTransferSrc,
	BufferStateTransferDst,
	BufferStateVertexInput,
	BufferStateIndexInput,
	BufferStateIndirectArgs,
	BufferStateVertexRead,
	BufferStateFragmentRead,
	BufferStateComputeRead,
	BufferStateComputeWrite,
};

enum PipelineStage
{
	PipelineStageTop,
	PipelineStageVertex,
	PipelineStageFragment,
	PipelineStageCompute,
	PipelineStageBottom,
};

enum LoadOp
{
	LoadOpLoad,
	LoadOpClear,
	LoadOpDontCare,
};

enum StoreOp
{
	StoreOpStore,
	StoreOpDontCare,
};

enum CompareOp
{
	CompareOpNone,
	CompareOpLess,
	CompareOpLessOrEqual,
	CompareOpGreater,
	CompareOpGreaterOrEqual,
	CompareOpEqual,
	CompareOpCount,
};

enum Format
{
	FormatUInt,
	FormatFloat,
	FormatFloat2,
	FormatFloat3,
	FormatR8,
	FormatR8_SRGB,
	FormatRG8,
	FormatRG8_SRGB,
	FormatRGB8,
	FormatRGB8_SRGB,
	FormatRGBA8,
	FormatRGBA8_SRGB,
	FormatBGRA8,
	FormatBGRA8_SRGB,
	FormatD32,
	FormatD32S1,
	FormatD24S1,
	FormatRG16F,
	FormatRG11B10F,
	FormatRGBA16F,
	FormatCount,
	FormatInvalid = FormatCount,
};

// Freed handles stay valid until the end of the frame, when the pools are compacted
#define DECLARE_GFX_HANDLE_TYPE(Name) \
	DECLARE_HANDLE_TYPE(Name); \
	inline bool operator==(Name a, Name b) { return a.gen == b.gen && a.slot == b.slot; } \
	inline bool operator!=(Name a, Name b) { return !(a == b); } \
	inline bool IsValid(Name h) { return h.gen != 0; }

DECLARE_GFX_HANDLE_TYPE(BufferH)
DECLARE_GFX_HANDLE_TYPE(BufferViewH)
DECLARE_GFX_HANDLE_TYPE(ImageH)
DECLARE_GFX_HANDLE_TYPE(SamplerH)
DECLARE_GFX_HANDLE_TYPE(PipelineH)
DECLARE_GFX_HANDLE_TYPE(RenderPassH)
DECLARE_GFX_HANDLE_TYPE(FramebufferH)
DECLARE_GFX_HANDLE_TYPE(BindGroupLayoutH)
DECLARE_GFX_HANDLE_TYPE(BindGroupAllocatorH)
DECLARE_GFX_HANDLE_TYPE(BindGroupH)
DECLARE_GFX_HANDLE_TYPE(TimestampPoolH)

// Its recording state lives in the backend, at `index`
struct GraphicsDevice;
struct CommandList
{
	GraphicsDevice *device;
	u32 index;
};

struct BlitRegion
{
	i32 x;
	i32 y;
	i32 width;
	i32 height;
	u32 mipLevel;
};

struct ShaderBinding
{
	u8 set;
	u8 binding;
	SpvType type;
	SpvStageFlags stageFlags;
	const char *name;
};

struct BindGroupAllocatorCounts
{
	union
	{
		struct
		{
			u32 uniformBufferCount;
			u32 storageBufferCount;
			u32 storageTexelBufferCount;
			u32 textureCount;
			u32 samplerCount;
			u32 combinedImageSamplerCount;
			u32 groupCount;
		};
		u32 type[7];
	};
	bool allowIndividualFrees;
};

struct BindGroupAllocatorDesc
{
	const char *name;
	BindGroupAllocatorCounts counts;
};

struct ResourceBinding
{
	u8 index;
	union
	{
		struct
		{
			BufferH buffer;
			u32 offset;
			u32 range;
		};
		BufferViewH bufferView;
		ImageH image;
		SamplerH sampler;
	};
};

struct BindGroupDesc
{
	BindGroupLayoutH layout;
	ResourceBinding bindings[MAX_SHADER_BINDINGS];
};

struct BufferArena
{
	BufferH buffer;
	u32 used;
};

struct BufferChunk
{
	BufferH buffer;
	u32 offset;
	u32 size;
};

struct ImagePixels
{
	byte *pixels;
	i32 width;
	i32 height;
	i32 channelCount;
	bool constPixels;
};

struct AttachmentDesc
{
	Format format;
	LoadOp loadOp;
	StoreOp storeOp;
	bool isSwapchain;
};

struct RenderpassDesc
{
	const char *name;
	unsigned char colorAttachmentCount;
	AttachmentDesc colorAttachments[4];
	bool hasDepthAttachment;
	AttachmentDesc depthAttachment;
};

struct FramebufferDesc
{
	RenderPassH renderPass;
	ImageH attachments[4];
	u32 attachmentCount;
};

struct SamplerDesc
{
	AddressMode addressMode;
	Filter filter;
	BorderColor borderColor;
	CompareOp compareOp;
};

struct ShaderSource
{
	u8 *data;
	u64 dataSize;
};

struct VertexBufferDesc
{
	unsigned int stride;
};

struct VertexAttributeDesc
{
	unsigned int bufferIndex;
	unsigned int location;
	unsigned int offset;
	Format format;
};

struct PipelineDesc
{
	const char *name;
	ShaderSource vertexShaderSource;
	ShaderSource fragmentShaderSource;
	const char *vsFunction;
	const char *fsFunction;
	unsigned int vertexBufferCount;
	VertexBufferDesc vertexBuffers[4];
	unsigned int vertexAttributeCount;
	VertexAttributeDesc vertexAttributes[4];
	bool depthTest;
	bool depthWrite;
	CompareOp depthCompareOp;
	bool blending;
	bool additiveBlending;
	RenderPassH renderPass;
};

struct ComputeDesc
{
	const char *name;
	ShaderSource computeShaderSource;
	const char *function;
};

struct Alignment
{
	u32 uniformBufferOffset;
	u32 storageBufferOffset;
	u32 optimalBufferCopyOffset;
	u32 optimalBufferCopyRowPitch;
	// u32 nonCoherentAtomSize;
	// VkExtent3D minImageTransferGranularity;
};

struct FormatSupport
{
	bool linearFilteredSampling;
	bool colorAttachmentBlend;
};

struct DeviceCaps
{
	Format defaultDepthFormat;
	Alignment alignment;
	bool timestampQueries;
	FormatSupport formatSupport[FormatCount];
};

struct GraphicsDevice;

struct Timestamp
{
	double millis;
};


////////////////////////////////////////////////////////////////////////
// Function type declarations
////////////////////////////////////////////////////////////////////////

class Window; // Required platform type
class WindowImpl; // Required platform type

typedef void FN_SetGraphicsStringInterning(StringInterning *stringInterning);
typedef void FN_WaitQueueIdle(const GraphicsDevice &device);
typedef void FN_WaitDeviceIdle(const GraphicsDevice &device);
typedef void FN_BeginDebugGroup(const CommandList &cmd, const char *labelName, float4 labelColor);
typedef void FN_EndDebugGroup(const CommandList &cmd);
typedef void FN_InsertDebugLabel(const CommandList &cmd, const char *labelName, float4 labelColor);
typedef void FN_SetObjectNamePipeline(const GraphicsDevice &device, PipelineH handle, const char *name);
typedef void FN_SetObjectNameImage(const GraphicsDevice &device, ImageH handle, const char *name);
typedef void FN_RecreateSwapchain(GraphicsDevice &device, Window &window);
typedef void FN_DestroySwapchain(GraphicsDevice &device);
typedef bool FN_IsValidSwapchain(const GraphicsDevice &device);
typedef GraphicsDevice *FN_CreateGraphicsDevice();
typedef bool FN_InitializeGraphicsDriver(GraphicsDevice &device, Arena scratch);
typedef bool FN_InitializeGraphicsSurface(GraphicsDevice &device, const Window &window);
typedef bool FN_InitializeGraphicsDevice(GraphicsDevice &device, Arena scratch);
typedef bool FN_IsValidGraphicsDevice(const GraphicsDevice &device);
typedef BindGroupAllocatorH FN_CreateBindGroupAllocator(GraphicsDevice &device, const BindGroupAllocatorDesc &desc);
typedef void FN_DestroyBindGroupAllocator(GraphicsDevice &device, BindGroupAllocatorH allocatorH);
typedef void FN_ResetBindGroupAllocator(GraphicsDevice &device, BindGroupAllocatorH allocatorH);
typedef BindGroupLayoutH FN_CreateBindGroupLayout(GraphicsDevice &device, const ShaderBinding bindings[], u8 bindingCount);
typedef BindGroupH FN_CreateBindGroup(GraphicsDevice &device, BindGroupLayoutH layoutH, BindGroupAllocatorH allocatorH);
typedef BindGroupH FN_CreateFullBindGroup(GraphicsDevice &device, const BindGroupDesc &desc, BindGroupAllocatorH allocatorH);
typedef void FN_DestroyBindGroup(GraphicsDevice &device, BindGroupH bindGroupH);
typedef void FN_UpdateBindGroup(const GraphicsDevice &device, const BindGroupDesc &bindGroupDesc, BindGroupH bindGroupH);
typedef BufferH FN_CreateBuffer(GraphicsDevice &device, u32 size, BufferUsageFlags bufferUsageFlags, HeapType heapType);
typedef void FN_DestroyBufferH(GraphicsDevice &device, BufferH bufferH);
typedef BufferViewH FN_CreateBufferView(GraphicsDevice &device, BufferH bufferHandle, Format format, u32 offset, u32 size);
typedef void FN_DestroyBufferViewH(GraphicsDevice &device, BufferViewH bufferViewH);
typedef ImageH FN_CreateImage(GraphicsDevice &device, u32 width, u32 height, u32 mipLevels, Format format, ImageUsageFlags usage, HeapType heapType);
typedef bool FN_IsSwapchainImage(ImageH image);
typedef void FN_DestroyImageH(GraphicsDevice &device, ImageH imageH);
typedef SamplerH FN_CreateSampler(GraphicsDevice &device, const SamplerDesc &desc);
typedef void FN_DestroySamplerH(GraphicsDevice &device, SamplerH samplerH);
typedef PipelineH FN_CreateGraphicsPipeline(GraphicsDevice &device, Arena scratch, const PipelineDesc &desc, BindGroupLayoutH globalBindGroupLayout);
typedef PipelineH FN_CreateComputePipeline(GraphicsDevice &device, Arena scratch, const ComputeDesc &desc, BindGroupLayoutH globalBindGroupLayout);
typedef bool FN_IsSamePipeline(PipelineH a, PipelineH b);
typedef void FN_DestroyPipelineH(GraphicsDevice &device, PipelineH handle);
typedef RenderPassH FN_CreateRenderPass(GraphicsDevice &device, const RenderpassDesc &desc);
typedef void FN_DestroyRenderPassH(GraphicsDevice &device, RenderPassH renderPassH);
typedef FramebufferH FN_CreateFramebuffer(GraphicsDevice &device, const FramebufferDesc &desc);
typedef void FN_DestroyFramebuffer(GraphicsDevice &device, FramebufferH framebufferH);
typedef uint2 FN_GetFramebufferSize(const GraphicsDevice &device, FramebufferH framebufferH);
typedef CommandList FN_BeginCommandList(GraphicsDevice &device);
typedef void FN_EndCommandList(const CommandList &commandList);
typedef CommandList FN_BeginTransientCommandList(GraphicsDevice &device);
typedef void FN_EndTransientCommandList(GraphicsDevice &device, const CommandList &commandList);
typedef void* FN_GetBufferPtr(const GraphicsDevice &device, BufferH bufferH);
typedef void FN_CopyBufferToBuffer(const CommandList &commandBuffer, BufferH srcBufferH, u32 srcOffset, BufferH dstBufferH, u32 dstOffset, u64 size);
typedef void FN_CopyBufferToImage(const CommandList &commandBuffer, BufferH bufferH, u32 bufferOffset, ImageH imageH);
typedef void FN_Blit(const CommandList &commandBuffer, ImageH srcImage, const BlitRegion &srcRegion, ImageH dstImage, const BlitRegion &dstRegion);
typedef void FN_TransitionImageLayout(const CommandList &commandBuffer, ImageH imageH, ImageState oldState, ImageState newState, u32 baseMipLevel, u32 levelCount);
typedef void FN_TransitionBufferState(const CommandList &commandBuffer, BufferH bufferH, BufferState oldState, BufferState newState);
typedef void FN_BeginRenderPass(const CommandList &commandList, FramebufferH framebufferH);
typedef void FN_SetViewport(const CommandList &commandList, rect viewport);
typedef void FN_SetScissor(const CommandList &commandList, rect scissor);
typedef void FN_SetViewportAndScissor(const CommandList &commandList, uint2 size);
typedef void FN_SetClearColorFloat4(const CommandList &commandList, u32 renderTargetIndex, float4 color);
typedef void FN_SetClearColorU32(const CommandList &commandList, u32 renderTargetIndex, u32 color);
typedef void FN_SetClearDepth(const CommandList &commandList, u32 renderTargetIndex, float depth);
typedef void FN_SetClearStencil(const CommandList &commandList, u32 renderTargetIndex, u32 stencil);
typedef void FN_SetPipeline(const CommandList &commandList, PipelineH pipelineH);
typedef void FN_SetBindGroup(const CommandList &commandList, u32 bindGroupIndex, BindGroupH bindGroupH);
typedef void FN_SetVertexBuffer(const CommandList &commandList, BufferH bufferH);
typedef void FN_SetIndexBuffer(const CommandList &commandList, BufferH bufferH);
typedef void FN_Draw(const CommandList &commandList, u32 vertexCount, u32 firstVertex);
typedef void FN_DrawIndexed(const CommandList &commandList, u32 indexCount, u32 firstIndex, u32 firstVertex, u32 firstInstance, u32 instanceCount);
typedef void FN_Dispatch(const CommandList &commandList, u32 x, u32 y, u32 z);
typedef void FN_EndRenderPass(const CommandList &commandList);
typedef TimestampPoolH FN_CreateTimestampPool(GraphicsDevice &device, u32 maxQueries);
typedef void FN_DestroyTimestampPool(GraphicsDevice &device, TimestampPoolH poolH);
typedef void FN_ResetTimestampPool(const CommandList &commandBuffer, TimestampPoolH poolH);
typedef u32 FN_WriteTimestamp(const CommandList &commandBuffer, TimestampPoolH poolH, PipelineStage stage);
typedef Timestamp FN_ReadTimestamp(const GraphicsDevice &device, TimestampPoolH poolH, u32 queryIndex);
typedef bool FN_ReadTimestamps(const GraphicsDevice &device, TimestampPoolH poolH, u32 firstQuery, u32 queryCount, Timestamp *outTimestamps);
typedef void FN_Submit(GraphicsDevice &device, const CommandList &commandList);
typedef bool FN_Present(GraphicsDevice &device);
typedef bool FN_BeginFrame(GraphicsDevice &device);
typedef void FN_EndFrame(GraphicsDevice &device);
typedef void FN_CleanupGraphicsDevice(GraphicsDevice &device, Arena scratch);
typedef void FN_CleanupGraphicsSurface(const GraphicsDevice &device);
typedef void FN_CleanupGraphicsDriver(GraphicsDevice &device);
typedef const char* FN_FormatName(Format format);
typedef const DeviceCaps& FN_GetDeviceCaps(const GraphicsDevice &device);
typedef u32 FN_GetFrameSlot(const GraphicsDevice &device);
typedef u32 FN_GetHeapSize(const GraphicsDevice &device, HeapType heapType);
typedef void FN_ResetHeap(GraphicsDevice &device, HeapType heapType);
typedef RenderPassH FN_FindRenderPass(const GraphicsDevice &device, const char *name);
typedef uint2 FN_GetSwapchainSize(const GraphicsDevice &device);
typedef Format FN_GetSwapchainFormat(const GraphicsDevice &device);
typedef float FN_GetSwapchainPreRotation(const GraphicsDevice &device);
typedef u32 FN_GetSwapchainImageCount(const GraphicsDevice &device);
typedef u32 FN_GetSwapchainImageIndex(const GraphicsDevice &device);
typedef ImageH FN_GetSwapchainImage(const GraphicsDevice &device, u32 index);
typedef u32 FN_GetBufferSize(const GraphicsDevice &device, BufferH handle);
typedef uint2 FN_GetImageSize(const GraphicsDevice &device, ImageH handle);
typedef Format FN_GetImageFormat(const GraphicsDevice &device, ImageH handle);
typedef u32 FN_GetImageMipLevels(const GraphicsDevice &device, ImageH handle);
typedef BindGroupLayoutH FN_GetPipelineBindGroupLayout(const GraphicsDevice &device, PipelineH handle, u32 bindGroupIndex);

#define EXPAND_API_TOOLS_GFX(EXPAND_MACRO) \
	EXPAND_MACRO(SetGraphicsStringInterning) \
	EXPAND_MACRO(WaitQueueIdle) \
	EXPAND_MACRO(WaitDeviceIdle) \
	EXPAND_MACRO(BeginDebugGroup) \
	EXPAND_MACRO(EndDebugGroup) \
	EXPAND_MACRO(InsertDebugLabel) \
	EXPAND_MACRO(SetObjectNamePipeline) \
	EXPAND_MACRO(SetObjectNameImage) \
	EXPAND_MACRO(RecreateSwapchain) \
	EXPAND_MACRO(DestroySwapchain) \
	EXPAND_MACRO(IsValidSwapchain) \
	EXPAND_MACRO(CreateGraphicsDevice) \
	EXPAND_MACRO(InitializeGraphicsDriver) \
	EXPAND_MACRO(InitializeGraphicsSurface) \
	EXPAND_MACRO(InitializeGraphicsDevice) \
	EXPAND_MACRO(IsValidGraphicsDevice) \
	EXPAND_MACRO(CreateBindGroupAllocator) \
	EXPAND_MACRO(DestroyBindGroupAllocator) \
	EXPAND_MACRO(ResetBindGroupAllocator) \
	EXPAND_MACRO(CreateBindGroupLayout) \
	EXPAND_MACRO(CreateBindGroup) \
	EXPAND_MACRO(CreateFullBindGroup) \
	EXPAND_MACRO(DestroyBindGroup) \
	EXPAND_MACRO(UpdateBindGroup) \
	EXPAND_MACRO(CreateBuffer) \
	EXPAND_MACRO(DestroyBufferH) \
	EXPAND_MACRO(CreateBufferView) \
	EXPAND_MACRO(DestroyBufferViewH) \
	EXPAND_MACRO(CreateImage) \
	EXPAND_MACRO(IsSwapchainImage) \
	EXPAND_MACRO(DestroyImageH) \
	EXPAND_MACRO(CreateSampler) \
	EXPAND_MACRO(DestroySamplerH) \
	EXPAND_MACRO(CreateGraphicsPipeline) \
	EXPAND_MACRO(CreateComputePipeline) \
	EXPAND_MACRO(IsSamePipeline) \
	EXPAND_MACRO(DestroyPipelineH) \
	EXPAND_MACRO(CreateRenderPass) \
	EXPAND_MACRO(DestroyRenderPassH) \
	EXPAND_MACRO(CreateFramebuffer) \
	EXPAND_MACRO(DestroyFramebuffer) \
	EXPAND_MACRO(GetFramebufferSize) \
	EXPAND_MACRO(BeginCommandList) \
	EXPAND_MACRO(EndCommandList) \
	EXPAND_MACRO(BeginTransientCommandList) \
	EXPAND_MACRO(EndTransientCommandList) \
	EXPAND_MACRO(GetBufferPtr) \
	EXPAND_MACRO(CopyBufferToBuffer) \
	EXPAND_MACRO(CopyBufferToImage) \
	EXPAND_MACRO(Blit) \
	EXPAND_MACRO(TransitionImageLayout) \
	EXPAND_MACRO(TransitionBufferState) \
	EXPAND_MACRO(BeginRenderPass) \
	EXPAND_MACRO(SetViewport) \
	EXPAND_MACRO(SetScissor) \
	EXPAND_MACRO(SetViewportAndScissor) \
	EXPAND_MACRO(SetClearColorFloat4) \
	EXPAND_MACRO(SetClearColorU32) \
	EXPAND_MACRO(SetClearDepth) \
	EXPAND_MACRO(SetClearStencil) \
	EXPAND_MACRO(SetPipeline) \
	EXPAND_MACRO(SetBindGroup) \
	EXPAND_MACRO(SetVertexBuffer) \
	EXPAND_MACRO(SetIndexBuffer) \
	EXPAND_MACRO(Draw) \
	EXPAND_MACRO(DrawIndexed) \
	EXPAND_MACRO(Dispatch) \
	EXPAND_MACRO(EndRenderPass) \
	EXPAND_MACRO(CreateTimestampPool) \
	EXPAND_MACRO(DestroyTimestampPool) \
	EXPAND_MACRO(ResetTimestampPool) \
	EXPAND_MACRO(WriteTimestamp) \
	EXPAND_MACRO(ReadTimestamp) \
	EXPAND_MACRO(ReadTimestamps) \
	EXPAND_MACRO(Submit) \
	EXPAND_MACRO(Present) \
	EXPAND_MACRO(BeginFrame) \
	EXPAND_MACRO(EndFrame) \
	EXPAND_MACRO(CleanupGraphicsDevice) \
	EXPAND_MACRO(CleanupGraphicsSurface) \
	EXPAND_MACRO(CleanupGraphicsDriver) \
	EXPAND_MACRO(FormatName) \
	EXPAND_MACRO(GetDeviceCaps) \
	EXPAND_MACRO(GetFrameSlot) \
	EXPAND_MACRO(GetHeapSize) \
	EXPAND_MACRO(ResetHeap) \
	EXPAND_MACRO(FindRenderPass) \
	EXPAND_MACRO(GetSwapchainSize) \
	EXPAND_MACRO(GetSwapchainFormat) \
	EXPAND_MACRO(GetSwapchainPreRotation) \
	EXPAND_MACRO(GetSwapchainImageCount) \
	EXPAND_MACRO(GetSwapchainImageIndex) \
	EXPAND_MACRO(GetSwapchainImage) \
	EXPAND_MACRO(GetBufferSize) \
	EXPAND_MACRO(GetImageSize) \
	EXPAND_MACRO(GetImageFormat) \
	EXPAND_MACRO(GetImageMipLevels) \
	EXPAND_MACRO(GetPipelineBindGroupLayout)

#define TOOLS_GFX_FUNCTION_POINTER(function_name) FN_ ## function_name * function_name;
#define TOOLS_GFX_FUNCTION_PROTOTYPE(function_name) FN_ ## function_name function_name;

////////////////////////////////////////////////////////////////////////
// API struct
////////////////////////////////////////////////////////////////////////

struct GraphicsAPI
{
	EXPAND_API_TOOLS_GFX(TOOLS_GFX_FUNCTION_POINTER);
};

////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////

#if defined(TOOLS_GFX_FUNCTION_PROTOTYPES)

EXPAND_API_TOOLS_GFX(TOOLS_GFX_FUNCTION_PROTOTYPE)

#define TOOLS_GFX_PROTOTYPE_TO_TABLE(name) api-> name = name;

inline void GetGraphicsAPI(GraphicsAPI *api)
{
	EXPAND_API_TOOLS_GFX(TOOLS_GFX_PROTOTYPE_TO_TABLE);
}

static void InvalidateSwapchain();

#endif

////////////////////////////////////////////////////////////////////////
// Function pointers
////////////////////////////////////////////////////////////////////////

#if defined(TOOLS_GFX_FUNCTION_POINTERS)

EXPAND_API_TOOLS_GFX(TOOLS_GFX_FUNCTION_POINTER)

#define TOOLS_GFX_TABLE_TO_POINTER(name) name = api->name;

inline void SetGraphicsAPI(GraphicsAPI *api)
{
	EXPAND_API_TOOLS_GFX(TOOLS_GFX_TABLE_TO_POINTER);
}

#endif

static constexpr float4 DebugGfxBlockDefaultColor = { 0.0f, 0.0f, 0.0f, 0.0f };

struct DebugGfxBlock
{
	const CommandList cmd;
	DebugGfxBlock(const CommandList &commandList, const char *name) : cmd(commandList) { BeginDebugGroup(commandList, name, DebugGfxBlockDefaultColor); }
	~DebugGfxBlock() { EndDebugGroup(cmd); }
};

#define DEBUG_GFX_BLOCK(cmd, name) DebugGfxBlock debugGfxBlock_##name(cmd, #name)

#endif // ILU_GFX_H


