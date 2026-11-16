#ifndef TOOLS_GFX_IMPLEMENTATION_INCLUDED
#define TOOLS_GFX_IMPLEMENTATION_INCLUDED


////////////////////////////////////////////////////////////////////////
// Vulkan headers and definitions
////////////////////////////////////////////////////////////////////////

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan_core.h>

#include "libs/offset_allocator/offsetAllocator.hpp"

#if USE_XCB
#	define VK_USE_PLATFORM_XCB_KHR 1
#elif USE_ANDROID
#	define VK_USE_PLATFORM_ANDROID_KHR 1
#elif USE_WINAPI
#	define VK_USE_PLATFORM_WIN32_KHR 1
#elif USE_COCOA
#	define VK_USE_PLATFORM_METAL_EXT 1
#else
#	error "Configure the proper platform for Vulkan"
#endif

#if VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan_win32.h>
#elif VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan_xcb.h>
#elif VK_USE_PLATFORM_ANDROID_KHR
#include <vulkan/vulkan_android.h>
#elif VK_USE_PLATFORM_METAL_EXT
#include <vulkan/vulkan_metal.h>
#endif

#if PLATFORM_APPLE
#define VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME "VK_KHR_portability_subset" // Provisional, in vulkan_beta.h
#endif

#define VULKAN_ALLOCATORS NULL

#define VK_CALL( call ) CheckVulkanResult( call, #call )


////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////

#define MAX_BIND_GROUPS SPV_MAX_DESCRIPTOR_SETS
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
#define FIRST_SWAPCHAIN_IMAGE_INDEX MAX_IMAGES

constexpr u32 HeapSize_General = MB(64);


////////////////////////////////////////////////////////////////////////
// Types
////////////////////////////////////////////////////////////////////////

constexpr const char *sCountTypeName[] = {
	"uniformBufferCount",
	"storageBufferCount",
	"storageTexelBufferCount",
	"textureCount",
	"samplerCount",
	"combinedImageSamplerCount",
	"groupCount",
};

struct Heap
{
	HeapType type;
	u32 size;
	u32 memoryTypeIndex;
	VkDeviceMemory memory;
	u8* data;
	u32 used;
};

struct Alloc
{
	HeapType heap;
	u64 offset;
	u64 size;
	OffsetAllocator::Allocation allocation;
};

struct BindGroupAllocator
{
	const char *name;
	BindGroupAllocatorCounts maxCounts;
	BindGroupAllocatorCounts usedCounts;
	VkDescriptorPool handle;
};

struct BindGroupLayout
{
	VkDescriptorSetLayout handle;
	u32 crc;
	ShaderBinding bindings[MAX_SHADER_BINDINGS];
	u8 bindingCount;
};

struct BindGroup
{
	VkDescriptorSet handle; // Null if released with its pool
	VkDescriptorPool descriptorPool;
	BindGroupLayoutH layout;
	BindGroupAllocatorH allocator;
};

struct PipelineLayout
{
	VkPipelineLayout handle;
	BindGroupLayoutH bindGroupLayouts[MAX_BIND_GROUPS];
};

struct Pipeline
{
	const char *name;
	VkPipeline handle;
	PipelineLayout layout;
	VkPipelineBindPoint bindPoint;
};

struct Buffer
{
	VkBuffer handle;
	Alloc alloc;
	u32 size;
};

struct BufferView
{
	VkBufferView handle;
};

struct Image
{
	VkImage handle;
	VkImageView imageViewHandle;
	Format format;
	u32 width;
	u32 height;
	u32 mipLevels;
	Alloc alloc;
};

struct RenderPass
{
	const char *name;
	VkRenderPass handle;
};

struct Framebuffer
{
	VkFramebuffer handle;
	VkRenderPass renderPassHandle;
	VkExtent2D extent;
	u32 attachmentCount;
};

struct Sampler
{
	VkSampler handle;
};

struct SwapchainInfo
{
	Format format;
	VkFormat vkFormat;
	VkColorSpaceKHR colorSpace;
	VkPresentModeKHR presentMode;
};

struct Swapchain
{
	VkSwapchainKHR handle;
	VkExtent2D extent;
	u32 imageCount;
	ImageH imageHandles[MAX_SWAPCHAIN_IMAGE_COUNT];
	Image images[MAX_SWAPCHAIN_IMAGE_COUNT];
	float preRotationDegrees;
	u32 currentImageIndex;
	bool imageAcquired; // Image at currentImageIndex was acquired and not presented yet
	bool valid;
};

struct ShaderModule
{
	VkShaderModule handle;
};

struct TimestampPool
{
	VkQueryPool handle;
	u32 queryCount;
	u32 maxQueries;
};

struct CommandListState
{
	bool inUse;
	VkCommandBuffer handle;
	bool transient;

	VkDescriptorSet descriptorSetHandles[MAX_BIND_GROUPS];
	u8 descriptorSetDirtyMask;

	PipelineH pipeline;
	VkPipelineBindPoint bindPoint;

	// State
	union
	{
		struct // For gfx pipelines
		{
			VkClearValue clearValues[MAX_RENDER_TARGETS];
			VkBuffer vertexBufferHandle;
			VkBuffer indexBufferHandle;
		};
	};
};

struct GraphicsDevice
{
	VkInstance instance;

	VkPhysicalDevice physicalDevice;
	VkDevice handle;

	DeviceCaps caps;

	u32 graphicsQueueFamilyIndex;
	u32 presentQueueFamilyIndex;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkCommandPool commandPools[MAX_FRAMES_IN_FLIGHT];
	VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT];
	VkCommandPool transientCommandPool;

	VkSurfaceKHR surface;
	SwapchainInfo swapchainInfo;
	Swapchain swapchain;

	VkFence imageAcquiredFences[MAX_SWAPCHAIN_IMAGE_COUNT];
	VkSemaphore imageAcquiredSemaphores[MAX_SWAPCHAIN_IMAGE_COUNT];
	VkSemaphore renderFinishedSemaphores[MAX_SWAPCHAIN_IMAGE_COUNT];
	u32 presentationIndex;

	VkSemaphore submitSignalSemaphore; // Waited by Present

	VkFence fences[MAX_FENCES];
	u32 firstFenceIndex;
	u32 usedFenceCount;

	struct FrameData
	{
		u32 firstFenceIndex;
		u32 usedFenceCount;
	};

	FrameData frameData[MAX_FRAMES_IN_FLIGHT];

	VkPipelineCache pipelineCache;

	Heap heaps[HeapType_COUNT];

	u32 frameSlot;

	struct
	{
		bool debugUtils;
	} support;
	struct
	{
		float timestampPeriod;
	} limits;

	VkDebugUtilsMessengerEXT debugUtilsMessenger;

	// Resources, indexed through their handle pools

	BindGroupLayout bindGroupLayouts[MAX_BIND_GROUP_LAYOUTS];
	HandlePool bindGroupLayoutPool;

	BindGroupAllocator bindGroupAllocators[MAX_BIND_GROUP_ALLOCATORS];
	HandlePool bindGroupAllocatorPool;

	BindGroup bindGroups[MAX_ALLOCATED_BIND_GROUPS];
	HandlePool bindGroupPool;

	Buffer buffers[MAX_BUFFERS];
	HandlePool bufferPool;

	BufferView bufferViews[MAX_BUFFERS];
	HandlePool bufferViewPool;

	Image images[MAX_IMAGES];
	HandlePool imagePool;

	Sampler samplers[MAX_SAMPLERS];
	HandlePool samplerPool;

	Pipeline pipelines[MAX_PIPELINES];
	HandlePool pipelinePool;

	RenderPass renderPasses[MAX_RENDERPASSES];
	HandlePool renderPassPool;

	Framebuffer framebuffers[MAX_FRAMEBUFFERS];
	HandlePool framebufferPool;

	TimestampPool timestampPools[MAX_TIMESTAMP_POOLS];
	HandlePool timestampPoolPool;

	byte poolMemory[KB(128)]; // For the pools above

	CommandListState commandLists[MAX_COMMAND_LISTS]; // Index 0 unused
};

inline bool IsValid(const Pipeline &pipeline)
{
	bool res = pipeline.handle != 0;
	return res;
}

inline bool IsValid(const Image &image)
{
	bool res = image.handle != 0;
	return res;
}




////////////////////////////////////////////////////////////////////////
// Internal functions
////////////////////////////////////////////////////////////////////////

Buffer &GetBuffer(GraphicsDevice &device, BufferH bufferHandle);
const Buffer &GetBufferConst(const GraphicsDevice &device, BufferH bufferHandle);
void DestroyBuffer(const GraphicsDevice &device, const Buffer &buffer);
const BufferView &GetBufferView(const GraphicsDevice &device, BufferViewH bufferViewH);
void DestroyBufferView(const GraphicsDevice &device, const BufferView &bufferView);
Image &GetImage(GraphicsDevice &device, ImageH imageH);
const Image &GetImageConst(const GraphicsDevice &device, ImageH imageH);
void DestroyImage(const GraphicsDevice &device, const Image &image);
const Sampler &GetSampler(const GraphicsDevice &device, SamplerH handle);
void DestroySampler(const GraphicsDevice &device, const Sampler &sampler);
const Pipeline &GetPipeline(const GraphicsDevice &device, PipelineH handle);
void DestroyPipeline(const GraphicsDevice &device, const Pipeline &pipeline);
const RenderPass &GetRenderPass(const GraphicsDevice &device, RenderPassH handle);
void DestroyRenderPass(const GraphicsDevice &device, const RenderPass &renderPass);
static const BindGroupLayout &GetBindGroupLayout(const GraphicsDevice &device, BindGroupLayoutH handle);
bool IsSwapchainImage(ImageH image);
static ImageH SwapchainImageHandle(u32 swapchainImageIndex);

// Element index of a handle, -1 for a zero handle. Asserts on stale handles.
static i32 GetElementIndex(const HandlePool &pool, Handle handle)
{
	if ( handle.gen == 0 ) { return -1; }
	const bool valid = IsValidHandle(pool, handle);
	ASSERT( valid && "Handle of a destroyed graphics resource" );
	return valid ? GetHandleIndex(pool, handle) : -1;
}


////////////////////////////////////////////////////////////////////////
// Vulkan function pointers
////////////////////////////////////////////////////////////////////////

// Vulkan 1.0 pre-instance functions
#define EXPAND_API_VK_PRE_INSTANCE(EXPAND_MACRO) \
	EXPAND_MACRO(vkCreateInstance) \
	EXPAND_MACRO(vkEnumerateInstanceExtensionProperties) \
	EXPAND_MACRO(vkEnumerateInstanceLayerProperties)

// Vulkan 1.0 instance functions
#define EXPAND_API_VK10_INSTANCE(EXPAND_MACRO) \
	EXPAND_MACRO(vkCreateDevice) \
	EXPAND_MACRO(vkDestroyInstance) \
	EXPAND_MACRO(vkEnumerateDeviceExtensionProperties) \
	EXPAND_MACRO(vkEnumeratePhysicalDevices) \
	EXPAND_MACRO(vkGetDeviceProcAddr) \
	EXPAND_MACRO(vkGetPhysicalDeviceFeatures) \
	EXPAND_MACRO(vkGetPhysicalDeviceFormatProperties) \
	EXPAND_MACRO(vkGetPhysicalDeviceMemoryProperties) \
	EXPAND_MACRO(vkGetPhysicalDeviceProperties) \
	EXPAND_MACRO(vkGetPhysicalDeviceQueueFamilyProperties)

// Debug utils instance functions
#define EXPAND_API_VK_DEBUG_UTILS(EXPAND_MACRO) \
	EXPAND_MACRO(vkCmdBeginDebugUtilsLabelEXT) \
	EXPAND_MACRO(vkCmdEndDebugUtilsLabelEXT) \
	EXPAND_MACRO(vkCmdInsertDebugUtilsLabelEXT) \
	EXPAND_MACRO(vkCreateDebugUtilsMessengerEXT) \
	EXPAND_MACRO(vkDestroyDebugUtilsMessengerEXT) \
	EXPAND_MACRO(vkSetDebugUtilsObjectNameEXT)

// Surface instance functions
#define EXPAND_API_VK_KHR_SURFACE(EXPAND_MACRO) \
	EXPAND_MACRO(vkDestroySurfaceKHR) \
	EXPAND_MACRO(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
	EXPAND_MACRO(vkGetPhysicalDeviceSurfaceFormatsKHR) \
	EXPAND_MACRO(vkGetPhysicalDeviceSurfacePresentModesKHR) \
	EXPAND_MACRO(vkGetPhysicalDeviceSurfaceSupportKHR)

#if defined(VK_KHR_win32_surface)
#define VK_INSTANCE_FUNCTION_LIST_KHR_SURFACE_PLATFORM(EXPAND_MACRO) EXPAND_MACRO(vkCreateWin32SurfaceKHR)
#endif /* defined(VK_KHR_win32_surface) */
#if defined(VK_KHR_xcb_surface)
#define VK_INSTANCE_FUNCTION_LIST_KHR_SURFACE_PLATFORM(EXPAND_MACRO) EXPAND_MACRO(vkCreateXcbSurfaceKHR)
#endif /* defined(VK_KHR_xcb_surface) */
#if defined(VK_KHR_android_surface)
#define VK_INSTANCE_FUNCTION_LIST_KHR_SURFACE_PLATFORM(EXPAND_MACRO) EXPAND_MACRO(vkCreateAndroidSurfaceKHR)
#endif /* defined(VK_KHR_android_surface) */
#if defined(VK_EXT_metal_surface)
#define VK_INSTANCE_FUNCTION_LIST_KHR_SURFACE_PLATFORM(EXPAND_MACRO) EXPAND_MACRO(vkCreateMetalSurfaceEXT)
#endif /* defined(VK_EXT_metal_surface) */

// All instance functions
#define EXPAND_API_VK_INSTANCE(EXPAND_MACRO) \
	EXPAND_API_VK10_INSTANCE(EXPAND_MACRO) \
	EXPAND_API_VK_DEBUG_UTILS(EXPAND_MACRO) \
	EXPAND_API_VK_KHR_SURFACE(EXPAND_MACRO) \
	VK_INSTANCE_FUNCTION_LIST_KHR_SURFACE_PLATFORM(EXPAND_MACRO)

// Device functions (Vulkan 1.0)
#define EXPAND_API_VK10_DEVICE(EXPAND_MACRO) \
	EXPAND_MACRO(vkAllocateCommandBuffers) \
	EXPAND_MACRO(vkAllocateDescriptorSets) \
	EXPAND_MACRO(vkFreeDescriptorSets) \
	EXPAND_MACRO(vkAllocateMemory) \
	EXPAND_MACRO(vkBeginCommandBuffer) \
	EXPAND_MACRO(vkBindBufferMemory) \
	EXPAND_MACRO(vkBindImageMemory) \
	EXPAND_MACRO(vkCmdBeginRenderPass) \
	EXPAND_MACRO(vkCmdBindDescriptorSets) \
	EXPAND_MACRO(vkCmdBindIndexBuffer) \
	EXPAND_MACRO(vkCmdBindPipeline) \
	EXPAND_MACRO(vkCmdBindVertexBuffers) \
	EXPAND_MACRO(vkCmdBlitImage) \
	EXPAND_MACRO(vkCmdCopyBuffer) \
	EXPAND_MACRO(vkCmdCopyBufferToImage) \
	EXPAND_MACRO(vkCmdDispatch) \
	EXPAND_MACRO(vkCmdDraw) \
	EXPAND_MACRO(vkCmdDrawIndexed) \
	EXPAND_MACRO(vkCmdEndRenderPass) \
	EXPAND_MACRO(vkCmdPipelineBarrier) \
	EXPAND_MACRO(vkCmdResetQueryPool) \
	EXPAND_MACRO(vkCmdSetScissor) \
	EXPAND_MACRO(vkCmdSetViewport) \
	EXPAND_MACRO(vkCmdWriteTimestamp) \
	EXPAND_MACRO(vkCreateBuffer) \
	EXPAND_MACRO(vkCreateBufferView) \
	EXPAND_MACRO(vkCreateCommandPool) \
	EXPAND_MACRO(vkCreateComputePipelines) \
	EXPAND_MACRO(vkCreateDescriptorPool) \
	EXPAND_MACRO(vkCreateDescriptorSetLayout) \
	EXPAND_MACRO(vkCreateFence) \
	EXPAND_MACRO(vkCreateFramebuffer) \
	EXPAND_MACRO(vkCreateGraphicsPipelines) \
	EXPAND_MACRO(vkCreateImage) \
	EXPAND_MACRO(vkCreateImageView) \
	EXPAND_MACRO(vkCreatePipelineCache) \
	EXPAND_MACRO(vkCreatePipelineLayout) \
	EXPAND_MACRO(vkCreateQueryPool) \
	EXPAND_MACRO(vkCreateRenderPass) \
	EXPAND_MACRO(vkCreateSampler) \
	EXPAND_MACRO(vkCreateSemaphore) \
	EXPAND_MACRO(vkCreateShaderModule) \
	EXPAND_MACRO(vkDestroyBuffer) \
	EXPAND_MACRO(vkDestroyBufferView) \
	EXPAND_MACRO(vkDestroyCommandPool) \
	EXPAND_MACRO(vkDestroyDescriptorPool) \
	EXPAND_MACRO(vkDestroyDescriptorSetLayout) \
	EXPAND_MACRO(vkDestroyDevice) \
	EXPAND_MACRO(vkDestroyFence) \
	EXPAND_MACRO(vkDestroyFramebuffer) \
	EXPAND_MACRO(vkDestroyImage) \
	EXPAND_MACRO(vkDestroyImageView) \
	EXPAND_MACRO(vkDestroyPipeline) \
	EXPAND_MACRO(vkDestroyPipelineCache) \
	EXPAND_MACRO(vkDestroyPipelineLayout) \
	EXPAND_MACRO(vkDestroyQueryPool) \
	EXPAND_MACRO(vkDestroyRenderPass) \
	EXPAND_MACRO(vkDestroySampler) \
	EXPAND_MACRO(vkDestroySemaphore) \
	EXPAND_MACRO(vkDestroyShaderModule) \
	EXPAND_MACRO(vkDeviceWaitIdle) \
	EXPAND_MACRO(vkEndCommandBuffer) \
	EXPAND_MACRO(vkFreeCommandBuffers) \
	EXPAND_MACRO(vkFreeMemory) \
	EXPAND_MACRO(vkGetBufferMemoryRequirements) \
	EXPAND_MACRO(vkGetDeviceQueue) \
	EXPAND_MACRO(vkGetImageMemoryRequirements) \
	EXPAND_MACRO(vkGetPipelineCacheData) \
	EXPAND_MACRO(vkGetQueryPoolResults) \
	EXPAND_MACRO(vkMapMemory) \
	EXPAND_MACRO(vkQueueSubmit) \
	EXPAND_MACRO(vkQueueWaitIdle) \
	EXPAND_MACRO(vkResetCommandPool) \
	EXPAND_MACRO(vkResetDescriptorPool) \
	EXPAND_MACRO(vkResetFences) \
	EXPAND_MACRO(vkUpdateDescriptorSets) \
	EXPAND_MACRO(vkWaitForFences) \
	EXPAND_MACRO(vkAcquireNextImageKHR) \
	EXPAND_MACRO(vkCreateSwapchainKHR) \
	EXPAND_MACRO(vkDestroySwapchainKHR) \
	EXPAND_MACRO(vkGetSwapchainImagesKHR) \
	EXPAND_MACRO(vkQueuePresentKHR)

PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;

// Expand device function prototypes
#define EXPAND_VK_PFN(function_name) PFN_##function_name function_name;
EXPAND_API_VK_PRE_INSTANCE(EXPAND_VK_PFN)
EXPAND_API_VK_INSTANCE(EXPAND_VK_PFN)
EXPAND_API_VK10_DEVICE(EXPAND_VK_PFN)

static bool VulkanLoadInitFunctions()
{
	const char *libraryFilenames[] = {
#if defined(PLATFORM_WINDOWS)
		"vulkan-1.dll",
#elif defined(PLATFORM_APPLE)
		"libvulkan.1.dylib",
		"libvulkan.dylib",
		"/usr/local/lib/libvulkan.1.dylib",
		"/opt/homebrew/lib/libvulkan.1.dylib",
#else
		"libvulkan.so.1",
		"libvulkan.so",
#endif
	};

	DynamicLibrary vulkan = {};
	for (u32 i = 0; !vulkan && i < ARRAY_COUNT(libraryFilenames); ++i)
	{
		vulkan = OpenLibrary(libraryFilenames[i]);
	}

	if (!vulkan)
	{
		return false;
	}

	vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)LoadSymbol(vulkan, "vkGetInstanceProcAddr");

	// Get all init function pointers
	#define EXPAND_VK_GET_INSTANCE_PROC_ADDR_INIT(function_name) function_name = (PFN_##function_name)vkGetInstanceProcAddr(NULL, #function_name);
	EXPAND_API_VK_PRE_INSTANCE(EXPAND_VK_GET_INSTANCE_PROC_ADDR_INIT)

	return true;
}

static void VulkanLoadInstanceFunctions(VkInstance instance)
{
	// Get all instance function pointers
	#define EXPAND_VK_GET_INSTANCE_PROC_ADDR(function_name) function_name = (PFN_##function_name)vkGetInstanceProcAddr(instance, #function_name);
	EXPAND_API_VK_INSTANCE(EXPAND_VK_GET_INSTANCE_PROC_ADDR)
}

static void VulkanLoadDeviceFunctions(VkDevice device)
{
	// Get all device function pointers
	#define EXPAND_VK_GET_DEVICE_PROC_ADDR(function_name) function_name = (PFN_##function_name)vkGetDeviceProcAddr(device, #function_name);
	EXPAND_API_VK10_DEVICE(EXPAND_VK_GET_DEVICE_PROC_ADDR)
}


////////////////////////////////////////////////////////////////////////
// Offset allocator
////////////////////////////////////////////////////////////////////////

static OffsetAllocator::Allocator generalAllocator(HeapSize_General);


////////////////////////////////////////////////////////////////////////
// String interning
////////////////////////////////////////////////////////////////////////

static StringInterning *gStringInterningGfx = nullptr;

void SetGraphicsStringInterning(StringInterning *stringInterning)
{
	ASSERT(gStringInterningGfx == nullptr);
	gStringInterningGfx = stringInterning;
}

#define InternStringGfx(str) MakeStringIntern(gStringInterningGfx, str)


////////////////////////////////////////////////////////////////////////
// Helper functions to map Vulkan types
////////////////////////////////////////////////////////////////////////

static const char *VkPhysicalDeviceTypeToString( VkPhysicalDeviceType type )
{
	static const char *toString[] = {
		"VK_PHYSICAL_DEVICE_TYPE_OTHER", // 0
		"VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU", // 1
		"VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU", // 2
		"VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU", // 3
		"VK_PHYSICAL_DEVICE_TYPE_CPU", // 4
	};
	ASSERT( type < ARRAY_COUNT(toString) );
	return toString[type];
}

#define CONCAT_FLAG(flag) if ( flags & flag ) StrCat(outString, #flag "|" );

static const char *VkMemoryPropertyFlagsToString( VkMemoryPropertyFlags flags, char *outString )
{
	*outString = 0;
	CONCAT_FLAG(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	CONCAT_FLAG(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	CONCAT_FLAG(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	CONCAT_FLAG(VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
	CONCAT_FLAG(VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
	CONCAT_FLAG(VK_MEMORY_PROPERTY_PROTECTED_BIT);
	CONCAT_FLAG(VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD);
	CONCAT_FLAG(VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD);
	CONCAT_FLAG(VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV);
	return outString;
}

static const char *VkMemoryHeapFlagsToString( VkMemoryHeapFlags flags, char *outString )
{
	*outString = 0;
	CONCAT_FLAG(VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);
	CONCAT_FLAG(VK_MEMORY_HEAP_MULTI_INSTANCE_BIT);
	return outString;
}

#undef CONCAT_FLAG

static const char *HeapTypeToString(HeapType heapType)
{
	static const char *toString[] = {
		"HeapType_General",
		"HeapType_RTs",
		"HeapType_Staging",
		"HeapType_Dynamic",
		"HeapType_Readback",
	};
	CT_ASSERT( ARRAY_COUNT(toString) == HeapType_COUNT );
	ASSERT( heapType < ARRAY_COUNT(toString) );
	return toString[heapType];
}

static VkMemoryPropertyFlags HeapTypeToVkMemoryPropertyFlags( HeapType heapType )
{
	static const VkMemoryPropertyFlags flags[] = {
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // HeapType_General,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // HeapType_RTs,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // HeapType_Staging,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // HeapType_Dynamic,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT, // HeapType_Readback,
	};
	CT_ASSERT( ARRAY_COUNT(flags) == HeapType_COUNT );
	return flags[heapType];
};

static VkDescriptorType SpvDescriptorTypeToVulkan(SpvType type)
{
	static const VkDescriptorType vkDescriptorTypes[] = {
		VK_DESCRIPTOR_TYPE_MAX_ENUM,
		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		VK_DESCRIPTOR_TYPE_SAMPLER,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // Should be DYNAMIC if we use dynamic offsets
		VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
	};
	CT_ASSERT(ARRAY_COUNT(vkDescriptorTypes) == SpvTypeCount);
	ASSERT(type != SpvTypeNone);
	ASSERT(type < SpvTypeCount);
	const VkDescriptorType vkDescriptorType = vkDescriptorTypes[type];
	return vkDescriptorType;
}

static VkShaderStageFlags SpvStageFlagsToVulkan(u8 stageFlags)
{
	VkShaderStageFlags vkStageFlags = 0;
	vkStageFlags |= ( stageFlags & SpvStageFlagsVertexBit ) ? VK_SHADER_STAGE_VERTEX_BIT : 0;
	vkStageFlags |= ( stageFlags & SpvStageFlagsFragmentBit ) ? VK_SHADER_STAGE_FRAGMENT_BIT : 0;
	vkStageFlags |= ( stageFlags & SpvStageFlagsComputeBit ) ? VK_SHADER_STAGE_COMPUTE_BIT : 0;
	return vkStageFlags;
}

static VkFormat FormatToVulkan(Format format)
{
	ASSERT(format < FormatCount);
	static VkFormat vkFormats[] = {
		VK_FORMAT_R32_UINT,
		VK_FORMAT_R32_SFLOAT,
		VK_FORMAT_R32G32_SFLOAT,
		VK_FORMAT_R32G32B32_SFLOAT,
		VK_FORMAT_R8_UNORM,
		VK_FORMAT_R8_SRGB,
		VK_FORMAT_R8G8_UNORM,
		VK_FORMAT_R8G8_SRGB,
		VK_FORMAT_R8G8B8_UNORM,
		VK_FORMAT_R8G8B8_SRGB,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_B8G8R8A8_SRGB,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_R16G16_SFLOAT,
		VK_FORMAT_B10G11R11_UFLOAT_PACK32,
		VK_FORMAT_R16G16B16A16_SFLOAT,
	};
	CT_ASSERT(ARRAY_COUNT(vkFormats) == FormatCount);
	const VkFormat vkFormat = format < FormatCount ? vkFormats[format] : VK_FORMAT_MAX_ENUM;
	return vkFormat;
}

static Format FormatFromVulkan(VkFormat format)
{
	switch (format) {
		case VK_FORMAT_R32_UINT: return FormatUInt;
		case VK_FORMAT_R32_SFLOAT: return FormatFloat;
		case VK_FORMAT_R32G32_SFLOAT: return FormatFloat2;
		case VK_FORMAT_R32G32B32_SFLOAT: return FormatFloat3;
		case VK_FORMAT_R8_UNORM: return FormatR8;
		case VK_FORMAT_R8_SRGB: return FormatR8_SRGB;
		case VK_FORMAT_R8G8_UNORM: return FormatRG8;
		case VK_FORMAT_R8G8_SRGB: return FormatRG8_SRGB;
		case VK_FORMAT_R8G8B8_UNORM: return FormatRGB8;
		case VK_FORMAT_R8G8B8_SRGB: return FormatRGB8_SRGB;
		case VK_FORMAT_R8G8B8A8_UNORM: return FormatRGBA8;
		case VK_FORMAT_R8G8B8A8_SRGB: return FormatRGBA8_SRGB;
		case VK_FORMAT_B8G8R8A8_UNORM: return FormatBGRA8;
		case VK_FORMAT_B8G8R8A8_SRGB: return FormatBGRA8_SRGB;
		case VK_FORMAT_D32_SFLOAT: return FormatD32;
		case VK_FORMAT_D32_SFLOAT_S8_UINT: return FormatD32S1;
		case VK_FORMAT_D24_UNORM_S8_UINT: return FormatD24S1;
		case VK_FORMAT_R16G16_SFLOAT: return FormatRG16F;
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32: return FormatRG11B10F;
		case VK_FORMAT_R16G16B16A16_SFLOAT: return FormatRGBA16F;
		default:;
	};
	return FormatInvalid;
}

const char *FormatName(Format format)
{
	static const char *names[] = {
		"VK_FORMAT_R32_UINT",
		"VK_FORMAT_R32_SFLOAT",
		"VK_FORMAT_R32G32_SFLOAT",
		"VK_FORMAT_R32G32B32_SFLOAT",
		"VK_FORMAT_R8_UNORM",
		"VK_FORMAT_R8_SRGB",
		"VK_FORMAT_R8G8_UNORM",
		"VK_FORMAT_R8G8_SRGB",
		"VK_FORMAT_R8G8B8_UNORM",
		"VK_FORMAT_R8G8B8_SRGB",
		"VK_FORMAT_R8G8B8A8_UNORM",
		"VK_FORMAT_R8G8B8A8_SRGB",
		"VK_FORMAT_B8G8R8A8_UNORM",
		"VK_FORMAT_B8G8R8A8_SRGB",
		"VK_FORMAT_D32_SFLOAT",
		"VK_FORMAT_D32_SFLOAT_S8_UINT",
		"VK_FORMAT_D24_UNORM_S8_UINT",
		"VK_FORMAT_R16G16_SFLOAT",
		"VK_FORMAT_B10G11R11_UFLOAT_PACK32",
		"VK_FORMAT_R16G16B16A16_SFLOAT",
	};
	CT_ASSERT(ARRAY_COUNT(names) == FormatCount);
	const char *name = format < FormatCount ? names[format] : "VK_FORMAT_INVALID";
	return name;
}

static VkBorderColor BorderColorToVulkan(BorderColor color)
{
	static const VkBorderColor vkBorderColors[] = {
		VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		VK_BORDER_COLOR_INT_OPAQUE_WHITE,
		VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
		VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
	};
	CT_ASSERT(ARRAY_COUNT(vkBorderColors) == BorderColorCount);
	ASSERT(color < BorderColorCount);
	const VkBorderColor vkBorderColor = vkBorderColors[color];
	return vkBorderColor;
};

static VkSamplerAddressMode AddressModeToVulkan(AddressMode mode)
{
	static const VkSamplerAddressMode vkAddressModes[] = {
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	};
	CT_ASSERT(ARRAY_COUNT(vkAddressModes) == AddressModeCount);
	ASSERT(mode < AddressModeCount);
	const VkSamplerAddressMode vkAddressMode = vkAddressModes[mode];
	return vkAddressMode;
}

static VkFilter FilterToVulkan(Filter filter)
{
	static const VkFilter vkFilters[] = {
		VK_FILTER_NEAREST,
		VK_FILTER_LINEAR,
	};
	CT_ASSERT(ARRAY_COUNT(vkFilters) == FilterCount);
	ASSERT(filter < FilterCount);
	const VkFilter vkFilter = vkFilters[filter];
	return vkFilter;
}

static VkCompareOp CompareOpToVulkan(CompareOp compareOp)
{
	static const VkCompareOp vkCompareOps[] = {
		VK_COMPARE_OP_NEVER,
		VK_COMPARE_OP_LESS,
		VK_COMPARE_OP_LESS_OR_EQUAL,
		VK_COMPARE_OP_GREATER,
		VK_COMPARE_OP_GREATER_OR_EQUAL,
		VK_COMPARE_OP_EQUAL,
	};
	CT_ASSERT(ARRAY_COUNT(vkCompareOps) == CompareOpCount);
	ASSERT(compareOp < CompareOpCount);
	const VkCompareOp vkCompareOp = vkCompareOps[compareOp];
	return vkCompareOp;
}

static VkBufferUsageFlags BufferUsageFlagsToVulkan(BufferUsageFlags flags)
{
	VkBufferUsageFlags vkFlags = 0;
	vkFlags |= ( flags & BufferUsageTransferSrc ) ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : 0;
	vkFlags |= ( flags & BufferUsageTransferDst ) ? VK_BUFFER_USAGE_TRANSFER_DST_BIT : 0;
	vkFlags |= ( flags & BufferUsageUniformBuffer ) ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : 0;
	vkFlags |= ( flags & BufferUsageUniformTexelBuffer ) ? VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT : 0;
	vkFlags |= ( flags & BufferUsageStorageBuffer ) ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0;
	vkFlags |= ( flags & BufferUsageStorageTexelBuffer ) ? VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT : 0;
	vkFlags |= ( flags & BufferUsageVertexBuffer ) ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : 0;
	vkFlags |= ( flags & BufferUsageIndexBuffer ) ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT : 0;
	return vkFlags;
}

static VkImageUsageFlags ImageUsageFlagsToVulkan(ImageUsageFlags flags)
{
	VkImageUsageFlags vkFlags = 0;
	vkFlags |= ( flags & ImageUsageTransferSrc ) ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0;
	vkFlags |= ( flags & ImageUsageTransferDst ) ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;
	vkFlags |= ( flags & ImageUsageSampled ) ? VK_IMAGE_USAGE_SAMPLED_BIT : 0;
	vkFlags |= ( flags & ImageUsageStorage ) ? VK_IMAGE_USAGE_STORAGE_BIT : 0;
	vkFlags |= ( flags & ImageUsageColorAttachment ) ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 0;
	vkFlags |= ( flags & ImageUsageDepthStencilAttachment ) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : 0;
	vkFlags |= ( flags & ImageUsageTransient ) ? VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT : 0;
	vkFlags |= ( flags & ImageUsageInputAttachment ) ? VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT : 0;
	return vkFlags;
}

//static bool IsDepthFormat(VkFormat format)
//{
//	const bool isDepthFormat =
//		format == VK_FORMAT_D24_UNORM_S8_UINT ||
//		format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
//		format == VK_FORMAT_D32_SFLOAT ||
//		format == VK_FORMAT_D16_UNORM_S8_UINT ||
//		format == VK_FORMAT_D16_UNORM;
//	return isDepthFormat;
//}
static bool IsDepthFormat(Format format)
{
	const bool isDepthFormat =
		format == FormatD32S1 ||
		format == FormatD24S1 ||
		format == FormatD32;
	return isDepthFormat;
}

//static bool HasStencilComponent(VkFormat format)
//{
//	const bool hasStencil =
//		format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
//		format == VK_FORMAT_D24_UNORM_S8_UINT ||
//		format == VK_FORMAT_D16_UNORM_S8_UINT;
//	return hasStencil;
//}
static bool HasStencilComponent(Format format)
{
	const bool hasStencil =
		format == FormatD32S1 ||
		format == FormatD24S1;
	return hasStencil;
}

static VkAttachmentLoadOp LoadOpToVulkan( LoadOp loadOp )
{
	ASSERT(loadOp <= LoadOpDontCare);
	static VkAttachmentLoadOp vkLoadOps[] = {
		VK_ATTACHMENT_LOAD_OP_LOAD,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
	};
	const VkAttachmentLoadOp vkLoadOp = vkLoadOps[loadOp];
	return vkLoadOp;
}

static VkAttachmentStoreOp StoreOpToVulkan( StoreOp storeOp )
{
	ASSERT(storeOp <= StoreOpDontCare);
	static VkAttachmentStoreOp vkStoreOps[] = {
		VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
	};
	const VkAttachmentStoreOp vkStoreOp = vkStoreOps[storeOp];
	return vkStoreOp;
}

static VkPipelineStageFlagBits PipelineStageToVulkan( PipelineStage stage )
{
	ASSERT(stage <= PipelineStageBottom);
	static VkPipelineStageFlagBits vkStages[] = {
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
	};
	const VkPipelineStageFlagBits vkStageFlags = vkStages[stage];
	return vkStageFlags;
};



////////////////////////////////////////////////////////////////////////
// Internal functions
////////////////////////////////////////////////////////////////////////

#if DEVELOPMENT_BUILD
static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugUtilsMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT		message_severity,
    VkDebugUtilsMessageTypeFlagsEXT				message_type,
    const VkDebugUtilsMessengerCallbackDataEXT	*callback_data,
    void 										*user_data)
{
	if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		LOG(Warning, "%u - %s: %s\n", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
	}
	else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		LOG(Error, "%u - %s: %s\n", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
	}
	else
	{
		LOG(Debug, "%u - %s: %s\n", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
	}
	return VK_FALSE;
}
#endif // DEVELOPMENT_BUILD

static const char *VkResultToString(VkResult result)
{
	switch (result)
	{
		case VK_SUCCESS: return "VK_SUCCESS";
		case VK_NOT_READY: return "VK_NOT_READY";
		case VK_TIMEOUT: return "VK_TIMEOUT";
		case VK_EVENT_SET: return "VK_EVENT_SET";
		case VK_EVENT_RESET: return "VK_EVENT_RESET";
		case VK_INCOMPLETE: return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
		case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
		case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
		default: break;
	}
	return "VK_ERROR_UNHANDLED";
}

static void CheckVulkanResult(VkResult result, const char *callString)
{
	if (result == VK_SUCCESS)
		return;
	LOG(Error, "[vulkan] VkResult error:\n");
	LOG(Error, "[vulkan] - errorCode: %d\n", result);
	LOG(Error, "[vulkan] - errorString: %s\n", VkResultToString(result));
	LOG(Error, "[vulkan] - callString: %s\n", callString);
	if (result < VK_SUCCESS)
		QUIT_ABNORMALLY();
}

static Heap CreateHeap(const GraphicsDevice &device, HeapType heapType, u32 size, bool mapMemory)
{
	u32 memoryTypeIndex = -1;

	VkMemoryPropertyFlags requiredFlags = HeapTypeToVkMemoryPropertyFlags( heapType );

	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(device.physicalDevice, &memoryProperties);

	for ( u32 i = 0; i < memoryProperties.memoryTypeCount; ++i )
	{
		const VkMemoryType &memoryType = memoryProperties.memoryTypes[i];

		if ( ( memoryType.propertyFlags & requiredFlags ) == requiredFlags )
		{
			memoryTypeIndex = i;
			break;
		}
	}

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = size;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

	VkDeviceMemory memory;
	VK_CALL( vkAllocateMemory(device.handle, &memoryAllocateInfo, VULKAN_ALLOCATORS, &memory) );

	u8 *data = 0;
	if ( mapMemory )
	{
		VK_CALL( vkMapMemory(device.handle, memory, 0, size, 0, (void**)&data) );
	}

	Heap heap = {};
	heap.type = heapType;
	heap.size = size;
	heap.memoryTypeIndex = memoryTypeIndex;
	heap.memory = memory;
	heap.data = data;
	return heap;
}

static void DestroyHeap(const GraphicsDevice &device, const Heap &heap)
{
	vkFreeMemory(device.handle, heap.memory, VULKAN_ALLOCATORS);
}

static ShaderModule CreateShaderModule(const GraphicsDevice &device, const ShaderSource &source)
{
	const VkShaderModuleCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = source.dataSize,
		.pCode = reinterpret_cast<const u32*>(source.data),
	};

	VkShaderModule shaderModuleHandle;
	if ( vkCreateShaderModule(device.handle, &createInfo, VULKAN_ALLOCATORS, &shaderModuleHandle) != VK_SUCCESS )
	{
		LOG(Error, "Error in CreateShaderModule.\n");
		shaderModuleHandle = VK_NULL_HANDLE;
	}

	const ShaderModule shaderModule = {
		.handle = shaderModuleHandle,
	};
	return shaderModule;
}

static void DestroyShaderModule(const GraphicsDevice &device, const ShaderModule &shaderModule)
{
	vkDestroyShaderModule(device.handle, shaderModule.handle, VULKAN_ALLOCATORS);
}

static void UpdateDescriptorSets(const GraphicsDevice &device, VkWriteDescriptorSet *descriptorWrites, u32 descriptorWriteCount)
{
	if ( descriptorWriteCount > 0 )
	{
		vkUpdateDescriptorSets(device.handle, descriptorWriteCount, descriptorWrites, 0, NULL);
	}
}

union VkDescriptorGenericInfo
{
	VkDescriptorImageInfo imageInfo;
	VkDescriptorBufferInfo bufferInfo;
	VkBufferView bufferView;
};

static const ResourceBinding &GetResourceBinding(const ResourceBinding resourceBindings[MAX_SHADER_BINDINGS], u8 bindingIndex)
{
	ASSERT(bindingIndex < MAX_SHADER_BINDINGS);
	const ResourceBinding &resourceBinding = resourceBindings[bindingIndex];
	if ( resourceBinding.index == bindingIndex ) {
		return resourceBinding;
	}
	INVALID_CODE_PATH_MSG("Missing resource binding for BindGroupLayout");
	static ResourceBinding nullResourceBinding = {};
	return nullResourceBinding;
}

static bool AddDescriptorWrite(const GraphicsDevice &device, const ResourceBinding &resourceBinding, const ShaderBinding &binding, VkDescriptorSet descriptorSet, VkDescriptorGenericInfo *descriptorInfos, VkWriteDescriptorSet *descriptorWrites, u32 &descriptorWriteCount)
{
	VkDescriptorImageInfo *imageInfo = 0;
	VkDescriptorBufferInfo *bufferInfo = 0;
	VkBufferView *bufferViewPtr = 0;

	if ( binding.type == SpvTypeSampler )
	{
		const Sampler &sampler = GetSampler(device, resourceBinding.sampler);

		imageInfo = &descriptorInfos[descriptorWriteCount].imageInfo;
		imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo->imageView = VK_NULL_HANDLE;
		imageInfo->sampler = sampler.handle;
	}
	else if ( binding.type == SpvTypeImage )
	{
		const Image &image = GetImageConst(device, resourceBinding.image);

		imageInfo = &descriptorInfos[descriptorWriteCount].imageInfo;
		imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo->imageView = image.imageViewHandle;
		imageInfo->sampler = VK_NULL_HANDLE;
	}
	else if ( binding.type == SpvTypeUniformBuffer || binding.type == SpvTypeStorageBuffer )
	{
		const Buffer &buffer = GetBufferConst(device, resourceBinding.buffer);
		const u32 offset = resourceBinding.offset;
		const u32 range = resourceBinding.range;

		bufferInfo = &descriptorInfos[descriptorWriteCount].bufferInfo;
		bufferInfo->buffer = buffer.handle;
		bufferInfo->offset = offset;
		bufferInfo->range = range > 0 ? range : buffer.size;
	}
	else if ( binding.type == SpvTypeStorageTexelBuffer )
	{
		const BufferView &bufferView = GetBufferView(device, resourceBinding.bufferView);

		bufferViewPtr = &descriptorInfos[descriptorWriteCount].bufferView;
		*bufferViewPtr = bufferView.handle;
	}
	else
	{
		LOG(Warning, "Unhandled descriptor type (%u) for binding %s.\n", binding.type, binding.name);
		return false;
	}

	VkWriteDescriptorSet *descriptorWrite = &descriptorWrites[descriptorWriteCount++];
	descriptorWrite->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite->dstSet = descriptorSet;
	descriptorWrite->dstBinding = binding.binding;
	descriptorWrite->dstArrayElement = 0;
	descriptorWrite->descriptorType = SpvDescriptorTypeToVulkan( binding.type );
	descriptorWrite->descriptorCount = 1;
	descriptorWrite->pBufferInfo = bufferInfo;
	descriptorWrite->pImageInfo = imageInfo;
	descriptorWrite->pTexelBufferView = bufferViewPtr;
	return true;
}

static VkFormat FindSupportedFormat(const GraphicsDevice &device, const VkFormat candidates[], u32 candidateCount, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (u32 i = 0; i < candidateCount; ++i)
	{
		VkFormat format = candidates[i];
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(device.physicalDevice, format, &properties);

		if (tiling == VK_IMAGE_TILING_LINEAR && (features & properties.linearTilingFeatures) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (features & properties.optimalTilingFeatures) == features)
		{
			return format;
		}
	}

	INVALID_CODE_PATH();
	return VK_FORMAT_MAX_ENUM;
}

struct ShaderBindings
{
	ShaderBinding bindings[MAX_SHADER_BINDINGS];
	u32 bindGroupBindingBase[MAX_BIND_GROUPS];
	u32 bindGroupBindingCount[MAX_BIND_GROUPS];
};

static ShaderBindings ReflectShaderBindings( Arena scratch, byte* microcodeData[], const u64 microcodeSize[], u32 microcodeCount )
{
	void *tempMem = scratch.base + scratch.used;
	const u32 tempMemSize = scratch.size - scratch.used;

	SpvDescriptorSetList spvDescriptorList = {};
	for (u32 i = 0; i < microcodeCount; ++i)
	{
		SpvParser parser = SpvParserInit( microcodeData[i], microcodeSize[i] );
		SpvParseDescriptors( &parser, &spvDescriptorList, tempMem, tempMemSize );
	}

	ShaderBindings shaderBindings = {};
	u32 baseBindingCount = 0;

	for (u32 setIndex = 0; setIndex < SPV_MAX_DESCRIPTOR_SETS; ++setIndex)
	{
		u32 setBindingCount = 0;

		for (u32 bindingIndex = 0; bindingIndex < SPV_MAX_DESCRIPTORS_PER_SET; ++bindingIndex)
		{
			SpvDescriptor &descriptor = spvDescriptorList.sets[setIndex].bindings[bindingIndex];

			if ( descriptor.type != SpvTypeNone )
			{
				ASSERT( baseBindingCount + setBindingCount < ARRAY_COUNT(shaderBindings.bindings) );
				ShaderBinding &shaderBinding = shaderBindings.bindings[baseBindingCount + setBindingCount++];
				shaderBinding.binding = descriptor.binding;
				shaderBinding.set = setIndex;
				shaderBinding.type = (SpvType)descriptor.type;
				shaderBinding.stageFlags = descriptor.stageFlags;
				shaderBinding.name = InternStringGfx( descriptor.name );
				//LOG(Info, "Descriptor name: %s\n", descriptor.name);
			}
		}

		shaderBindings.bindGroupBindingBase[setIndex] = baseBindingCount;
		shaderBindings.bindGroupBindingCount[setIndex] = setBindingCount;
		baseBindingCount += setBindingCount;
	}

	return shaderBindings;
}

static ShaderBindings ReflectShaderBindings( Arena scratch, const ShaderSource &source )
{
	byte* microcodeData[] = { source.data };
	const u64 microcodeSize[] = { source.dataSize };
	const ShaderBindings shaderBindings = ReflectShaderBindings(scratch, microcodeData, microcodeSize, ARRAY_COUNT(microcodeData));
	return shaderBindings;
}

static ShaderBindings ReflectShaderBindings( Arena scratch, const ShaderSource &vertexSource, const ShaderSource &fragmentSource )
{
	byte* microcodeData[] = { vertexSource.data, fragmentSource.data };
	const u64 microcodeSize[] = { vertexSource.dataSize, fragmentSource.dataSize };
	ShaderBindings shaderBindings = ReflectShaderBindings(scratch, microcodeData, microcodeSize, ARRAY_COUNT(microcodeData));
	return shaderBindings;
}

static GraphicsDevice &GetDevice(const CommandList &commandList)
{
	ASSERT(commandList.device);
	return *commandList.device;
}

static CommandListState &GetState(const CommandList &commandList)
{
	ASSERT(commandList.device);
	ASSERT(commandList.index != 0 && commandList.index < MAX_COMMAND_LISTS);
	CommandListState &state = commandList.device->commandLists[commandList.index];
	ASSERT(state.inUse);
	return state;
}

static void BindDescriptorSets(const CommandList &commandList)
{
	CommandListState &state = GetState(commandList);

	while ( state.descriptorSetDirtyMask )
	{
		const u32 descriptorSetFirst = CTZ(state.descriptorSetDirtyMask);

		u32 descriptorSetCount = 0;
		VkDescriptorSet descriptorSets[MAX_BIND_GROUPS] = {};

		for (u32 i = descriptorSetFirst; i < MAX_BIND_GROUPS; ++i )
		{
			const VkDescriptorSet descriptorSet = state.descriptorSetHandles[i];

			if ( state.descriptorSetDirtyMask & (1 << i) )
			{
				state.descriptorSetDirtyMask &= ~(1 << i);

				if ( descriptorSet )
				{
					descriptorSets[descriptorSetCount++] = descriptorSet;
				}
			}
			else
			{
				break;
			}
		}

		if ( descriptorSetCount > 0 )
		{
			const Pipeline &pipeline = GetPipeline(GetDevice(commandList), state.pipeline);
			vkCmdBindDescriptorSets(state.handle, pipeline.bindPoint, pipeline.layout.handle, descriptorSetFirst, descriptorSetCount, descriptorSets, 0, NULL);
		}
	}
}

static VkImageAspectFlags FormatToVulkanAspect(Format format)
{
	VkImageAspectFlags aspectMask = 0;
	aspectMask |= IsDepthFormat(format) ? VK_IMAGE_ASPECT_DEPTH_BIT : 0;
	aspectMask |= HasStencilComponent(format) ? VK_IMAGE_ASPECT_STENCIL_BIT : 0;
	aspectMask = aspectMask ? aspectMask : VK_IMAGE_ASPECT_COLOR_BIT;
	return aspectMask;
}

static VkImageView CreateImageView(const GraphicsDevice &device, VkImage image, VkFormat format, VkImageAspectFlags aspect, u32 levelCount)
{
	const VkImageViewCreateInfo viewInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.components = {
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY,
		},
		.subresourceRange = {
			.aspectMask = aspect,
			.baseMipLevel = 0,
			.levelCount = levelCount,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};

	VkImageView imageView;
	VK_CALL( vkCreateImageView(device.handle, &viewInfo, VULKAN_ALLOCATORS, &imageView) );
	return imageView;
}

static void DestroyImageView(const GraphicsDevice &device, const VkImageView &imageView)
{
	vkDestroyImageView(device.handle, imageView, VULKAN_ALLOCATORS);
}

static const char *sPipelineCachePath = "build/pipeline_cache.bin";

static void SavePipelineCacheData(const GraphicsDevice &device, Arena scratch)
{
	size_t pipelineCacheSize = 0;
	if ( vkGetPipelineCacheData( device.handle, device.pipelineCache, &pipelineCacheSize, NULL ) == VK_SUCCESS && pipelineCacheSize > 0 )
	{
		// The cache only speeds up later runs, so warn and skip rather than assert inside PushSize
		const u64 remainingSize = scratch.size - scratch.used;
		if ( (u64)pipelineCacheSize > remainingSize )
		{
			LOG(Warning, "Pipeline cache (%zu bytes) does not fit in the scratch arena (%llu bytes available). Not saving it to %s.\n",
					pipelineCacheSize, remainingSize, sPipelineCachePath);
			return;
		}

		void *pipelineCacheData = PushSize(scratch, (u32)pipelineCacheSize);
		if ( pipelineCacheData )
		{
			if ( vkGetPipelineCacheData( device.handle, device.pipelineCache, &pipelineCacheSize, pipelineCacheData ) == VK_SUCCESS )
			{
				WriteEntireFile( sPipelineCachePath, pipelineCacheData, (u64)pipelineCacheSize );
				LOG(Info, "Pipeline cache saved to %s (%zu bytes).\n", sPipelineCachePath, pipelineCacheSize);
			}
		}
	}

}

static void *LoadPipelineCacheData(const GraphicsDevice &device, Arena &arena, u32 *pipelineCacheSize)
{
	void *pipelineCacheData = NULL;
	u64 pipelineCacheSize64 = 0;
	if ( GetFileSize(sPipelineCachePath, pipelineCacheSize64, false) && pipelineCacheSize64 > 0 )
	{
		// Skipping the cache is not fatal: vkCreatePipelineCache accepts null initial data and
		// just compiles the pipelines from scratch on this run.
		const u64 remainingSize = arena.size - arena.used;
		if ( pipelineCacheSize64 > remainingSize )
		{
			LOG(Warning, "Pipeline cache at %s (%llu bytes) does not fit in the scratch arena (%llu bytes available). Ignoring it.\n",
					sPipelineCachePath, pipelineCacheSize64, remainingSize);
			pipelineCacheSize64 = 0;
		}
		else
		{
			pipelineCacheData = PushSize(arena, (u32)pipelineCacheSize64);
			if ( ReadEntireFile(sPipelineCachePath, pipelineCacheData, pipelineCacheSize64) )
			{
				LOG(Info, "Pipeline cache loaded from %s (%llu bytes).\n", sPipelineCachePath, pipelineCacheSize64);
			}
			else
			{
				pipelineCacheData = NULL;
				pipelineCacheSize64 = 0;
			}
		}
	}

	*pipelineCacheSize = (u32)pipelineCacheSize64;
	return pipelineCacheData;
}







////////////////////////////////////////////////////////////////////////
// Public API
////////////////////////////////////////////////////////////////////////

//////////////////////////////
// WaitIdle
//////////////////////////////

void WaitQueueIdle(const GraphicsDevice &device)
{
	vkQueueWaitIdle(device.graphicsQueue);
}

void WaitDeviceIdle(const GraphicsDevice &device)
{
	vkDeviceWaitIdle( device.handle );
}


//////////////////////////////
// Debug utils
//////////////////////////////

void BeginDebugGroup(const CommandList &cmd, const char *labelName, float4 labelColor)
{
#if DEVELOPMENT_BUILD
	if ( !cmd.device->support.debugUtils ) { return; }
	const VkDebugUtilsLabelEXT label = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
		.pNext = NULL,
		.pLabelName = labelName,
		.color = { labelColor.r, labelColor.g, labelColor.b, labelColor.a },
	};
	vkCmdBeginDebugUtilsLabelEXT(GetState(cmd).handle, &label);
#endif
}

void EndDebugGroup(const CommandList &cmd)
{
#if DEVELOPMENT_BUILD
	if ( !cmd.device->support.debugUtils ) { return; }
	vkCmdEndDebugUtilsLabelEXT(GetState(cmd).handle);
#endif
}

void InsertDebugLabel(const CommandList &cmd, const char *labelName, float4 labelColor)
{
#if DEVELOPMENT_BUILD
	if ( !cmd.device->support.debugUtils ) { return; }
	const VkDebugUtilsLabelEXT label = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
		.pNext = NULL,
		.pLabelName = labelName,
		.color = { labelColor.r, labelColor.g, labelColor.b, labelColor.a },
	};
	vkCmdInsertDebugUtilsLabelEXT(GetState(cmd).handle, &label);
#endif
}

void SetObjectNamePipeline(const GraphicsDevice &device, PipelineH handle, const char *name)
{
#if DEVELOPMENT_BUILD
	if ( !device.support.debugUtils ) { return; }
	const Pipeline &pipeline = GetPipeline(device, handle);

	const VkDebugUtilsObjectNameInfoEXT objectName = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		.objectType = VK_OBJECT_TYPE_PIPELINE,
		.objectHandle = (uint64_t)pipeline.handle,
		.pObjectName = name,
	};
	VK_CALL( vkSetDebugUtilsObjectNameEXT(device.handle, &objectName) );
#endif
}

void SetObjectNameShader(const GraphicsDevice &device, const ShaderModule &shader, const char *name)
{
#if DEVELOPMENT_BUILD
	if ( !device.support.debugUtils ) { return; }
	const VkDebugUtilsObjectNameInfoEXT objectName = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		.objectType = VK_OBJECT_TYPE_SHADER_MODULE,
		.objectHandle = (uint64_t)shader.handle,
		.pObjectName = name,
	};
	VK_CALL( vkSetDebugUtilsObjectNameEXT(device.handle, &objectName) );
#endif
}

void SetObjectNameImage(const GraphicsDevice &device, ImageH handle, const char *name)
{
#if DEVELOPMENT_BUILD
	if ( !device.support.debugUtils ) { return; }
	const Image &image = GetImageConst(device, handle);

	const VkDebugUtilsObjectNameInfoEXT objectName = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		.objectType = VK_OBJECT_TYPE_IMAGE,
		.objectHandle = (uint64_t)image.handle,
		.pObjectName = name,
	};
	VK_CALL( vkSetDebugUtilsObjectNameEXT(device.handle, &objectName) );
#endif
}

//////////////////////////////
// Device
//////////////////////////////

static bool sExternalSwapchainValid = false;

void RecreateSwapchain(GraphicsDevice &device, Window &window)
{
	// An image acquired at EndFrame that will never be presented leaves its
	// imageAcquired semaphore with a signal no submit will consume, and
	// vkAcquireNextImageKHR requires an unsignaled semaphore. Replace it
	// (callers wait for device idle before recreating the swapchain). The
	// paired fence needs no such handling: AcquireSwapchainImage already
	// waits on it and resets it synchronously before returning.
	if ( device.swapchain.imageAcquired )
	{
		device.swapchain.imageAcquired = false;
		VkSemaphore &semaphore = device.imageAcquiredSemaphores[device.presentationIndex];
		vkDestroySemaphore( device.handle, semaphore, VULKAN_ALLOCATORS );
		const VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VK_CALL( vkCreateSemaphore( device.handle, &semaphoreCreateInfo, VULKAN_ALLOCATORS, &semaphore ) );
	}

	Swapchain swapchain = {};
	const SwapchainInfo &swapchainInfo = device.swapchainInfo;

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.physicalDevice, device.surface, &surfaceCapabilities);

	// Swapchain extent
	if ( surfaceCapabilities.currentExtent.width != 0xFFFFFFFF )
	{
		swapchain.extent = surfaceCapabilities.currentExtent;

		// This is the size of the window. We get it here just in case it was not set by the window manager yet.
		window.width = swapchain.extent.width;
		window.height = swapchain.extent.height;
	}
	else
	{
		swapchain.extent.width = Clamp( window.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width );
		swapchain.extent.height = Clamp( window.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height );
	}

	// Format
	const Format format = device.swapchainInfo.format;

	// Log info
	LOG(Debug, "Swapchain:\n");
#if PLATFORM_ANDROID
	// TODO: Pick the largest divisor of the window size that keeps the scene's on-screen integer
	// scale, instead of always halving. Halving makes the compositor upscale by a non-integer
	// factor when a window dimension is odd, and can lose integer scale steps on some aspect ratios.
	const u32 baseWidth = swapchain.extent.width;
	const u32 baseHeight = swapchain.extent.height;
	swapchain.extent.width = Max(baseWidth/2, surfaceCapabilities.minImageExtent.width);
	swapchain.extent.height = Max(baseHeight/2, surfaceCapabilities.minImageExtent.height);
	LOG(Debug, "- min extent (%ux%u)\n", surfaceCapabilities.minImageExtent.width, surfaceCapabilities.minImageExtent.height);
	LOG(Debug, "- max extent (%ux%u)\n", surfaceCapabilities.maxImageExtent.width, surfaceCapabilities.maxImageExtent.height);
	LOG(Debug, "- base extent (%ux%u)\n", baseWidth, baseHeight);
#endif
	LOG(Debug, "- extent (%ux%u)\n", swapchain.extent.width, swapchain.extent.height);
	LOG(Debug, "- format %s\n", FormatName(format));


	// Pre transform
	VkSurfaceTransformFlagBitsKHR preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	if ( surfaceCapabilities.currentTransform & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ) {
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	} else if ( surfaceCapabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR ) {
		preTransform = VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR;
		const u32 temp = swapchain.extent.width;
		swapchain.extent.width = swapchain.extent.height;
		swapchain.extent.height = temp;
		swapchain.preRotationDegrees = 90.0f;
	} else if ( surfaceCapabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR ) {
		preTransform = VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR;
		swapchain.preRotationDegrees = 180.0f;
	} else if ( surfaceCapabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR ) {
		preTransform = VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR;
		const u32 temp = swapchain.extent.width;
		swapchain.extent.width = swapchain.extent.height;
		swapchain.extent.height = temp;
		swapchain.preRotationDegrees = 270.0f;
	} else if ( surfaceCapabilities.currentTransform & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR ) {
		INVALID_CODE_PATH();
	} else if ( surfaceCapabilities.currentTransform & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR) {
		INVALID_CODE_PATH();
	} else if ( surfaceCapabilities.currentTransform & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR) {
		INVALID_CODE_PATH();
	} else if ( surfaceCapabilities.currentTransform & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR) {
		INVALID_CODE_PATH();
	} else if ( surfaceCapabilities.currentTransform & VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR) {
		INVALID_CODE_PATH();
	} else {
		INVALID_CODE_PATH();
	}
	LOG(Debug, "- preRotationDegrees: %f\n", swapchain.preRotationDegrees);


	// Image count: request the minimum allowed by the surface. Every extra
	// image allows one more pending present in the queue, which under FIFO
	// adds a full vsync period of input latency. MAILBOX instead needs a third
	// image to keep rendering while one is on screen and another one is pending,
	// replacing the pending present without ever blocking.
	u32 imageCount = surfaceCapabilities.minImageCount;
	if ( swapchainInfo.presentMode == VK_PRESENT_MODE_MAILBOX_KHR )
		imageCount = Max( imageCount, 3u );
	if ( surfaceCapabilities.maxImageCount > 0 )
		imageCount = Min( imageCount, surfaceCapabilities.maxImageCount );


	// Queues
	u32 queueFamilyIndices[] = {
		device.graphicsQueueFamilyIndex,
		device.presentQueueFamilyIndex
	};

	VkSharingMode imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	u32 queueFamilyIndexCount = 0;
	u32 *pQueueFamilyIndices = NULL;
	if ( device.graphicsQueueFamilyIndex != device.presentQueueFamilyIndex )
	{
		imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		queueFamilyIndexCount = ARRAY_COUNT(queueFamilyIndices);
		pQueueFamilyIndices = queueFamilyIndices;
	}


	// Composite alpha
	VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	if ( surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR ) {
		compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	} else if ( surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR ) {
		compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	} else if ( surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR ) {
		compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
	} else if ( surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR ) {
		compositeAlpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
	} else {
		INVALID_CODE_PATH();
	}


	// Swapchain
	const VkSwapchainCreateInfoKHR swapchainCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = device.surface,
		.minImageCount = imageCount,
		.imageFormat = swapchainInfo.vkFormat,
		.imageColorSpace = swapchainInfo.colorSpace,
		.imageExtent = swapchain.extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = imageSharingMode,
		.queueFamilyIndexCount = queueFamilyIndexCount,
		.pQueueFamilyIndices = pQueueFamilyIndices,
		.preTransform = preTransform,
		.compositeAlpha = compositeAlpha,
		.presentMode = swapchainInfo.presentMode,
		.clipped = VK_TRUE, // Don't care about pixels obscured by other windows
		.oldSwapchain = device.swapchain.handle,
	};

	VK_CALL( vkCreateSwapchainKHR(device.handle, &swapchainCreateInfo, VULKAN_ALLOCATORS, &swapchain.handle) );


	// Get the swapchain images
	VkImage swapchainImages[MAX_SWAPCHAIN_IMAGE_COUNT] = {};
	vkGetSwapchainImagesKHR( device.handle, swapchain.handle, &swapchain.imageCount, NULL );
	ASSERT( swapchain.imageCount <= ARRAY_COUNT(swapchainImages) );
	vkGetSwapchainImagesKHR( device.handle, swapchain.handle, &swapchain.imageCount, swapchainImages );
	LOG(Debug, "- imageCount %u (min requested %u)\n", swapchain.imageCount, imageCount);

	// Create image views
	for ( u32 i = 0; i < swapchain.imageCount; ++i )
	{
		const VkImage imageHandle = swapchainImages[i];
		const u32 mipLevels = 1;
		const VkImageView imageViewHandle = CreateImageView(device, imageHandle, swapchainInfo.vkFormat, FormatToVulkanAspect(format), mipLevels);
		const Image image = {
			.handle = imageHandle,
			.imageViewHandle = imageViewHandle,
			.format = format,
			.width = swapchain.extent.width,
			.height = swapchain.extent.height,
			.mipLevels = 1,
		};
		swapchain.imageHandles[i] = SwapchainImageHandle(i);
		swapchain.images[i] = image;
	}

	swapchain.valid = true;

	DestroySwapchain(device); // Destroy old swapchain

	device.swapchain = swapchain;
	sExternalSwapchainValid = true;
}

void DestroySwapchain(GraphicsDevice &device)
{
	Swapchain &swapchain = device.swapchain;

	if ( swapchain.handle != VK_NULL_HANDLE )
	{
		for ( u32 i = 0; i < swapchain.imageCount; ++i )
		{
			VkImageView &imageView = swapchain.images[i].imageViewHandle;
			DestroyImageView(device, imageView);
			imageView = VK_NULL_HANDLE;
		}

		vkDestroySwapchainKHR(device.handle, swapchain.handle, VULKAN_ALLOCATORS);
	}

	swapchain = {};
	sExternalSwapchainValid = false;
}

bool IsValidSwapchain(const GraphicsDevice &device)
{
	return device.swapchain.valid && sExternalSwapchainValid;
}

// To call only from platform code
static void InvalidateSwapchain()
{
	sExternalSwapchainValid = false;
}


// Lives in the platform binary, so it survives engine hot reloads
static GraphicsDevice sDevice;

GraphicsDevice &GetGraphicsDevice()
{
	return sDevice;
}

bool InitializeGraphicsDriver(GraphicsDevice &device, Arena scratch)
{
	MemSet(&device, sizeof(device), 0);

	// Load basic Vulkan function pointers
	if ( !VulkanLoadInitFunctions() )
	{
		LOG(Error, "The Vulkan loader was not found in the system.\n");
		return false;
	}


	// Instance creation
	const VkApplicationInfo applicationInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Vulkan application",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "Vulkan engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_1,
	};

#if DEVELOPMENT_BUILD
	u32 instanceLayerCount;
	VK_CALL( vkEnumerateInstanceLayerProperties( &instanceLayerCount, NULL ) );
	VkLayerProperties *instanceLayers = PushArray(scratch, VkLayerProperties, instanceLayerCount);
	VK_CALL( vkEnumerateInstanceLayerProperties( &instanceLayerCount, instanceLayers ) );

	const char *wantedInstanceLayerNames[] = {
		"VK_LAYER_KHRONOS_validation",
	};
	const char *enabledInstanceLayerNames[ARRAY_COUNT(wantedInstanceLayerNames)];
	u32 enabledInstanceLayerCount = 0;

	LOG(Info, "Instance layers:\n");
	for (u32 i = 0; i < instanceLayerCount; ++i)
	{
		bool enabled = false;

		const char *iteratedLayerName = instanceLayers[i].layerName;
		for (u32 j = 0; j < ARRAY_COUNT(wantedInstanceLayerNames); ++j)
		{
			const char *wantedLayerName = wantedInstanceLayerNames[j];
			if ( StrEq( iteratedLayerName, wantedLayerName ) )
			{
				enabledInstanceLayerNames[enabledInstanceLayerCount++] = wantedLayerName;
				enabled = true;
			}
		}

		LOG(Info, "%c %s%s\n", enabled?'*':'-', instanceLayers[i].layerName, enabled ? " (enabled)" : "");
	}
#else
	const char *enabledInstanceLayerNames[1] = {};
	u32 enabledInstanceLayerCount = 0;
#endif

	u32 availableInstanceExtensionCount;
	VK_CALL( vkEnumerateInstanceExtensionProperties( NULL, &availableInstanceExtensionCount, NULL ) );
	VkExtensionProperties *availableInstanceExtensions = PushArray(scratch, VkExtensionProperties, availableInstanceExtensionCount);
	VK_CALL( vkEnumerateInstanceExtensionProperties( NULL, &availableInstanceExtensionCount, availableInstanceExtensions ) );

	const char *requiredInstanceExtensionNames[] = {
		VK_KHR_SURFACE_EXTENSION_NAME,
#if VK_USE_PLATFORM_XCB_KHR
		VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#elif VK_USE_PLATFORM_ANDROID_KHR
		VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
#elif VK_USE_PLATFORM_WIN32_KHR
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif VK_USE_PLATFORM_METAL_EXT
		VK_EXT_METAL_SURFACE_EXTENSION_NAME,
#endif
	};

	const char *enabledInstanceExtensionNames[16];
	u32 enabledInstanceExtensionCount = 0;

	const auto EnableExtension = [&](const char *extensionName)
	{
		bool found = false;

		for (u32 i = 0; i < availableInstanceExtensionCount && !found; ++i)
		{
			found = StrEq( availableInstanceExtensions[i].extensionName, extensionName );
		}

		if ( found )
		{
			ASSERT(enabledInstanceExtensionCount < ARRAY_COUNT(enabledInstanceExtensionNames));
			enabledInstanceExtensionNames[enabledInstanceExtensionCount++] = extensionName;
		}

		return found;
	};

	// Enable mandatory extensions
	for (u32 i = 0; i < ARRAY_COUNT(requiredInstanceExtensionNames); ++i)
	{
		if ( !EnableExtension( requiredInstanceExtensionNames[i] ) )
		{
			return false;
		}
	}

	// Enable optional extensions

	VkInstanceCreateFlags instanceCreateFlags = 0;
#if PLATFORM_APPLE
	// Lists implementations that are not fully conformant, like MoltenVK on top of Metal.
	// Without them, vkCreateInstance fails with VK_ERROR_INCOMPATIBLE_DRIVER on MacOS.
	if ( EnableExtension( VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME ) ) {
		instanceCreateFlags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	}
#endif

#if DEVELOPMENT_BUILD
	if ( EnableExtension( VK_EXT_DEBUG_UTILS_EXTENSION_NAME ) ) {
		device.support.debugUtils = true;
	}

	LOG(Info, "Enabled extensions:\n");
	for ( u32 i = 0; i < enabledInstanceExtensionCount; ++i )
	{
		LOG(Info, "- %s\n", enabledInstanceExtensionNames[i]);
	}
#endif

#if DEVELOPMENT_BUILD
	const VkDebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
		.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
		.pfnUserCallback = VulkanDebugUtilsMessengerCallback,
	};

	const void *pNext = nullptr;
	if ( device.support.debugUtils )
	{
		// To capture events that occur while creating or destroying the instance
		pNext = &debugUtilsCreateInfo;
	}
#else
	const void *pNext = nullptr;
#endif

	const VkInstanceCreateInfo instanceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = pNext,
		.flags = instanceCreateFlags,
		.pApplicationInfo = &applicationInfo,
		.enabledLayerCount = enabledInstanceLayerCount,
		.ppEnabledLayerNames = enabledInstanceLayerNames,
		.enabledExtensionCount = enabledInstanceExtensionCount,
		.ppEnabledExtensionNames = enabledInstanceExtensionNames,
	};

	VkResult result = vkCreateInstance( &instanceCreateInfo, VULKAN_ALLOCATORS, &device.instance );
#if DEVELOPMENT_BUILD
	if ( result == VK_ERROR_LAYER_NOT_PRESENT && instanceCreateInfo.enabledLayerCount > 0 )
	{
		// A layer was listed but its library could not be loaded (a broken install, or on macOS
		// a Homebrew layer whose library is not in the search path, see the Makefile)
		LOG(Warning, "- Instance layers could not be loaded, continuing without them\n");
		VkInstanceCreateInfo noLayersCreateInfo = instanceCreateInfo;
		noLayersCreateInfo.enabledLayerCount = 0;
		result = vkCreateInstance( &noLayersCreateInfo, VULKAN_ALLOCATORS, &device.instance );
	}
#endif
	VK_CALL( result );


	// Load the instance-related Vulkan function pointers
	VulkanLoadInstanceFunctions(device.instance);

#if DEVELOPMENT_BUILD
	if ( device.support.debugUtils )
	{
		VK_CALL( vkCreateDebugUtilsMessengerEXT( device.instance, &debugUtilsCreateInfo, VULKAN_ALLOCATORS, &device.debugUtilsMessenger ) );
	}
#endif

	return true;
}

bool InitializeGraphicsSurface(GraphicsDevice &device, const Window &window)
{
#if VK_USE_PLATFORM_XCB_KHR
	const VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
		.connection = window.impl->connection,
		.window = window.impl->window,
	};
	VK_CALL( vkCreateXcbSurfaceKHR( device.instance, &surfaceCreateInfo, VULKAN_ALLOCATORS, &device.surface ) );
#elif VK_USE_PLATFORM_ANDROID_KHR
	const VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
		.window = window.impl->nativeWindow,
	};
	VK_CALL( vkCreateAndroidSurfaceKHR( device.instance, &surfaceCreateInfo, VULKAN_ALLOCATORS, &device.surface ) );
#elif VK_USE_PLATFORM_WIN32_KHR
	const VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = window.impl->hInstance,
		.hwnd = window.impl->hWnd,
	};
	VK_CALL( vkCreateWin32SurfaceKHR( device.instance, &surfaceCreateInfo, VULKAN_ALLOCATORS, &device.surface ) );
#elif VK_USE_PLATFORM_METAL_EXT
	const VkMetalSurfaceCreateInfoEXT surfaceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
		.pLayer = window.impl->layer,
	};
	VK_CALL( vkCreateMetalSurfaceEXT( device.instance, &surfaceCreateInfo, VULKAN_ALLOCATORS, &device.surface ) );
#endif

	return true;
}

bool InitializeGraphicsDevice(GraphicsDevice &device, Arena scratch)
{
	VkResult result = VK_RESULT_MAX_ENUM;


	// List of physical devices
	u32 physicalDeviceCount = 0;
	VK_CALL( vkEnumeratePhysicalDevices( device.instance, &physicalDeviceCount, NULL ) );
	VkPhysicalDevice *physicalDevices = PushArray( scratch, VkPhysicalDevice, physicalDeviceCount );
	VK_CALL( vkEnumeratePhysicalDevices( device.instance, &physicalDeviceCount, physicalDevices ) );

	const char *requiredDeviceExtensionNames[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if PLATFORM_APPLE
		VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME, // Exposed by MoltenVK
#endif
	};


	// Physical device selection
	u32 bestDeviceScore = 0;

	LOG(Info, "Physical device selection:\n");

	for (u32 i = 0; i < physicalDeviceCount; ++i)
	{
		LOG(Info, "- Device[%d]\n", i);

		u32 deviceScore = 0;
		VkPhysicalDevice physicalDevice = physicalDevices[i];

		Arena scratch2 = MakeSubArena(scratch, "Scratch - InitializeGraphicsDevice");

		// Prioritize discrete GPUs over integrated ones
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties( physicalDevice, &properties );
		LOG(Info, "  - deviceType: %s\n", VkPhysicalDeviceTypeToString( properties.deviceType ));
		deviceScore +=
			( properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ) ? 200 :
			( properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ) ? 100 :
			( properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU ) ? 50 :
			( properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU ) ? 10 :
			0;
		if ( deviceScore == 0 ) {
			LOG(Error, "  - Rejected: Unsupported device type\n");
			continue;
		}

		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures( physicalDevice, &features );
		// Check any needed features here
		if ( features.samplerAnisotropy == VK_FALSE ) {
			LOG(Error, "  - Rejected: No sampler anisotropy\n");
			continue;
		}

		// Check the available queue families
		u32 queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties( physicalDevice, &queueFamilyCount, NULL);
		VkQueueFamilyProperties *queueFamilies = PushArray( scratch2, VkQueueFamilyProperties, queueFamilyCount );
		vkGetPhysicalDeviceQueueFamilyProperties( physicalDevice, &queueFamilyCount, queueFamilies );

		u32 gfxFamilyIndex = -1;
		u32 presentFamilyIndex = -1;
		for ( u32 i = 0; i < queueFamilyCount; ++i )
		{
			VkBool32 presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR( physicalDevice, i, device.surface, &presentSupport );
			if ( presentSupport )
			{
				presentFamilyIndex = i;
			}

			// We want the gfx queue to support both gfx and compute tasks
			if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
				(queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
			{
				gfxFamilyIndex = i;
			}
		}

		// We don't want a device that does not support both queue types
		if ( gfxFamilyIndex == -1 || presentFamilyIndex == -1 ) {
			LOG(Error, "  - Rejected: No gfx or present queues available\n");
			continue;
		}

		// Check if this physical device has all the required extensions
		u32 deviceExtensionCount;
		VK_CALL( vkEnumerateDeviceExtensionProperties( physicalDevice, NULL, &deviceExtensionCount, NULL ) );
		VkExtensionProperties *deviceExtensions = PushArray( scratch2, VkExtensionProperties, deviceExtensionCount );
		VK_CALL( vkEnumerateDeviceExtensionProperties( physicalDevice, NULL, &deviceExtensionCount, deviceExtensions ) );

		u32 foundDeviceExtensionCount = 0;

		for (u32 j = 0; j < ARRAY_COUNT(requiredDeviceExtensionNames); ++j)
		{
			const char *requiredExtensionName = requiredDeviceExtensionNames[j];
			bool found = false;

			for (u32 i = 0; i < deviceExtensionCount; ++i)
			{
				const char *availableExtensionName = deviceExtensions[i].extensionName;
				if ( StrEq( availableExtensionName, requiredExtensionName ) )
				{
					foundDeviceExtensionCount++;
					found = true;
					break;
				}
			}

			if ( !found )
			{
				LOG(Error, "  - Extension %s not available\n", requiredExtensionName);
				break;
			}
		}

		// We only want devices with all the required extensions
		if ( foundDeviceExtensionCount < ARRAY_COUNT(requiredDeviceExtensionNames) ) {
			LOG(Error, "  - Rejected: Not all required extensions available\n");
			continue;
		}

		// Swapchain format
		u32 surfaceFormatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, device.surface, &surfaceFormatCount, NULL);
		if ( surfaceFormatCount == 0 ) {
			LOG(Error, "  - Rejected: No surface formats available\n");
			continue;
		}
		VkSurfaceFormatKHR *surfaceFormats = PushArray( scratch2, VkSurfaceFormatKHR, surfaceFormatCount );
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, device.surface, &surfaceFormatCount, surfaceFormats);

		struct FormatPriorityEntry
		{
			VkFormat format;
			VkColorSpaceKHR colorSpace;
		};
		const FormatPriorityEntry formatPriorityArray[] = {
			{ VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
			{ VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
		};
		u32 selectedEntryIndex = U32_MAX;
		u32 surfaceFormatIndex = 0;

		device.swapchainInfo.vkFormat = VK_FORMAT_MAX_ENUM;
		for ( u32 i = 0; i < surfaceFormatCount; ++i )
		{
			bool supported = false;
			const VkFormat format = surfaceFormats[i].format;
			const VkColorSpaceKHR colorSpace = surfaceFormats[i].colorSpace;

			for (u32 entryIndex = 0; entryIndex < ARRAY_COUNT(formatPriorityArray); ++entryIndex)
			{
				if ( entryIndex < selectedEntryIndex &&
					format == formatPriorityArray[entryIndex].format &&
					colorSpace == formatPriorityArray[entryIndex].colorSpace )
				{
					selectedEntryIndex = entryIndex;
					surfaceFormatIndex = i;
					supported = true;
				}
			}

			LOG(Info, "  - Surface format[%d]. %s (%d) / colorSpace: %d %s\n", i, FormatName(FormatFromVulkan(format)), format, colorSpace, supported ? "" : "(unsupported)");
		}

		if ( selectedEntryIndex == U32_MAX )
		{
			LOG(Error, "  - Rejected: Could not find a supported surface format\n");
			continue;
		}

		LOG(Info, "  - Selected surface format: %d\n", surfaceFormatIndex);
		device.swapchainInfo.vkFormat = surfaceFormats[surfaceFormatIndex].format;
		device.swapchainInfo.format = FormatFromVulkan(device.swapchainInfo.vkFormat);
		device.swapchainInfo.colorSpace = surfaceFormats[surfaceFormatIndex].colorSpace;

		// Swapchain present mode
		u32 surfacePresentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, device.surface, &surfacePresentModeCount, NULL);
		if ( surfacePresentModeCount == 0 ) {
			LOG(Error, "  - Rejected: No present modes available\n");
			continue;
		}
		VkPresentModeKHR *surfacePresentModes = PushArray( scratch2, VkPresentModeKHR, surfacePresentModeCount );
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, device.surface, &surfacePresentModeCount, surfacePresentModes);

#define USE_SWAPCHAIN_MAILBOX_PRESENT_MODE 0
#if USE_SWAPCHAIN_MAILBOX_PRESENT_MODE
		device.swapchainInfo.presentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
		for ( u32 i = 0; i < surfacePresentModeCount; ++i )
		{
			if ( surfacePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR )
			{
				device.swapchainInfo.presentMode = surfacePresentModes[i];
			}
		}
		if ( device.swapchainInfo.presentMode == VK_PRESENT_MODE_MAX_ENUM_KHR )
			device.swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
#else
		device.swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
#endif

		// At this point, we know this device meets all the requirements
		if (deviceScore > bestDeviceScore)
		{
			bestDeviceScore = deviceScore;
			device.physicalDevice = physicalDevice;
			device.graphicsQueueFamilyIndex = gfxFamilyIndex;
			device.presentQueueFamilyIndex = presentFamilyIndex;
		}
	}

	if ( bestDeviceScore == 0 )
	{
		LOG(Error, "- Could not find any suitable GFX device.\n");
		return false;
	}


	// Device creation
	u32 queueCount = 1;
	float queuePriorities[1] = { 1.0f };
	VkDeviceQueueCreateInfo queueCreateInfos[2] = {};
	u32 queueCreateInfoCount = 0;
	u32 queueCreateInfoIndex = queueCreateInfoCount++;
	queueCreateInfos[queueCreateInfoIndex].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[queueCreateInfoIndex].queueFamilyIndex = device.graphicsQueueFamilyIndex;
	queueCreateInfos[queueCreateInfoIndex].queueCount = queueCount;
	queueCreateInfos[queueCreateInfoIndex].pQueuePriorities = queuePriorities;
	if ( device.presentQueueFamilyIndex != device.graphicsQueueFamilyIndex )
	{
		queueCreateInfoIndex = queueCreateInfoCount++;
		queueCreateInfos[queueCreateInfoIndex].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfos[queueCreateInfoIndex].queueFamilyIndex = device.presentQueueFamilyIndex;
		queueCreateInfos[queueCreateInfoIndex].queueCount = queueCount;
		queueCreateInfos[queueCreateInfoIndex].pQueuePriorities = queuePriorities;
	}

#if 0
	u32 deviceExtensionCount;
	VK_CALL( vkEnumerateDeviceExtensionProperties( device.physicalDevice, NULL, &deviceExtensionCount, NULL ) );
	VkExtensionProperties *deviceExtensions = PushArray(scratch, VkExtensionProperties, deviceExtensionCount);
	VK_CALL( vkEnumerateDeviceExtensionProperties( device.physicalDevice, NULL, &deviceExtensionCount, deviceExtensions ) );

	// We don't need this loop anymore unless we want to print this device extensions
	const char *enabledDeviceExtensionNames[ARRAY_COUNT(requiredDeviceExtensionNames)];
	u32 enabledDeviceExtensionCount = 0;

	LOG(Info, "Device extensions:\n");
	for (u32 i = 0; i < deviceExtensionCount; ++i)
	{
		bool enabled = false;

		const char *availableExtensionName = deviceExtensions[i].extensionName;
		for (u32 j = 0; j < ARRAY_COUNT(requiredDeviceExtensionNames); ++j)
		{
			const char *requiredExtensionName = requiredDeviceExtensionNames[j];
			if ( StrEq( availableExtensionName, requiredExtensionName ) )
			{
				enabledDeviceExtensionNames[enabledDeviceExtensionCount++] = requiredExtensionName;
				enabled = true;
			}
		}

		LOG(Info, "%c %s\n", enabled?'*':' ', deviceExtensions[i].extensionName);
	}
#else
	const char **enabledDeviceExtensionNames = requiredDeviceExtensionNames;
	const u32 enabledDeviceExtensionCount = ARRAY_COUNT(requiredDeviceExtensionNames);
#endif

	VkPhysicalDeviceFeatures requiredPhysicalDeviceFeatures = {};
	requiredPhysicalDeviceFeatures.samplerAnisotropy = VK_TRUE;

	const VkDeviceCreateInfo deviceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = queueCreateInfoCount,
		.pQueueCreateInfos = queueCreateInfos,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = enabledDeviceExtensionCount,
		.ppEnabledExtensionNames = enabledDeviceExtensionNames,
		.pEnabledFeatures = &requiredPhysicalDeviceFeatures,
	};

	result = vkCreateDevice( device.physicalDevice, &deviceCreateInfo, VULKAN_ALLOCATORS, &device.handle );
	if ( result != VK_SUCCESS )
	{
		LOG(Error, "vkCreateDevice failed!\n");
		return false;
	}


	// Load all the remaining device-related Vulkan function pointers
	VulkanLoadDeviceFunctions(device.handle);


	// Print physical device info
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(device.physicalDevice, &properties);
	LOG(Info, "Physical device info:\n");
	LOG(Info, "- apiVersion: %u.%u.%u\n",
			VK_API_VERSION_MAJOR(properties.apiVersion),
			VK_API_VERSION_MINOR(properties.apiVersion),
			VK_API_VERSION_PATCH(properties.apiVersion)); // uint32_t
	LOG(Info, "- driverVersion: %u.%u.%u\n",
			VK_VERSION_MAJOR(properties.driverVersion),
			VK_VERSION_MINOR(properties.driverVersion),
			VK_VERSION_PATCH(properties.driverVersion)); // uint32_t
	LOG(Info, "- vendorID: %u\n", properties.vendorID); // uint32_t
	LOG(Info, "- deviceID: %u\n", properties.deviceID); // uint32_t
	LOG(Info, "- deviceType: %s\n", VkPhysicalDeviceTypeToString(properties.deviceType)); // VkPhysicalDeviceType
	LOG(Info, "- deviceName: %s\n", properties.deviceName); // char
	//LOG(Info, "- \n"); // uint8_t							 pipelineCacheUUID[VK_UUID_SIZE];
	//LOG(Info, "- \n"); // VkPhysicalDeviceLimits			  limits;
	//LOG(Info, "- \n"); // VkPhysicalDeviceSparseProperties	sparseProperties;


	// Depth format support
	const VkFormat depthFormatCandidates[] = {
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT
	};
	const VkFormat defaultVkDepthFormat = FindSupportedFormat(device,
			depthFormatCandidates, ARRAY_COUNT(depthFormatCandidates),
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	device.caps.defaultDepthFormat = FormatFromVulkan(defaultVkDepthFormat);


	// Get alignments
	device.caps.alignment.uniformBufferOffset = properties.limits.minUniformBufferOffsetAlignment;
	device.caps.alignment.storageBufferOffset = properties.limits.minStorageBufferOffsetAlignment;
	device.caps.alignment.optimalBufferCopyOffset = properties.limits.optimalBufferCopyOffsetAlignment;
	device.caps.alignment.optimalBufferCopyRowPitch = properties.limits.optimalBufferCopyRowPitchAlignment;
	// TODO: We should get `bufferImageGranularity` to avoid aliasing between adjacent resources with linear vs. tiled layouts


	// Timestamp queries support
	device.caps.timestampQueries = properties.limits.timestampComputeAndGraphics;
	device.limits.timestampPeriod = properties.limits.timestampPeriod;


	// Format support
	for (u32 format = 0; format < FormatCount; ++format)
	{
		const VkFormat vkFormat = FormatToVulkan((Format)format);
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(device.physicalDevice, vkFormat, &formatProperties);
		device.caps.formatSupport[format].linearFilteredSampling = formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
		device.caps.formatSupport[format].colorAttachmentBlend = formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT;
	}


	// Create heaps
	device.heaps[HeapType_General] = CreateHeap(device, HeapType_General, HeapSize_General, false);
	device.heaps[HeapType_RTs] = CreateHeap(device, HeapType_RTs, MB(256), false);
	device.heaps[HeapType_Staging] = CreateHeap(device, HeapType_Staging, MB(16), true);
	device.heaps[HeapType_Dynamic] = CreateHeap(device, HeapType_Dynamic, MB(32), true);
	device.heaps[HeapType_Readback] = CreateHeap(device, HeapType_Readback, MB(1), true);


	// Retrieve queues
	vkGetDeviceQueue(device.handle, device.graphicsQueueFamilyIndex, 0, &device.graphicsQueue);
	vkGetDeviceQueue(device.handle, device.presentQueueFamilyIndex, 0, &device.presentQueue);


	// Command pools
	for (u32 i = 0; i < ARRAY_COUNT(device.commandPools); ++i)
	{
		const VkCommandPoolCreateInfo commandPoolCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = 0, // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT allows individual resets using vkResetCommandBuffer
			.queueFamilyIndex = device.graphicsQueueFamilyIndex,
		};
		VK_CALL( vkCreateCommandPool(device.handle, &commandPoolCreateInfo, VULKAN_ALLOCATORS, &device.commandPools[i]) );
	}


	// Command buffers
	for (u32 i = 0; i < ARRAY_COUNT(device.commandBuffers); ++i)
	{
		const VkCommandBufferAllocateInfo commandBufferAllocInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = device.commandPools[i],
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};
		VK_CALL( vkAllocateCommandBuffers( device.handle, &commandBufferAllocInfo, &device.commandBuffers[i]) );
	}


	// Transient command pool
	const VkCommandPoolCreateInfo transientCommandPoolCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
		.queueFamilyIndex = device.graphicsQueueFamilyIndex,
	};
	VK_CALL( vkCreateCommandPool(device.handle, &transientCommandPoolCreateInfo, VULKAN_ALLOCATORS, &device.transientCommandPool) );


	// Resource pools
	Arena poolArena = MakeArena(device.poolMemory, sizeof(device.poolMemory), "GraphicsPools");
	Initialize(device.bindGroupLayoutPool, poolArena, MAX_BIND_GROUP_LAYOUTS);
	Initialize(device.bindGroupAllocatorPool, poolArena, MAX_BIND_GROUP_ALLOCATORS);
	Initialize(device.bindGroupPool, poolArena, MAX_ALLOCATED_BIND_GROUPS);
	Initialize(device.bufferPool, poolArena, MAX_BUFFERS);
	Initialize(device.bufferViewPool, poolArena, MAX_BUFFERS);
	Initialize(device.imagePool, poolArena, MAX_IMAGES);
	Initialize(device.samplerPool, poolArena, MAX_SAMPLERS);
	Initialize(device.pipelinePool, poolArena, MAX_PIPELINES);
	Initialize(device.renderPassPool, poolArena, MAX_RENDERPASSES);
	Initialize(device.framebufferPool, poolArena, MAX_FRAMEBUFFERS);
	Initialize(device.timestampPoolPool, poolArena, MAX_TIMESTAMP_POOLS);


	// Synchronization objects
	const VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	const VkFenceCreateInfo fenceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		//.flags = VK_FENCE_CREATE_SIGNALED_BIT, // Start signaled
	};
	for ( u32 i = 0; i < MAX_SWAPCHAIN_IMAGE_COUNT; ++i )
	{
		VK_CALL( vkCreateSemaphore( device.handle, &semaphoreCreateInfo, VULKAN_ALLOCATORS, &device.imageAcquiredSemaphores[i] ) );
		VK_CALL( vkCreateFence( device.handle, &fenceCreateInfo, VULKAN_ALLOCATORS, &device.imageAcquiredFences[i] ) );
		VK_CALL( vkCreateSemaphore( device.handle, &semaphoreCreateInfo, VULKAN_ALLOCATORS, &device.renderFinishedSemaphores[i] ) );
	}

	for ( u32 i = 0; i < MAX_FENCES; ++i )
	{
		VK_CALL( vkCreateFence( device.handle, &fenceCreateInfo, VULKAN_ALLOCATORS, &device.fences[i] ) );
	}


	// Create pipeline cache, loading previously saved data if available
	u32 pipelineCacheSize = 0;
	void *pipelineCacheData = LoadPipelineCacheData(device, scratch, &pipelineCacheSize);

	const VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
		.flags = 0,
		.initialDataSize = (size_t)pipelineCacheSize,
		.pInitialData = pipelineCacheData,
	};
	VK_CALL( vkCreatePipelineCache( device.handle, &pipelineCacheCreateInfo, VULKAN_ALLOCATORS, &device.pipelineCache ) );

	return true;
}

bool IsValidGraphicsDevice(const GraphicsDevice &device)
{
	const bool valid = device.handle != VK_NULL_HANDLE;
	return valid;
}



//////////////////////////////
// BindGroupAllocator
//////////////////////////////

static BindGroupAllocator &GetBindGroupAllocator(GraphicsDevice &device, BindGroupAllocatorH handle)
{
	const i32 index = GetElementIndex(device.bindGroupAllocatorPool, handle);
	ASSERT(index >= 0);
	return device.bindGroupAllocators[index];
}

// A zero handle reads as an empty layout
static const BindGroupLayout &GetBindGroupLayout(const GraphicsDevice &device, BindGroupLayoutH handle)
{
	static const BindGroupLayout sNullLayout = {};
	const i32 index = GetElementIndex(device.bindGroupLayoutPool, handle);
	return index >= 0 ? device.bindGroupLayouts[index] : sNullLayout;
}

static BindGroup &GetBindGroup(GraphicsDevice &device, BindGroupH handle)
{
	const i32 index = GetElementIndex(device.bindGroupPool, handle);
	ASSERT(index >= 0);
	return device.bindGroups[index];
}

// sign: 1 adds, -1 removes
static void CountBindGroupDescriptors(BindGroupAllocatorCounts &counts, const BindGroupLayout &layout, i32 sign)
{
	for (u32 i = 0; i < layout.bindingCount; ++i)
	{
		const ShaderBinding &binding = layout.bindings[i];
		switch (binding.type) {
			case SpvTypeImage: counts.textureCount += sign; break;
			case SpvTypeSampler: counts.samplerCount += sign; break;
			case SpvTypeSampledImage: counts.combinedImageSamplerCount += sign; break;
			case SpvTypeUniformBuffer: counts.uniformBufferCount += sign; break;
			case SpvTypeStorageBuffer: counts.storageBufferCount += sign; break;
			case SpvTypeStorageTexelBuffer: counts.storageTexelBufferCount += sign; break;
			default: INVALID_CODE_PATH();
		}
	}
	counts.groupCount += sign;
}

// The caller releases the descriptor sets, resetting or destroying the pool
static void FreeAllocatorBindGroups(GraphicsDevice &device, BindGroupAllocatorH allocatorH)
{
	for (u16 i = 0; i < HandleCount(device.bindGroupPool); ++i)
	{
		BindGroup &bindGroup = device.bindGroups[i];
		if ( bindGroup.allocator == allocatorH && bindGroup.handle )
		{
			bindGroup.handle = VK_NULL_HANDLE; // Released with the pool
			FreeHandle(device.bindGroupPool, GetHandleAt(device.bindGroupPool, i));
		}
	}
}

BindGroupAllocatorH CreateBindGroupAllocator(GraphicsDevice &device, const BindGroupAllocatorDesc &desc)
{
	const BindGroupAllocatorCounts &counts = desc.counts;

	u32 poolSizeCount = 0;
	VkDescriptorPoolSize poolSizes[8] = {};

	if (counts.uniformBufferCount > 0) {
		poolSizes[poolSizeCount++] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, counts.uniformBufferCount };
	}
	if (counts.storageBufferCount > 0) {
		poolSizes[poolSizeCount++] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, counts.storageBufferCount };
	}
	if (counts.storageTexelBufferCount > 0) {
		poolSizes[poolSizeCount++] = { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, counts.storageTexelBufferCount };
	}
	if (counts.textureCount > 0) {
		poolSizes[poolSizeCount++] = { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, counts.textureCount };
	}
	if (counts.samplerCount > 0) {
		poolSizes[poolSizeCount++] = { VK_DESCRIPTOR_TYPE_SAMPLER, counts.samplerCount };
	}
	if (counts.combinedImageSamplerCount > 0) {
		poolSizes[poolSizeCount++] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, counts.combinedImageSamplerCount };
	}

	ASSERT(poolSizeCount <= ARRAY_COUNT(poolSizes));

	const VkDescriptorPoolCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = counts.allowIndividualFrees ? VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT : 0U,
		.maxSets = counts.groupCount,
		.poolSizeCount = poolSizeCount,
		.pPoolSizes = poolSizes,
	};

	VkDescriptorPool descriptorPool;
	VK_CALL( vkCreateDescriptorPool( device.handle, &createInfo, VULKAN_ALLOCATORS, &descriptorPool) );

	const BindGroupAllocatorH handle = NewHandle(device.bindGroupAllocatorPool);
	device.bindGroupAllocators[GetHandleIndex(device.bindGroupAllocatorPool, handle)] = {
		.name = InternStringGfx(desc.name),
		.maxCounts = counts,
		.handle = descriptorPool,
	};
	return handle;
}

void DestroyBindGroupAllocator(GraphicsDevice &device, BindGroupAllocatorH allocatorH)
{
	if ( !IsValid(allocatorH) ) { return; }

	FreeAllocatorBindGroups(device, allocatorH);
	FreeHandle(device.bindGroupAllocatorPool, allocatorH);
}

// The GPU must be done with this allocator's bind groups
void ResetBindGroupAllocator(GraphicsDevice &device, BindGroupAllocatorH allocatorH)
{
	BindGroupAllocator &allocator = GetBindGroupAllocator(device, allocatorH);
	VK_CALL( vkResetDescriptorPool(device.handle, allocator.handle, 0) );
	FreeAllocatorBindGroups(device, allocatorH);
	allocator.usedCounts = {};
}


//////////////////////////////
// BindGroup
//////////////////////////////



// Owned by the device until it is destroyed. Equal layouts are shared.
BindGroupLayoutH CreateBindGroupLayout(GraphicsDevice &device, const ShaderBinding bindings[], u8 bindingCount)
{
	ASSERT(bindingCount <= MAX_SHADER_BINDINGS);

	// Try finding if an equal layout was already created
	const u32 crc = HashFNV(bindings, bindingCount * sizeof(ShaderBinding));

	for (u16 i = 0; i < HandleCount(device.bindGroupLayoutPool); ++i)
	{
		const BindGroupLayout &layout = device.bindGroupLayouts[i];
		if ( layout.crc == crc && layout.bindingCount == bindingCount )
		{
			const int res = MemCompare(layout.bindings, bindings, layout.bindingCount * sizeof(ShaderBinding));
			if ( res == 0 )
			{
				return GetHandleAt(device.bindGroupLayoutPool, i);
			}
		}
	}

	// Create a new bind group layout
	VkDescriptorSetLayoutBinding vkBindings[MAX_SHADER_BINDINGS] = {};

	for (u32 bindingIndex = 0; bindingIndex < bindingCount; ++bindingIndex)
	{
		const ShaderBinding &shaderBinding = bindings[bindingIndex];

		VkDescriptorSetLayoutBinding &binding = vkBindings[bindingIndex];
		binding.binding = shaderBinding.binding;
		binding.descriptorType = SpvDescriptorTypeToVulkan((SpvType)shaderBinding.type);
		binding.descriptorCount = 1;
		binding.stageFlags = SpvStageFlagsToVulkan(shaderBinding.stageFlags);
		binding.pImmutableSamplers = NULL;
	}

	const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = bindingCount,
		.pBindings = bindingCount > 0 ? vkBindings : NULL,
	};

	VkDescriptorSetLayout layoutHandle;
	VK_CALL( vkCreateDescriptorSetLayout(device.handle, &descriptorSetLayoutCreateInfo, VULKAN_ALLOCATORS, &layoutHandle) );

	const BindGroupLayoutH handle = NewHandle(device.bindGroupLayoutPool);
	BindGroupLayout &layout = device.bindGroupLayouts[GetHandleIndex(device.bindGroupLayoutPool, handle)];
	layout = {
		.handle = layoutHandle,
		.crc = crc,
		.bindingCount = bindingCount,
	};
	MemCopy(layout.bindings, bindings, bindingCount * sizeof(ShaderBinding));

	return handle;
}

BindGroupH CreateBindGroup(GraphicsDevice &device, BindGroupLayoutH layoutH, BindGroupAllocatorH allocatorH)
{
	const BindGroupLayout &layout = GetBindGroupLayout(device, layoutH);
	if ( !layout.handle ) { return {}; }

	BindGroupAllocator &allocator = GetBindGroupAllocator(device, allocatorH);

	// Update the used descriptor counters in the allocator
	CountBindGroupDescriptors(allocator.usedCounts, layout, 1);

	for (u32 i = 0; i < ARRAY_COUNT(allocator.usedCounts.type); ++i)
	{
		if (allocator.usedCounts.type[i] > allocator.maxCounts.type[i]) {
			const char *typeName = sCountTypeName[i];
			LOG(Warning, "BindGroupAllocator %s: Type %s exceeded allocation (%u > %u)\n", allocator.name, typeName, allocator.usedCounts.type[i], allocator.maxCounts.type[i]);
		}
	}

	const VkDescriptorSetAllocateInfo allocateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = allocator.handle,
		.descriptorSetCount = 1,
		.pSetLayouts = &layout.handle,
	};
	VkDescriptorSet descriptorSet;
	VK_CALL( vkAllocateDescriptorSets(device.handle, &allocateInfo, &descriptorSet) );

	const BindGroupH handle = NewHandle(device.bindGroupPool);
	GetBindGroup(device, handle) = {
		.handle = descriptorSet,
		.descriptorPool = allocator.handle,
		.layout = layoutH,
		.allocator = allocatorH,
	};
	return handle;
}

// Requires allowIndividualFrees. The GPU must be done with it.
void DestroyBindGroup(GraphicsDevice &device, BindGroupH bindGroupH)
{
	if ( !IsValid(bindGroupH) ) { return; }

	const BindGroup &bindGroup = GetBindGroup(device, bindGroupH);
	BindGroupAllocator &allocator = GetBindGroupAllocator(device, bindGroup.allocator);
	ASSERT(allocator.maxCounts.allowIndividualFrees && "BindGroupAllocator not created with allowIndividualFrees");

	CountBindGroupDescriptors(allocator.usedCounts, GetBindGroupLayout(device, bindGroup.layout), -1);

	FreeHandle(device.bindGroupPool, bindGroupH);
}

void UpdateBindGroup(const GraphicsDevice &device, const BindGroupDesc &bindGroupDesc, BindGroupH bindGroupH)
{
	const i32 bindGroupIndex = GetElementIndex(device.bindGroupPool, bindGroupH);
	if ( bindGroupIndex < 0 ) { return; }

	const BindGroupLayout &layout = GetBindGroupLayout(device, bindGroupDesc.layout);
	const VkDescriptorSet descriptorSet = device.bindGroups[bindGroupIndex].handle;

	VkDescriptorGenericInfo descriptorInfos[MAX_SHADER_BINDINGS] = {};
	VkWriteDescriptorSet descriptorWrites[MAX_SHADER_BINDINGS] = {};
	u32 descriptorWriteCount = 0;

	for (u32 i = 0; i < layout.bindingCount; ++i)
	{
		const ShaderBinding &shaderBinding = layout.bindings[i];
		const ResourceBinding &resourceBinding = GetResourceBinding(bindGroupDesc.bindings, shaderBinding.binding);

		if ( !AddDescriptorWrite(device, resourceBinding, shaderBinding, descriptorSet, descriptorInfos, descriptorWrites, descriptorWriteCount) )
		{
			LOG(Warning, "Could not add descriptor write for binding %s of pipeline %s.\n", shaderBinding.name, "<pipeline>");
		}
	}

	UpdateDescriptorSets(device, descriptorWrites, descriptorWriteCount);
}

BindGroupH CreateFullBindGroup(GraphicsDevice &device, const BindGroupDesc &desc, BindGroupAllocatorH allocatorH)
{
	const BindGroupH bindGroupH = CreateBindGroup(device, desc.layout, allocatorH);

	UpdateBindGroup(device, desc, bindGroupH);

	return bindGroupH;
}



//////////////////////////////
// Heap allocation
//////////////////////////////

Alloc Allocate(Heap &heap, u32 size, u32 alignment)
{
	if ( heap.type == HeapType_General )
	{
		ASSERT( heap.used == 0 ); // Check only this code path allocates from the general heap

		const u32 totalSize = size + alignment;
		const OffsetAllocator::Allocation allocation = generalAllocator.allocate(totalSize);
		if (allocation.offset == OffsetAllocator::Allocation::NO_SPACE) {
			LOG(Error, "Allocate - Heap: %s (%luMB) - Could not allocate size(%uB) with alignment (%uB)\n", HeapTypeToString(heap.type), heap.size / MB(1), size, alignment);
		}

		const VkDeviceSize offset = AlignUp( allocation.offset, alignment );

		const Alloc alloc = {
			.heap = heap.type,
			.offset = offset,
			.size = size,
			.allocation = allocation,
		};

		return alloc;
	}
	else
	{
		const VkDeviceSize offset = AlignUp( heap.used, alignment );

		ASSERTMSG( offset + size <= heap.size, "Allocate - Heap: %s (%luMB) - Could not allocate size (%uB) with alignment (%uB)\n", HeapTypeToString(heap.type), heap.size / MB(1), size, alignment);
		heap.used = offset + size;

		const Alloc alloc = {
			.heap = heap.type,
			.offset = offset,
			.size = size,
		};

		return alloc;
	}
}

void Deallocate(Alloc alloc)
{
	// TODO: Missing implementation
	// Ideally we would want to have to possibility to associate a different allocator
	// for each heap so that we can use it to alloc/free chunks of memory on it.
	if ( alloc.heap == HeapType_General )
	{
		generalAllocator.free(alloc.allocation);
	}
}



//////////////////////////////
// Buffer
//////////////////////////////

Buffer CreateBufferInternal(GraphicsDevice &device, u32 size, BufferUsageFlags bufferUsageFlags, HeapType heapType)
{
	// Buffer
	const VkBufferCreateInfo bufferCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = BufferUsageFlagsToVulkan(bufferUsageFlags),
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	VkBuffer bufferHandle;
	VK_CALL( vkCreateBuffer(device.handle, &bufferCreateInfo, VULKAN_ALLOCATORS, &bufferHandle) );

	// Memory
	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(device.handle, bufferHandle, &memoryRequirements);

	Heap &heap = device.heaps[heapType];
	const Alloc alloc = Allocate(heap, memoryRequirements.size, memoryRequirements.alignment);

	VK_CALL( vkBindBufferMemory(device.handle, bufferHandle, heap.memory, alloc.offset) );

	const Buffer buffer = {
		.handle = bufferHandle,
		.alloc = alloc,
		.size = size,
	};
	return buffer;
}

BufferH CreateBuffer(GraphicsDevice &device, u32 size, BufferUsageFlags bufferUsageFlags, HeapType heapType)
{
	const Buffer buffer = CreateBufferInternal(device, size, bufferUsageFlags, heapType);
	const BufferH bufferHandle = NewHandle(device.bufferPool);
	device.buffers[GetHandleIndex(device.bufferPool, bufferHandle)] = buffer;
	return bufferHandle;
}

Buffer &GetBuffer(GraphicsDevice &device, BufferH bufferHandle)
{
	const i32 index = GetElementIndex(device.bufferPool, bufferHandle);
	ASSERT(index >= 0);
	return device.buffers[index];
}

const Buffer &GetBufferConst(const GraphicsDevice &device, BufferH bufferHandle)
{
	static const Buffer sNullBuffer = {};
	const i32 index = GetElementIndex(device.bufferPool, bufferHandle);
	return index >= 0 ? device.buffers[index] : sNullBuffer;
}

void DestroyBuffer(const GraphicsDevice &device, const Buffer &buffer)
{
	vkDestroyBuffer( device.handle, buffer.handle, VULKAN_ALLOCATORS );

	Deallocate(buffer.alloc);
}

void DestroyBufferH(GraphicsDevice &device, BufferH bufferH)
{
	if ( IsValid(bufferH) ) {
		FreeHandle(device.bufferPool, bufferH);
	}
}


//////////////////////////////
// BufferView
//////////////////////////////

BufferView CreateBufferViewInternal(const GraphicsDevice &device, const Buffer &buffer, Format format, u32 offset, u32 size)
{
	const VkBufferViewCreateInfo bufferViewCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
		.buffer = buffer.handle,
		.format = FormatToVulkan(format),
		.offset = offset,
		.range = size > 0 ? size : buffer.size,
	};

	VkBufferView bufferViewHandle;
	VK_CALL( vkCreateBufferView(device.handle, &bufferViewCreateInfo, VULKAN_ALLOCATORS, &bufferViewHandle) );

	const BufferView bufferView = {
		.handle = bufferViewHandle
	};
	return bufferView;
}

BufferViewH CreateBufferView(GraphicsDevice &device, BufferH bufferHandle, Format format, u32 offset, u32 size)
{
	const Buffer &buffer = GetBufferConst(device, bufferHandle);
	const BufferView bufferView = CreateBufferViewInternal(device, buffer, format, offset, size);

	const BufferViewH bufferViewH = NewHandle(device.bufferViewPool);
	device.bufferViews[GetHandleIndex(device.bufferViewPool, bufferViewH)] = bufferView;
	return bufferViewH;
}

const BufferView &GetBufferView(const GraphicsDevice &device, BufferViewH bufferViewH)
{
	static const BufferView sNullBufferView = {};
	const i32 index = GetElementIndex(device.bufferViewPool, bufferViewH);
	return index >= 0 ? device.bufferViews[index] : sNullBufferView;
}

void DestroyBufferView(const GraphicsDevice &device, const BufferView &bufferView)
{
	vkDestroyBufferView( device.handle, bufferView.handle, VULKAN_ALLOCATORS );
}

void DestroyBufferViewH(GraphicsDevice &device, BufferViewH bufferViewH)
{
	if ( IsValid(bufferViewH) ) {
		FreeHandle(device.bufferViewPool, bufferViewH);
	}
}



//////////////////////////////
// Image
//////////////////////////////

Image CreateImageInternal(GraphicsDevice &device, u32 width, u32 height, u32 mipLevels, Format format, ImageUsageFlags usage, HeapType heapType)
{
	const VkFormat vkFormat = FormatToVulkan(format);
	// Image
	const VkImageCreateInfo imageCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.flags = 0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = vkFormat,
		.extent = { .width = width, .height = height, .depth = 1, },
		.mipLevels = mipLevels,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = ImageUsageFlagsToVulkan(usage),
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	VkImage imageHandle;
	VK_CALL( vkCreateImage(device.handle, &imageCreateInfo, VULKAN_ALLOCATORS, &imageHandle) );

	// Memory
	VkMemoryRequirements memoryRequirements = {};
	vkGetImageMemoryRequirements(device.handle, imageHandle, &memoryRequirements);

	Heap &heap = device.heaps[heapType];
	const Alloc alloc = Allocate(heap, memoryRequirements.size, memoryRequirements.alignment);

	VK_CALL( vkBindImageMemory(device.handle, imageHandle, heap.memory, alloc.offset) );

	const VkImageView imageViewHandle = CreateImageView(device, imageHandle, FormatToVulkan(format), FormatToVulkanAspect(format), mipLevels);

	const Image image = {
		.handle = imageHandle,
		.imageViewHandle = imageViewHandle,
		.format = format,
		.width = width,
		.height = height,
		.mipLevels = mipLevels,
		.alloc = alloc,
	};
	return image;
}

ImageH CreateImage(GraphicsDevice &device, u32 width, u32 height, u32 mipLevels, Format format, ImageUsageFlags usage, HeapType heapType)
{
	const Image image = CreateImageInternal(device, width, height, mipLevels, format, usage, heapType);
	const ImageH imageH = NewHandle(device.imagePool);
	device.images[GetHandleIndex(device.imagePool, imageH)] = image;
	return imageH;
}

Image &GetImage(GraphicsDevice &device, ImageH imageH)
{
	if ( IsSwapchainImage(imageH) )
	{
		ASSERT(imageH.slot - FIRST_SWAPCHAIN_IMAGE_INDEX < MAX_SWAPCHAIN_IMAGE_COUNT);
		return device.swapchain.images[imageH.slot - FIRST_SWAPCHAIN_IMAGE_INDEX];
	}
	const i32 index = GetElementIndex(device.imagePool, imageH);
	ASSERT(index >= 0);
	return device.images[index];
}

const Image &GetImageConst(const GraphicsDevice &device, ImageH imageH)
{
	if ( IsSwapchainImage(imageH) )
	{
		ASSERT(imageH.slot - FIRST_SWAPCHAIN_IMAGE_INDEX < MAX_SWAPCHAIN_IMAGE_COUNT);
		return device.swapchain.images[imageH.slot - FIRST_SWAPCHAIN_IMAGE_INDEX];
	}
	static const Image sNullImage = {};
	const i32 index = GetElementIndex(device.imagePool, imageH);
	return index >= 0 ? device.images[index] : sNullImage;
}

// Swapchain images use the slots after the image pool
bool IsSwapchainImage(ImageH image)
{
	const bool isSwapchainImage = image.gen != 0 && image.slot >= FIRST_SWAPCHAIN_IMAGE_INDEX;
	return isSwapchainImage;
}

static ImageH SwapchainImageHandle(u32 swapchainImageIndex)
{
	ImageH imageH = {};
	imageH.gen = 1;
	imageH.slot = (u16)(FIRST_SWAPCHAIN_IMAGE_INDEX + swapchainImageIndex);
	return imageH;
}

void DestroyImage(const GraphicsDevice &device, const Image &image)
{
	if ( image.imageViewHandle != VK_NULL_HANDLE )
	{
		DestroyImageView( device, image.imageViewHandle );
	}

	if ( image.handle != VK_NULL_HANDLE )
	{
		vkDestroyImage( device.handle, image.handle, VULKAN_ALLOCATORS );
		Deallocate(image.alloc);
	}
}

void DestroyImageH(GraphicsDevice &device, ImageH imageH)
{
	if ( IsSwapchainImage(imageH) )
	{
		Image &image = GetImage(device, imageH);
		DestroyImage(device, image);
		image.handle = VK_NULL_HANDLE;
		image.imageViewHandle = VK_NULL_HANDLE;
	}
	else if ( IsValid(imageH) )
	{
		FreeHandle(device.imagePool, imageH);
	}
}



//////////////////////////////
// Sampler
//////////////////////////////

Sampler CreateSamplerInternal(const GraphicsDevice &device, const SamplerDesc &desc)
{
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(device.physicalDevice, &properties);

	const bool useLinear = desc.filter == FilterLinear;
	const VkSamplerCreateInfo samplerCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = FilterToVulkan(desc.filter),
		.minFilter = FilterToVulkan(desc.filter),
		.mipmapMode = useLinear ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.addressModeU = AddressModeToVulkan(desc.addressMode),
		.addressModeV = AddressModeToVulkan(desc.addressMode),
		.addressModeW = AddressModeToVulkan(desc.addressMode),
		.mipLodBias = 0.0f,
		.anisotropyEnable = useLinear ? VK_TRUE : VK_FALSE,
		.maxAnisotropy = useLinear ? properties.limits.maxSamplerAnisotropy : 1.0f,
		.compareEnable = desc.compareOp != CompareOpNone, // For PCF shadows for instance
		.compareOp = CompareOpToVulkan(desc.compareOp),
		.minLod = 0.0f,
		.maxLod = VK_LOD_CLAMP_NONE,
		.borderColor = BorderColorToVulkan(desc.borderColor),
		.unnormalizedCoordinates = VK_FALSE,
	};

	VkSampler vkSampler;
	VK_CALL( vkCreateSampler(device.handle, &samplerCreateInfo, VULKAN_ALLOCATORS, &vkSampler) );

	const Sampler sampler = {
		.handle = vkSampler
	};
	return sampler;
}

SamplerH CreateSampler(GraphicsDevice &device, const SamplerDesc &desc)
{
	const Sampler sampler = CreateSamplerInternal(device, desc);
	const SamplerH samplerHandle = NewHandle(device.samplerPool);
	device.samplers[GetHandleIndex(device.samplerPool, samplerHandle)] = sampler;
	return samplerHandle;
}

const Sampler &GetSampler(const GraphicsDevice &device, SamplerH handle)
{
	static const Sampler sNullSampler = {};
	const i32 index = GetElementIndex(device.samplerPool, handle);
	return index >= 0 ? device.samplers[index] : sNullSampler;
}

void DestroySampler(const GraphicsDevice &device, const Sampler &sampler)
{
	vkDestroySampler( device.handle, sampler.handle, VULKAN_ALLOCATORS );
}

void DestroySamplerH(GraphicsDevice &device, SamplerH samplerH)
{
	if ( IsValid(samplerH) ) {
		FreeHandle(device.samplerPool, samplerH);
	}
}



//////////////////////////////
// Pipelines
//////////////////////////////

// TODO: Avoid having to pass renderPass as a parameter instead of within the PipelineDesc
Pipeline CreateGraphicsPipelineInternal(GraphicsDevice &device, Arena scratch, const PipelineDesc &desc, BindGroupLayoutH globalBindGroupLayout)
{
	// Create pipeline
	const ShaderSource vertexShaderSource = desc.vertexShaderSource;
	const ShaderSource fragmentShaderSource = desc.fragmentShaderSource;
	const ShaderModule vertexShaderModule = CreateShaderModule(device, vertexShaderSource);
	const ShaderModule fragmentShaderModule = CreateShaderModule(device, fragmentShaderSource);
	const ShaderBindings shaderBindings = ReflectShaderBindings(scratch, vertexShaderSource, fragmentShaderSource);

	const VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vertexShaderModule.handle,
		.pName = desc.vsFunction ? desc.vsFunction : "main",
	};

	const VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = fragmentShaderModule.handle,
		.pName = desc.fsFunction ? desc.fsFunction : "main",
	};

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo};

	VkVertexInputBindingDescription bindingDescriptions[4] = {};
	for (u32 i = 0; i < desc.vertexBufferCount; ++i)
	{
		bindingDescriptions[i].binding = i;
		bindingDescriptions[i].stride = desc.vertexBuffers[i].stride,
		bindingDescriptions[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	}

	VkVertexInputAttributeDescription attributeDescriptions[4] = {};
	for (u32 i = 0; i < desc.vertexAttributeCount; ++i)
	{
		attributeDescriptions[i].binding = desc.vertexAttributes[i].bufferIndex;
		attributeDescriptions[i].location = desc.vertexAttributes[i].location;
		attributeDescriptions[i].format = FormatToVulkan(desc.vertexAttributes[i].format);
		attributeDescriptions[i].offset = desc.vertexAttributes[i].offset;
	}

	const VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = desc.vertexBufferCount,
		.pVertexBindingDescriptions = bindingDescriptions, // Optional
		.vertexAttributeDescriptionCount = desc.vertexAttributeCount,
		.pVertexAttributeDescriptions = attributeDescriptions, // Optional
	};

	const VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};

	const VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	const VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = ARRAY_COUNT(dynamicStates),
		.pDynamicStates = dynamicStates,
	};

	const VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1,
	};
	// NOTE: We don't set values for the viewport and scissor
	// rect because they will be set dynamically using commands

	const VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f, // Optional
		.depthBiasClamp = 0.0f, // Optional
		.depthBiasSlopeFactor = 0.0f, // Optional
		.lineWidth = 1.0f,
	};

	const VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.0f, // Optional
		.pSampleMask = nullptr, // Optional
		.alphaToCoverageEnable = VK_FALSE, // Optional
		.alphaToOneEnable = VK_FALSE, // Optional
	};

	const VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = desc.depthTest ? VK_TRUE : VK_FALSE,
		.depthWriteEnable = desc.depthWrite ? VK_TRUE : VK_FALSE,
		.depthCompareOp = CompareOpToVulkan(desc.depthCompareOp),
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.front = {}, // Optional
		.back = {}, // Optional
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f,
	};

	const VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
		.blendEnable         = desc.blending || desc.additiveBlending ? VK_TRUE : VK_FALSE,
		.srcColorBlendFactor = desc.additiveBlending ? VK_BLEND_FACTOR_ONE : desc.blending ? VK_BLEND_FACTOR_SRC_ALPHA : VK_BLEND_FACTOR_ONE, // Optional
		.dstColorBlendFactor = desc.additiveBlending ? VK_BLEND_FACTOR_ONE : desc.blending ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ZERO, // Optional
		.colorBlendOp        = VK_BLEND_OP_ADD, // Optional
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
		.alphaBlendOp        = VK_BLEND_OP_ADD, // Optional
		.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};

	const VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY, // Optional
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachmentState,
		.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }, // Optional
	};

	// BindGroup layouts
	const BindGroupLayoutH bindGroupLayouts[MAX_BIND_GROUPS] = {
		globalBindGroupLayout,
		CreateBindGroupLayout(device, shaderBindings.bindings + shaderBindings.bindGroupBindingBase[1], shaderBindings.bindGroupBindingCount[1]),
		CreateBindGroupLayout(device, shaderBindings.bindings + shaderBindings.bindGroupBindingBase[2], shaderBindings.bindGroupBindingCount[2]),
		CreateBindGroupLayout(device, shaderBindings.bindings + shaderBindings.bindGroupBindingBase[3], shaderBindings.bindGroupBindingCount[3]),
	};

	// Descriptor set layouts (same as BindGroup, but Vulkan handles)
	const VkDescriptorSetLayout descriptorSetLayouts[MAX_BIND_GROUPS] = {
		GetBindGroupLayout(device, bindGroupLayouts[0]).handle,
		GetBindGroupLayout(device, bindGroupLayouts[1]).handle,
		GetBindGroupLayout(device, bindGroupLayouts[2]).handle,
		GetBindGroupLayout(device, bindGroupLayouts[3]).handle,
	};

	// Pipeline layout
	const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = ARRAY_COUNT(descriptorSetLayouts),
		.pSetLayouts = descriptorSetLayouts,
	};

	VkPipelineLayout pipelineLayout;
	VK_CALL( vkCreatePipelineLayout(device.handle, &pipelineLayoutCreateInfo, VULKAN_ALLOCATORS, &pipelineLayout) );

	const VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = ARRAY_COUNT(shaderStages),
		.pStages = shaderStages,
		.pVertexInputState = &vertexInputCreateInfo,
		.pInputAssemblyState = &inputAssemblyCreateInfo,
		.pViewportState = &viewportStateCreateInfo,
		.pRasterizationState = &rasterizerCreateInfo,
		.pMultisampleState = &multisamplingCreateInfo,
		.pDepthStencilState = &depthStencilCreateInfo,
		.pColorBlendState = &colorBlendingCreateInfo,
		.pDynamicState = &dynamicStateCreateInfo,
		.layout = pipelineLayout,
		.renderPass = GetRenderPass(device, desc.renderPass).handle,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE, // Optional
		.basePipelineIndex = -1, // Optional
	};

	VkPipeline vkPipelineHandle;
	VK_CALL( vkCreateGraphicsPipelines( device.handle, device.pipelineCache, 1, &graphicsPipelineCreateInfo, VULKAN_ALLOCATORS, &vkPipelineHandle ) );

	DestroyShaderModule(device, vertexShaderModule);
	DestroyShaderModule(device, fragmentShaderModule);

	const Pipeline pipeline = {
		.name = InternStringGfx(desc.name),
		.handle = vkPipelineHandle,
		.layout = {
			.handle = pipelineLayout,
			.bindGroupLayouts = {
				bindGroupLayouts[0],
				bindGroupLayouts[1],
				bindGroupLayouts[2],
				bindGroupLayouts[3],
			},
		},
		.bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
	};

	return pipeline;
}

Pipeline CreateComputePipelineInternal(GraphicsDevice &device, Arena scratch, const ComputeDesc &desc, BindGroupLayoutH globalBindGroupLayout)
{
	const ShaderSource shaderSource = desc.computeShaderSource;
	const ShaderModule shaderModule = CreateShaderModule(device, shaderSource);
	const ShaderBindings shaderBindings = ReflectShaderBindings(scratch, shaderSource);

	const VkPipelineShaderStageCreateInfo computeShaderStageInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		.module = shaderModule.handle,
		.pName = desc.function ? desc.function : "main",
	};

	// BindGroup layouts
	const BindGroupLayoutH bindGroupLayouts[MAX_BIND_GROUPS] = {
		globalBindGroupLayout,
		CreateBindGroupLayout(device, shaderBindings.bindings + shaderBindings.bindGroupBindingBase[1], shaderBindings.bindGroupBindingCount[1]),
		CreateBindGroupLayout(device, shaderBindings.bindings + shaderBindings.bindGroupBindingBase[2], shaderBindings.bindGroupBindingCount[2]),
		CreateBindGroupLayout(device, shaderBindings.bindings + shaderBindings.bindGroupBindingBase[3], shaderBindings.bindGroupBindingCount[3]),
	};

	// Descriptor set layouts (same as BindGroup, but Vulkan handles)
	const VkDescriptorSetLayout descriptorSetLayouts[MAX_BIND_GROUPS] = {
		GetBindGroupLayout(device, bindGroupLayouts[0]).handle,
		GetBindGroupLayout(device, bindGroupLayouts[1]).handle,
		GetBindGroupLayout(device, bindGroupLayouts[2]).handle,
		GetBindGroupLayout(device, bindGroupLayouts[3]).handle,
	};

	// Pipeline layout
	const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = ARRAY_COUNT(descriptorSetLayouts),
		.pSetLayouts = descriptorSetLayouts,
	};

	VkPipelineLayout pipelineLayout;
	VK_CALL( vkCreatePipelineLayout(device.handle, &pipelineLayoutCreateInfo, VULKAN_ALLOCATORS, &pipelineLayout) );

	const VkComputePipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.stage = computeShaderStageInfo,
		.layout = pipelineLayout,
	};

	VkPipeline computePipeline;
	VK_CALL( vkCreateComputePipelines(device.handle, device.pipelineCache, 1, &pipelineInfo, VULKAN_ALLOCATORS, &computePipeline) );

	DestroyShaderModule(device, shaderModule);

	const Pipeline pipeline = {
		.name = InternStringGfx(desc.name),
		.handle = computePipeline,
		.layout = {
			.handle = pipelineLayout,
			.bindGroupLayouts = {
				bindGroupLayouts[0],
				bindGroupLayouts[1],
				bindGroupLayouts[2],
				bindGroupLayouts[3],
			},
		},
		.bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE,
	};

	return pipeline;
}

static Pipeline &NewPipeline(GraphicsDevice &device, PipelineH &outHandle)
{
	outHandle = NewHandle(device.pipelinePool);
	return device.pipelines[GetHandleIndex(device.pipelinePool, outHandle)];
}

PipelineH CreateGraphicsPipeline(GraphicsDevice &device, Arena scratch, const PipelineDesc &desc, BindGroupLayoutH globalBindGroupLayout)
{
	const Pipeline pipeline = CreateGraphicsPipelineInternal(device, scratch, desc, globalBindGroupLayout);
	PipelineH pipelineHandle;
	NewPipeline(device, pipelineHandle) = pipeline;
	return pipelineHandle;
}

PipelineH CreateComputePipeline(GraphicsDevice &device, Arena scratch, const ComputeDesc &desc, BindGroupLayoutH globalBindGroupLayout)
{
	const Pipeline pipeline = CreateComputePipelineInternal(device, scratch, desc, globalBindGroupLayout);
	PipelineH pipelineHandle;
	NewPipeline(device, pipelineHandle) = pipeline;
	return pipelineHandle;
}

const Pipeline &GetPipeline(const GraphicsDevice &device, PipelineH handle)
{
	static const Pipeline sNullPipeline = {};
	const i32 index = GetElementIndex(device.pipelinePool, handle);
	return index >= 0 ? device.pipelines[index] : sNullPipeline;
}

bool IsSamePipeline(PipelineH a, PipelineH b)
{
	const bool isSame = a == b;
	return isSame;
}

void DestroyPipeline(const GraphicsDevice &device, const Pipeline &pipeline)
{
	ASSERT( pipeline.handle != VK_NULL_HANDLE );
	ASSERT( pipeline.layout.handle != VK_NULL_HANDLE );
	vkDestroyPipeline( device.handle, pipeline.handle, VULKAN_ALLOCATORS );
	vkDestroyPipelineLayout( device.handle, pipeline.layout.handle, VULKAN_ALLOCATORS );
}

void DestroyPipelineH(GraphicsDevice &device, PipelineH handle)
{
	if ( IsValid(handle) ) {
		FreeHandle(device.pipelinePool, handle);
	}
}


//////////////////////////////
// RenderPass
//////////////////////////////

RenderPass CreateRenderPassInternal( const GraphicsDevice &device, const RenderpassDesc &desc )
{
	CT_ASSERT(MAX_COLOR_ATTACHMENTS + MAX_DEPTH_ATTACHMENTS == MAX_RENDER_TARGETS);
	ASSERT(desc.colorAttachmentCount <= MAX_COLOR_ATTACHMENTS);

	VkAttachmentDescription attachmentDescs[MAX_COLOR_ATTACHMENTS + MAX_DEPTH_ATTACHMENTS] = {};
	VkAttachmentReference colorAttachmentRefs[MAX_COLOR_ATTACHMENTS] = {};
	VkAttachmentReference depthAttachmentRef = {};

	for (u8 i = 0; i < desc.colorAttachmentCount; ++i)
	{
		attachmentDescs[i].format = FormatToVulkan( desc.colorAttachments[i].format );
		attachmentDescs[i].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescs[i].loadOp = LoadOpToVulkan( desc.colorAttachments[i].loadOp );
		attachmentDescs[i].storeOp = StoreOpToVulkan( desc.colorAttachments[i].storeOp );
		attachmentDescs[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescs[i].initialLayout = desc.colorAttachments[i].loadOp == LoadOpLoad ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescs[i].finalLayout = desc.colorAttachments[i].isSwapchain ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		colorAttachmentRefs[i].attachment = i;
		colorAttachmentRefs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	if ( desc.hasDepthAttachment )
	{
		const u8 depthAttachmentIndex = desc.colorAttachmentCount;
		VkAttachmentDescription &depthAttachmentDesc = attachmentDescs[depthAttachmentIndex];
		depthAttachmentDesc.format = FormatToVulkan(device.caps.defaultDepthFormat);
		depthAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachmentDesc.loadOp = LoadOpToVulkan(desc.depthAttachment.loadOp);
		depthAttachmentDesc.storeOp = StoreOpToVulkan(desc.depthAttachment.storeOp);
		depthAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachmentDesc.initialLayout = desc.depthAttachment.loadOp == LoadOpLoad ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		depthAttachmentRef.attachment = depthAttachmentIndex;;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	const VkSubpassDescription subpassDesc = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = desc.colorAttachmentCount,
		.pColorAttachments = desc.colorAttachmentCount ? colorAttachmentRefs : NULL,
		.pDepthStencilAttachment = desc.hasDepthAttachment ? &depthAttachmentRef : NULL,
	};

	const VkSubpassDependency subpassDependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		.dependencyFlags = 0,
	};

	const VkRenderPassCreateInfo renderPassCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = desc.colorAttachmentCount + (desc.hasDepthAttachment ? 1U : 0U),
		.pAttachments = attachmentDescs,
		.subpassCount = 1,
		.pSubpasses = &subpassDesc,
		.dependencyCount = 1,
		.pDependencies = &subpassDependency,
	};

	VkRenderPass vkRenderPass;
	VK_CALL( vkCreateRenderPass( device.handle, &renderPassCreateInfo, VULKAN_ALLOCATORS, &vkRenderPass ) );

	const RenderPass renderPass = {
		.name = InternStringGfx(desc.name),
		.handle = vkRenderPass
	};

	return renderPass;
}

RenderPassH CreateRenderPass( GraphicsDevice &device, const RenderpassDesc &desc )
{
	const RenderPass renderPass = CreateRenderPassInternal(device, desc);
	const RenderPassH renderPassH = NewHandle(device.renderPassPool);
	device.renderPasses[GetHandleIndex(device.renderPassPool, renderPassH)] = renderPass;
	return renderPassH;
}

const RenderPass &GetRenderPass(const GraphicsDevice &device, RenderPassH handle)
{
	static const RenderPass sNullRenderPass = {};
	const i32 index = GetElementIndex(device.renderPassPool, handle);
	return index >= 0 ? device.renderPasses[index] : sNullRenderPass;
}

void DestroyRenderPass(const GraphicsDevice &device, const RenderPass &renderPass)
{
	vkDestroyRenderPass( device.handle, renderPass.handle, VULKAN_ALLOCATORS );
}

void DestroyRenderPassH(GraphicsDevice &device, RenderPassH renderPassH)
{
	if ( IsValid(renderPassH) ) {
		FreeHandle(device.renderPassPool, renderPassH);
	}
}


//////////////////////////////
// Framebuffer
//////////////////////////////

static const Framebuffer &GetFramebuffer(const GraphicsDevice &device, FramebufferH handle)
{
	static const Framebuffer sNullFramebuffer = {};
	const i32 index = GetElementIndex(device.framebufferPool, handle);
	return index >= 0 ? device.framebuffers[index] : sNullFramebuffer;
}

FramebufferH CreateFramebuffer(GraphicsDevice &device, const FramebufferDesc &desc)
{
	ASSERT(desc.attachmentCount > 0 && desc.attachmentCount < MAX_COLOR_ATTACHMENTS + MAX_DEPTH_ATTACHMENTS);

	u32 width = 0;
	u32 height = 0;

	VkImageView attachments[MAX_COLOR_ATTACHMENTS + MAX_DEPTH_ATTACHMENTS] = {};
	for (u32 i = 0; i < desc.attachmentCount; ++i)
	{
		const Image &image = GetImageConst(device, desc.attachments[i]);
		attachments[i] = image.imageViewHandle;

		if ( i == 0 )
		{
			width = image.width;
			height = image.height;
		}
		else
		{
			ASSERT(width == image.width && height == image.height);
		}
	}

	const RenderPass &renderPass = GetRenderPass(device, desc.renderPass);

	const VkFramebufferCreateInfo framebufferCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = renderPass.handle,
		.attachmentCount = desc.attachmentCount,
		.pAttachments = attachments,
		.width = width,
		.height = height,
		.layers = 1,
	};

	VkFramebuffer handle;
	VK_CALL( vkCreateFramebuffer( device.handle, &framebufferCreateInfo, VULKAN_ALLOCATORS, &handle) );

	const FramebufferH framebufferH = NewHandle(device.framebufferPool);
	device.framebuffers[GetHandleIndex(device.framebufferPool, framebufferH)] = {
		.handle = handle,
		.renderPassHandle = renderPass.handle,
		.extent = { width, height },
		.attachmentCount = desc.attachmentCount,
	};
	return framebufferH;
}

void DestroyFramebuffer(GraphicsDevice &device, FramebufferH framebufferH)
{
	if ( IsValid(framebufferH) ) {
		FreeHandle(device.framebufferPool, framebufferH);
	}
}

uint2 GetFramebufferSize(const GraphicsDevice &device, FramebufferH framebufferH)
{
	const Framebuffer &framebuffer = GetFramebuffer(device, framebufferH);
	return { framebuffer.extent.width, framebuffer.extent.height };
}


//////////////////////////////
// CommandList
//////////////////////////////

static CommandList AllocCommandList(GraphicsDevice &device, VkCommandBuffer commandBuffer, bool transient)
{
	u32 index = 1; // 0 unused
	while ( index < MAX_COMMAND_LISTS && device.commandLists[index].inUse ) {
		index++;
	}
	ASSERT(index < MAX_COMMAND_LISTS && "Too many command lists recording at the same time");

	device.commandLists[index] = {
		.inUse = true,
		.handle = commandBuffer,
		.transient = transient,
	};
	const CommandList commandList = {
		.device = &device,
		.index = index,
	};
	return commandList;
}

static void FreeCommandList(const CommandList &commandList)
{
	GetState(commandList) = {};
}

// Released by Submit
CommandList BeginCommandList(GraphicsDevice &device)
{
	VkCommandBuffer commandBuffer = device.commandBuffers[device.frameSlot];

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = 0; // Optional
	commandBufferBeginInfo.pInheritanceInfo = NULL; // Optional
	VK_CALL( vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) );

	return AllocCommandList(device, commandBuffer, false);
}

void EndCommandList(const CommandList &commandList)
{
	VK_CALL( vkEndCommandBuffer( GetState(commandList).handle ) );
}

// Released by EndTransientCommandList
CommandList BeginTransientCommandList(GraphicsDevice &device)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = device.transientCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device.handle, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return AllocCommandList(device, commandBuffer, true);
}

void EndTransientCommandList(GraphicsDevice &device, const CommandList &commandList)
{
	ASSERT(GetState(commandList).transient);
	VkCommandBuffer commandBuffer = GetState(commandList).handle;

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(device.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

	WaitQueueIdle(device);

	vkFreeCommandBuffers(device.handle, device.transientCommandPool, 1, &commandBuffer);

	FreeCommandList(commandList);
}



//////////////////////////////
// Commands
//////////////////////////////

void *GetBufferPtr(const GraphicsDevice &device, BufferH bufferH)
{
	const Buffer &buffer = GetBufferConst(device, bufferH);
	const Heap &heap = device.heaps[buffer.alloc.heap];

	if (heap.data) // Check if the heap memory is mapped
	{
		void *ptr = heap.data + buffer.alloc.offset;
		return ptr;
	}
	else
	{
		return 0;
	}
}

void CopyBufferToBuffer(const CommandList &commandBuffer, BufferH srcBufferH, u32 srcOffset, BufferH dstBufferH, u32 dstOffset, u64 size)
{
	const GraphicsDevice &device = GetDevice(commandBuffer);
	const Buffer &srcBuffer = GetBufferConst(device, srcBufferH);
	const Buffer &dstBuffer = GetBufferConst(device, dstBufferH);

	ASSERT(srcOffset + size <= srcBuffer.size);
	ASSERT(dstOffset + size <= dstBuffer.size);

	const VkBufferCopy copyRegion = {
		srcOffset = srcOffset,
		dstOffset = dstOffset,
		size = size,
	};
	vkCmdCopyBuffer(GetState(commandBuffer).handle, srcBuffer.handle, dstBuffer.handle, 1, &copyRegion);
}

void CopyBufferToImage(const CommandList &commandBuffer, BufferH bufferH, u32 bufferOffset, ImageH imageH)
{
	const GraphicsDevice &device = GetDevice(commandBuffer);
	const Buffer &buffer = GetBufferConst(device, bufferH);
	const Image &image = GetImageConst(device, imageH);

	const VkBufferImageCopy region = {
		.bufferOffset = bufferOffset,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = {
			.aspectMask = FormatToVulkanAspect(image.format),
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.imageOffset = {0, 0, 0},
		.imageExtent = { image.width, image.height, 1 },
	};

	vkCmdCopyBufferToImage(
			GetState(commandBuffer).handle,
			buffer.handle,
			image.handle,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // Assuming the right layout
			1,
			&region
			);
}

void Blit(const CommandList &commandBuffer, ImageH srcImageH, const BlitRegion &srcRegion, ImageH dstImageH, const BlitRegion &dstRegion)
{
	const GraphicsDevice &device = GetDevice(commandBuffer);
	const Image &srcImage = GetImageConst(device, srcImageH);
	const Image &dstImage = GetImageConst(device, dstImageH);
	const VkImageAspectFlags aspectMask = FormatToVulkanAspect(srcImage.format);

	const VkImageBlit blit = {
		.srcSubresource = {
			.aspectMask = aspectMask,
			.mipLevel = srcRegion.mipLevel,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.srcOffsets = {
			{ srcRegion.x, srcRegion.y, 0u }, // lower bound
			{ srcRegion.x + srcRegion.width, srcRegion.y + srcRegion.height, 1u }, // upper bound
		},
		.dstSubresource = {
			.aspectMask = aspectMask,
			.mipLevel = dstRegion.mipLevel,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.dstOffsets = {
			{ dstRegion.x, dstRegion.y, 0 }, // lower bound
			{ dstRegion.x + dstRegion.width, dstRegion.y + dstRegion.height, 1 }, // upper bound
		},
	};

	vkCmdBlitImage(GetState(commandBuffer).handle,
			srcImage.handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // Assuming the proper layout
			dstImage.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // Assuming the proper layout
			1, &blit,
			VK_FILTER_LINEAR); // Assuming linear filtering
}

void TransitionImageLayout(const CommandList &commandBuffer, ImageH imageH, ImageState oldState, ImageState newState, u32 baseMipLevel, u32 levelCount)
{
	const Image &image = GetImageConst(GetDevice(commandBuffer), imageH);

	const bool isDepth = IsDepthFormat(image.format);

	VkAccessFlags srcAccess = 0;
	VkAccessFlags dstAccess = 0;
	VkPipelineStageFlags srcStage = 0;
	VkPipelineStageFlags dstStage = 0;
	VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout newLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	switch (oldState)
	{
		case ImageStateInitial: oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; break;
		case ImageStateTransferSrc: oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; break;
		case ImageStateTransferDst: oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; break;
		case ImageStateShaderInput: oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
		case ImageStateRenderTarget: oldLayout = isDepth
			? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			: VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			break;
		default: INVALID_CODE_PATH();
	}

	switch (newState)
	{
		case ImageStateInitial: newLayout = VK_IMAGE_LAYOUT_UNDEFINED; break;
		case ImageStateTransferSrc: newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; break;
		case ImageStateTransferDst: newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; break;
		case ImageStateShaderInput: newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
		case ImageStateRenderTarget: newLayout = isDepth
			? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			: VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			break;
		default: INVALID_CODE_PATH();
	}

	switch (oldLayout)
	{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			srcAccess = 0;
			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			srcAccess = VK_ACCESS_TRANSFER_READ_BIT;
			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			srcAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			srcAccess = VK_ACCESS_SHADER_READ_BIT;
			srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			srcAccess = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			srcAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			break;
		default:
			INVALID_CODE_PATH();
	}

	switch (newLayout)
	{
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			dstAccess = VK_ACCESS_TRANSFER_READ_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			dstAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			dstAccess = VK_ACCESS_SHADER_READ_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			dstAccess = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			dstAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			break;
		default:
			INVALID_CODE_PATH();
	}

	const VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = srcAccess,
		.dstAccessMask = dstAccess,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image.handle,
		.subresourceRange = {
			.aspectMask = FormatToVulkanAspect(image.format),
			.baseMipLevel = baseMipLevel,
			.levelCount = levelCount,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};

	vkCmdPipelineBarrier(GetState(commandBuffer).handle,
		srcStage,
		dstStage,
		0,          // 0 or VK_DEPENDENCY_BY_REGION_BIT
		0, NULL,    // Memory barriers
		0, NULL,    // Buffer barriers
		1, &barrier // Image barriers
		);
}

static void BufferStateToVulkan(BufferState state, VkAccessFlags &access, VkPipelineStageFlags &stage)
{
	switch (state)
	{
		case BufferStateInitial:
			access = 0;
			stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			break;
		case BufferStateTransferSrc:
			access = VK_ACCESS_TRANSFER_READ_BIT;
			stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case BufferStateTransferDst:
			access = VK_ACCESS_TRANSFER_WRITE_BIT;
			stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case BufferStateVertexInput:
			access = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			stage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
			break;
		case BufferStateIndexInput:
			access = VK_ACCESS_INDEX_READ_BIT;
			stage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
			break;
		case BufferStateIndirectArgs:
			access = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
			stage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
			break;
		case BufferStateVertexRead:
			access = VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
			stage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
			break;
		case BufferStateFragmentRead:
			access = VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
			stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;
		case BufferStateComputeRead:
			access = VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
			stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		case BufferStateComputeWrite:
			// Storage buffers are bound read-write, so a write state covers reads too
			access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		default:
			INVALID_CODE_PATH();
	}
}

void TransitionBufferState(const CommandList &commandBuffer, BufferH bufferH, BufferState oldState, BufferState newState)
{
	ASSERT(newState != BufferStateInitial);

	const Buffer &buffer = GetBufferConst(GetDevice(commandBuffer), bufferH);

	VkAccessFlags srcAccess = 0;
	VkAccessFlags dstAccess = 0;
	VkPipelineStageFlags srcStage = 0;
	VkPipelineStageFlags dstStage = 0;

	BufferStateToVulkan(oldState, srcAccess, srcStage);
	BufferStateToVulkan(newState, dstAccess, dstStage);

	const VkBufferMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.srcAccessMask = srcAccess,
		.dstAccessMask = dstAccess,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.buffer = buffer.handle,
		.offset = 0,
		.size = VK_WHOLE_SIZE,
	};

	vkCmdPipelineBarrier(GetState(commandBuffer).handle,
		srcStage,
		dstStage,
		0,           // 0 or VK_DEPENDENCY_BY_REGION_BIT
		0, NULL,     // Memory barriers
		1, &barrier, // Buffer barriers
		0, NULL      // Image barriers
		);
}

void BeginRenderPass(const CommandList &commandList, FramebufferH framebufferH)
{
	const Framebuffer &framebuffer = GetFramebuffer(GetDevice(commandList), framebufferH);
	const CommandListState &state = GetState(commandList);

	const VkRenderPassBeginInfo renderPassBeginInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = framebuffer.renderPassHandle,
		.framebuffer = framebuffer.handle,
		.renderArea = {
			.offset = { 0, 0 },
			.extent = framebuffer.extent
		},
		.clearValueCount = framebuffer.attachmentCount,
		.pClearValues = state.clearValues,
	};

	vkCmdBeginRenderPass( state.handle, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
}

void SetViewport(const CommandList &commandList, rect r)
{
	const VkViewport viewport = {
		.x = static_cast<float>(r.pos.x),
		.y = static_cast<float>(r.size.y + r.pos.y),
		.width = static_cast<float>(r.size.x),
		.height = -static_cast<float>(r.size.y),
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	vkCmdSetViewport(GetState(commandList).handle, 0, 1, &viewport);
}

void SetScissor(const CommandList &commandList, rect r)
{
	const int2 p0 = Max(r.pos, {0, 0});
	const int2 windowSize = { I32_MAX, I32_MAX };
	const int2 p1 = Min(r.pos + r.size, windowSize );
	const int2 size = p1 - p0;

	const VkRect2D scissor = {
		.offset = {p0.x, p0.y},
		.extent = {(u32)size.x, (u32)size.y},
	};
	vkCmdSetScissor(GetState(commandList).handle, 0, 1, &scissor);
}

void SetViewportAndScissor(const CommandList &commandList, uint2 size)
{
	SetViewport(commandList, rect{0, 0, size.x, size.y});

	SetScissor(commandList, rect{0, 0, size.x, size.y});
}

void SetClearColorFloat4(const CommandList &commandList, u32 renderTargetIndex, float4 color)
{
	ASSERT(renderTargetIndex < MAX_RENDER_TARGETS);
	GetState(commandList).clearValues[renderTargetIndex].color.float32[0] = color.r;
	GetState(commandList).clearValues[renderTargetIndex].color.float32[1] = color.g;
	GetState(commandList).clearValues[renderTargetIndex].color.float32[2] = color.b;
	GetState(commandList).clearValues[renderTargetIndex].color.float32[3] = color.a;
}

void SetClearColorU32(const CommandList &commandList, u32 renderTargetIndex, u32 color)
{
	ASSERT(renderTargetIndex < MAX_RENDER_TARGETS);
	GetState(commandList).clearValues[renderTargetIndex].color.uint32[0] = color;
}

void SetClearDepth(const CommandList &commandList, u32 renderTargetIndex, float depth)
{
	ASSERT(renderTargetIndex < MAX_RENDER_TARGETS);
	GetState(commandList).clearValues[renderTargetIndex].depthStencil.depth = depth;
}

void SetClearStencil(const CommandList &commandList, u32 renderTargetIndex, u32 stencil)
{
	ASSERT(renderTargetIndex < MAX_RENDER_TARGETS);
	GetState(commandList).clearValues[renderTargetIndex].depthStencil.stencil = stencil;
}

void SetPipeline(const CommandList &commandList, PipelineH pipelineH)
{
	CommandListState &state = GetState(commandList);

	if ( !IsSamePipeline(state.pipeline, pipelineH) )
	{
		state.pipeline = pipelineH;
		const Pipeline &pipeline = GetPipeline(GetDevice(commandList), pipelineH);

		// Descriptor sets are bound per bind point, but the handles cached here are not. A
		// graphics pass following a dispatch would ask for the same set it just used, see no
		// change, and never bind it for graphics. Forget them so the next SetBindGroup binds.
		if ( state.bindPoint != pipeline.bindPoint )
		{
			state.bindPoint = pipeline.bindPoint;
			MemSet(state.descriptorSetHandles, sizeof(state.descriptorSetHandles), 0);
			state.descriptorSetDirtyMask = 0;
		}

		vkCmdBindPipeline( state.handle, pipeline.bindPoint, pipeline.handle );
	}
}

void SetBindGroup(const CommandList &commandList, u32 bindGroupIndex, BindGroupH bindGroupH)
{
	CommandListState &state = GetState(commandList);
	const i32 index = GetElementIndex(GetDevice(commandList).bindGroupPool, bindGroupH);
	const VkDescriptorSet descriptorSet = index >= 0 ? GetDevice(commandList).bindGroups[index].handle : VK_NULL_HANDLE;

	if ( state.descriptorSetHandles[bindGroupIndex] != descriptorSet )
	{
		state.descriptorSetDirtyMask |= 1 << bindGroupIndex;
		state.descriptorSetHandles[bindGroupIndex] = descriptorSet;
	}
}

void SetVertexBuffer(const CommandList &commandList, BufferH bufferH)
{
	const Buffer &buffer = GetBufferConst(GetDevice(commandList), bufferH);

	if ( buffer.handle && buffer.handle != GetState(commandList).vertexBufferHandle )
	{
		GetState(commandList).vertexBufferHandle = buffer.handle;
		VkBuffer vertexBuffers[] = { buffer.handle };
		VkDeviceSize vertexBufferOffsets[] = { 0 };
		vkCmdBindVertexBuffers(GetState(commandList).handle, 0, ARRAY_COUNT(vertexBuffers), vertexBuffers, vertexBufferOffsets);
	}
}

void SetIndexBuffer(const CommandList &commandList, BufferH bufferH)
{
	const Buffer &buffer = GetBufferConst(GetDevice(commandList), bufferH);

	if ( buffer.handle && buffer.handle != GetState(commandList).indexBufferHandle )
	{
		GetState(commandList).indexBufferHandle = buffer.handle;
		VkDeviceSize indexBufferOffset = 0;
		vkCmdBindIndexBuffer(GetState(commandList).handle, buffer.handle, indexBufferOffset, VK_INDEX_TYPE_UINT16);
	}
}

void Draw(const CommandList &commandList, u32 vertexCount, u32 firstVertex)
{
	BindDescriptorSets(commandList);

	vkCmdDraw(GetState(commandList).handle, vertexCount, 1, firstVertex, 0);
}

void DrawIndexed(const CommandList &commandList, u32 indexCount, u32 firstIndex, u32 firstVertex, u32 firstInstance, u32 instanceCount)
{
	BindDescriptorSets(commandList);

	vkCmdDrawIndexed(GetState(commandList).handle, indexCount, instanceCount, firstIndex, firstVertex, firstInstance);
}

void Dispatch(const CommandList &commandList, u32 x, u32 y, u32 z)
{
	BindDescriptorSets(commandList);

	vkCmdDispatch(GetState(commandList).handle, x, y, z);
}

void EndRenderPass(const CommandList &commandList)
{
	vkCmdEndRenderPass(GetState(commandList).handle);
}



//////////////////////////////
// Timestamp queries
//////////////////////////////

static TimestampPool &GetTimestampPool(GraphicsDevice &device, TimestampPoolH handle)
{
	const i32 index = GetElementIndex(device.timestampPoolPool, handle);
	ASSERT(index >= 0);
	return device.timestampPools[index];
}

static const TimestampPool &GetTimestampPoolConst(const GraphicsDevice &device, TimestampPoolH handle)
{
	const i32 index = GetElementIndex(device.timestampPoolPool, handle);
	ASSERT(index >= 0);
	return device.timestampPools[index];
}

TimestampPoolH CreateTimestampPool(GraphicsDevice &device, u32 maxQueries)
{
#if DEVELOPMENT_BUILD
	const VkQueryPoolCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
		.queryType = VK_QUERY_TYPE_TIMESTAMP,
		.queryCount = maxQueries,
	};

	VkQueryPool poolHandle;
	VK_CALL( vkCreateQueryPool(device.handle, &createInfo, VULKAN_ALLOCATORS, &poolHandle) );

	const TimestampPoolH poolH = NewHandle(device.timestampPoolPool);
	GetTimestampPool(device, poolH) = {
		.handle = poolHandle,
		.queryCount = 0,
		.maxQueries = maxQueries,
	};
	return poolH;
#else
	return {};
#endif
}

void DestroyTimestampPool(GraphicsDevice &device, TimestampPoolH poolH)
{
	if ( IsValid(poolH) ) {
		FreeHandle(device.timestampPoolPool, poolH);
	}
}

//void ResetTimestampPool(const GraphicsDevice &device, TimestampPool &pool)
//{
//	// Vulkan 1.2
//	vkResetQueryPool(device.handle, pool.handle, 0, pool.maxQueries);
//	pool.queryCount = 0;
//}

void ResetTimestampPool(const CommandList &commandBuffer, TimestampPoolH poolH)
{
#if DEVELOPMENT_BUILD
	if ( !IsValid(poolH) ) { return; }

	TimestampPool &pool = GetTimestampPool(GetDevice(commandBuffer), poolH);
	vkCmdResetQueryPool(GetState(commandBuffer).handle, pool.handle, 0, pool.maxQueries);
	pool.queryCount = 0;
#endif
}

u32 WriteTimestamp(const CommandList &commandBuffer, TimestampPoolH poolH, PipelineStage stage)
{
#if DEVELOPMENT_BUILD
	if ( !IsValid(poolH) ) { return 0; }

	TimestampPool &pool = GetTimestampPool(GetDevice(commandBuffer), poolH);
	ASSERT(pool.queryCount < pool.maxQueries);
	const VkPipelineStageFlagBits vkState = PipelineStageToVulkan(stage);
	const u32 queryIndex = pool.queryCount++;
	vkCmdWriteTimestamp(GetState(commandBuffer).handle, vkState, pool.handle, queryIndex);
	return queryIndex;
#else
	return 0;
#endif
}

Timestamp ReadTimestamp(const GraphicsDevice &device, TimestampPoolH poolH, u32 queryIndex)
{
#if DEVELOPMENT_BUILD
	double millis = 0.0f;

	if ( IsValid(poolH) && queryIndex < GetTimestampPoolConst(device, poolH).queryCount )
	{
		const TimestampPool &pool = GetTimestampPoolConst(device, poolH);
		u64 data[2];
		const VkQueryResultFlags flags = VK_QUERY_RESULT_WITH_AVAILABILITY_BIT | VK_QUERY_RESULT_64_BIT;
		const VkDeviceSize stride = sizeof(data);
		const u32 queryCount = 1;
		VkResult result = vkGetQueryPoolResults(device.handle, pool.handle, queryIndex, queryCount, sizeof(data), data, stride, flags);

		// Exit with error
		if (result < VK_SUCCESS) {
			CheckVulkanResult(result, "vkGetQueryPoolResults");
		}

		// result could be VK_NOT_READY
		const bool isAvailable = result == VK_SUCCESS && data[1] != 0;
		ASSERT(isAvailable);

		const u64 ticks = isAvailable ? data[0] : 0;
		const double nanos = (double)ticks * device.limits.timestampPeriod;
		millis = nanos / 1000000.0;
	}

	const Timestamp timestamp = {
		.millis = millis,
	};
	return timestamp;
#else
	return {};
#endif
}

// Reads a range of queries at once, which is what a profiler wants when resolving a whole frame.
// Timestamps are only readable once the GPU is done with the commands that wrote them (normally
// MAX_FRAMES_IN_FLIGHT frames later), so this returns false, writing nothing, when any query in
// the range is not available yet. The caller is expected to retry (or drop the frame) in that case.
bool ReadTimestamps(const GraphicsDevice &device, TimestampPoolH poolH, u32 firstQuery, u32 queryCount, Timestamp *outTimestamps)
{
#if DEVELOPMENT_BUILD
	if ( !IsValid(poolH) ) {
		return false;
	}

	const TimestampPool &pool = GetTimestampPoolConst(device, poolH);
	if ( firstQuery + queryCount > pool.queryCount ) {
		return false;
	}

	constexpr u32 maxBatchCount = 64;
	u64 data[maxBatchCount * 2]; // Pairs of (ticks, availability)

	const VkQueryResultFlags flags = VK_QUERY_RESULT_WITH_AVAILABILITY_BIT | VK_QUERY_RESULT_64_BIT;
	const VkDeviceSize stride = 2 * sizeof(u64);

	for (u32 i = 0; i < queryCount; i += maxBatchCount)
	{
		const u32 batchCount = Min(maxBatchCount, queryCount - i);
		const VkDeviceSize dataSize = batchCount * stride;
		VkResult result = vkGetQueryPoolResults(device.handle, pool.handle, firstQuery + i, batchCount, dataSize, data, stride, flags);

		// Exit with error
		if (result < VK_SUCCESS) {
			CheckVulkanResult(result, "vkGetQueryPoolResults");
		}

		// result could be VK_NOT_READY, but availability is checked per query anyway
		for (u32 j = 0; j < batchCount; ++j)
		{
			if ( data[2*j + 1] == 0 ) {
				return false;
			}
			const double nanos = (double)data[2*j] * device.limits.timestampPeriod;
			outTimestamps[i + j].millis = nanos / 1000000.0;
		}
	}

	return true;
#else
	return false;
#endif
}



//////////////////////////////
// Submission and Presentation
//////////////////////////////

// Releases the command list
void Submit(GraphicsDevice &device, const CommandList &commandList)
{
	ASSERT(!GetState(commandList).transient);

	VkSemaphore waitSemaphores[] = { device.imageAcquiredSemaphores[device.presentationIndex] };
	VkSemaphore signalSemaphores[] = { device.renderFinishedSemaphores[device.presentationIndex] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	const VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = ARRAY_COUNT(waitSemaphores),
		.pWaitSemaphores = waitSemaphores,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &GetState(commandList).handle,
		.signalSemaphoreCount = ARRAY_COUNT(signalSemaphores),
		.pSignalSemaphores = signalSemaphores,
	};

	ASSERT(device.usedFenceCount < MAX_FENCES);
	device.usedFenceCount++;

	GraphicsDevice::FrameData &frameData = device.frameData[device.frameSlot];
	const u32 fenceIndex = ( frameData.firstFenceIndex + frameData.usedFenceCount ) % MAX_FENCES;
	frameData.usedFenceCount++;

	VK_CALL( vkQueueSubmit( device.graphicsQueue, 1, &submitInfo, device.fences[fenceIndex] ) );

	device.submitSignalSemaphore = signalSemaphores[0];

	FreeCommandList(commandList);
}

bool Present(GraphicsDevice &device)
{
	const VkSemaphore waitSemaphores[] = { device.submitSignalSemaphore };

	const VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = ARRAY_COUNT(waitSemaphores),
		.pWaitSemaphores = waitSemaphores,
		.swapchainCount = 1,
		.pSwapchains = &device.swapchain.handle,
		.pImageIndices = &device.swapchain.currentImageIndex,
		.pResults = NULL, // Optional
	};

	VkResult presentResult = vkQueuePresentKHR( device.presentQueue, &presentInfo );

	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
	{
		LOG(Debug, "vkQueuePresentKHR - result: VK_ERROR_OUT_OF_DATE_KHR || VK_SUBOPTIMAL_KHR\n");
		device.swapchain.valid = false;
	}
	else if (presentResult != VK_SUCCESS)
	{
		LOG(Error, "vkQueuePresentKHR failed.\n");
		return false;
	}

	return true;
}



//////////////////////////////
// Frame
//////////////////////////////

//////////////////////////////
// Resource pools
//////////////////////////////

typedef void RetireElementFunc(GraphicsDevice &device, const void *element);

static void RetireBindGroup(GraphicsDevice &device, const void *element)
{
	const BindGroup &e = *(const BindGroup*)element;
	if ( e.handle ) {
		VK_CALL( vkFreeDescriptorSets(device.handle, e.descriptorPool, 1, &e.handle) );
	}
}

static void RetireBindGroupAllocator(GraphicsDevice &device, const void *element)
{
	const BindGroupAllocator &e = *(const BindGroupAllocator*)element;
	vkDestroyDescriptorPool(device.handle, e.handle, VULKAN_ALLOCATORS);
}

static void RetirePipeline(GraphicsDevice &device, const void *element)
{
	DestroyPipeline(device, *(const Pipeline*)element);
}

static void RetireFramebuffer(GraphicsDevice &device, const void *element)
{
	const Framebuffer &e = *(const Framebuffer*)element;
	vkDestroyFramebuffer(device.handle, e.handle, VULKAN_ALLOCATORS);
}

static void RetireRenderPass(GraphicsDevice &device, const void *element)
{
	DestroyRenderPass(device, *(const RenderPass*)element);
}

static void RetireBufferView(GraphicsDevice &device, const void *element)
{
	DestroyBufferView(device, *(const BufferView*)element);
}

static void RetireBuffer(GraphicsDevice &device, const void *element)
{
	DestroyBuffer(device, *(const Buffer*)element);
}

static void RetireImage(GraphicsDevice &device, const void *element)
{
	DestroyImage(device, *(const Image*)element);
}

static void RetireSampler(GraphicsDevice &device, const void *element)
{
	DestroySampler(device, *(const Sampler*)element);
}

static void RetireTimestampPool(GraphicsDevice &device, const void *element)
{
#if DEVELOPMENT_BUILD
	const TimestampPool &e = *(const TimestampPool*)element;
	vkDestroyQueryPool(device.handle, e.handle, VULKAN_ALLOCATORS);
#endif
}

struct PoolCompaction
{
	GraphicsDevice *device;
	byte *elements;
	u32 elementSize;
	RetireElementFunc *retire;
};

static MOVE_ELEMENT(MovePoolElement)
{
	const PoolCompaction &c = *(const PoolCompaction*)data;
	MemCopy(c.elements + dstIndex * c.elementSize, c.elements + srcIndex * c.elementSize, c.elementSize);
}

static REMOVE_ELEMENT(RemovePoolElement)
{
	const PoolCompaction &c = *(const PoolCompaction*)data;
	c.retire(*c.device, c.elements + index * c.elementSize);
}

static void CompactResourcePool(GraphicsDevice &device, HandlePool &pool, void *elements, u32 elementSize, RetireElementFunc *retire)
{
	const u16 oldCount = HandleCount(pool);
	const PoolCompaction compaction = { &device, (byte*)elements, elementSize, retire };
	CompactPool(pool, MovePoolElement, RemovePoolElement, (void*)&compaction);

	const u16 newCount = HandleCount(pool);
	MemSet(compaction.elements + newCount * elementSize, (oldCount - newCount) * elementSize, 0);
}

#define COMPACT_RESOURCE_POOL(device, pool, array, retire) \
	CompactResourcePool(device, device.pool, device.array, sizeof(device.array[0]), retire)

// Users before what they use
static void CompactResourcePools(GraphicsDevice &device)
{
	COMPACT_RESOURCE_POOL(device, bindGroupPool, bindGroups, RetireBindGroup);
	COMPACT_RESOURCE_POOL(device, bindGroupAllocatorPool, bindGroupAllocators, RetireBindGroupAllocator);
	COMPACT_RESOURCE_POOL(device, pipelinePool, pipelines, RetirePipeline);
	COMPACT_RESOURCE_POOL(device, framebufferPool, framebuffers, RetireFramebuffer);
	COMPACT_RESOURCE_POOL(device, renderPassPool, renderPasses, RetireRenderPass);
	COMPACT_RESOURCE_POOL(device, bufferViewPool, bufferViews, RetireBufferView);
	COMPACT_RESOURCE_POOL(device, bufferPool, buffers, RetireBuffer);
	COMPACT_RESOURCE_POOL(device, imagePool, images, RetireImage);
	COMPACT_RESOURCE_POOL(device, samplerPool, samplers, RetireSampler);
	COMPACT_RESOURCE_POOL(device, timestampPoolPool, timestampPools, RetireTimestampPool);
}


void WaitFrameFences(GraphicsDevice &device)
{
	// Catch-up frame fences
	VkFence fences[MAX_FENCES] = {};
	GraphicsDevice::FrameData &frameData = device.frameData[device.frameSlot];

	if ( frameData.usedFenceCount > 0 )
	{
		// Populate from ring buffer to linear buffer
		for (u32 i = 0; i < frameData.usedFenceCount; ++i)
		{
			const u32 fenceIndex = ( frameData.firstFenceIndex + i ) % MAX_FENCES;
			fences[i] = device.fences[fenceIndex];
		}

		// Fence wait/lock
		VK_CALL( vkWaitForFences( device.handle, frameData.usedFenceCount, fences, VK_TRUE, UINT64_MAX ) );

		// Reset this frame fences
		VK_CALL( vkResetFences( device.handle, frameData.usedFenceCount, fences ) );

		// Advance global fence ring tail
		device.firstFenceIndex = ( device.firstFenceIndex + frameData.usedFenceCount ) % MAX_FENCES;
		ASSERT(device.usedFenceCount >= frameData.usedFenceCount);
		device.usedFenceCount -= frameData.usedFenceCount;
	}

	// Make this frame fences point to the head of the global fence ring buffer
	frameData.firstFenceIndex = ( device.firstFenceIndex + device.usedFenceCount ) % MAX_FENCES;
	frameData.usedFenceCount = 0;
}

bool AcquireSwapchainImage(GraphicsDevice &device)
{
	const VkFence fence = device.imageAcquiredFences[device.presentationIndex];

	u32 imageIndex;
	VkResult acquireResult = vkAcquireNextImageKHR( device.handle, device.swapchain.handle, UINT64_MAX, device.imageAcquiredSemaphores[device.presentationIndex], fence, &imageIndex );

	if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		LOG(Warning, "vkAcquireNextImageKHR - result: VK_ERROR_OUT_OF_DATE_KHR\n");
		device.swapchain.valid = false;
		return false;
	}
	else if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
	{
		LOG(Error, "vkAcquireNextImageKHR failed.\n");
		return false;
	}

	// The semaphore above only unblocks the GPU; some drivers let the CPU-side
	// call return before the image is actually free. Force the CPU to pace
	// itself to the display here too, via the fence, so this wait lands before
	// the next frame's input is sampled rather than bleeding into Present.
	const u64 fenceTimeoutNs = 1000000000ull; // 1s cap: never hang forever on a stalled presentation engine
	const VkResult fenceWaitResult = vkWaitForFences( device.handle, 1, &fence, VK_TRUE, fenceTimeoutNs );
	if ( fenceWaitResult == VK_TIMEOUT )
	{
		LOG(Warning, "AcquireSwapchainImage - fence wait timed out\n");
		device.swapchain.valid = false;
		return false;
	}
	CheckVulkanResult(fenceWaitResult, "vkWaitForFences");
	VK_CALL( vkResetFences( device.handle, 1, &fence ) );

	device.swapchain.currentImageIndex = imageIndex;
	device.swapchain.imageAcquired = true;

	return true;
}

bool BeginFrame(GraphicsDevice &device)
{
	// No-op if this frame slot fences were already waited after the previous present
	WaitFrameFences(device);

	// The image is normally acquired at EndFrame already. Acquire here only on
	// the first frame or right after a swapchain recreation.
	if ( !device.swapchain.imageAcquired )
	{
		if ( !AcquireSwapchainImage(device) )
		{
			return false;
		}
	}

	// Reset commands for this frame
	VkCommandPool commandPool = device.commandPools[device.frameSlot];
	VK_CALL( vkResetCommandPool(device.handle, commandPool, 0) );

	return true;
}

void EndFrame(GraphicsDevice &device)
{
	// The image acquired for this frame was handed over to Present
	device.swapchain.imageAcquired = false;

	// Destroy what was freed this frame
	CompactResourcePools(device);

	device.frameSlot = ( device.frameSlot + 1 ) % MAX_FRAMES_IN_FLIGHT;
	device.presentationIndex = ( device.presentationIndex + 1 ) % MAX_SWAPCHAIN_IMAGE_COUNT;

	// Throttle here, after present, instead of at BeginFrame. This way the CPU
	// blocks before the platform samples input for the next frame, not after,
	// which reduces input latency when frames are produced faster than vsync.
	WaitFrameFences(device);

	// Acquire the next image here too: with FIFO/MAILBOX and a full present
	// queue this is where the CPU blocks waiting for a free image, so the block
	// happens before input sampling for the next frame, not after it.
	if ( device.swapchain.valid )
	{
		AcquireSwapchainImage(device);
	}
}



//////////////////////////////
// Cleanup
//////////////////////////////

void CleanupGraphicsDevice(GraphicsDevice &device, Arena scratch)
{
	SavePipelineCacheData(device, scratch);

	WaitDeviceIdle(device);
	CompactResourcePools(device);


	for (u16 i = 0; i < HandleCount(device.framebufferPool); ++i)
	{
		vkDestroyFramebuffer( device.handle, device.framebuffers[i].handle, VULKAN_ALLOCATORS );
	}

	for (u16 i = 0; i < HandleCount(device.renderPassPool); ++i)
	{
		DestroyRenderPass( device, device.renderPasses[i] );
	}

	for (u16 i = 0; i < HandleCount(device.pipelinePool); ++i)
	{
		DestroyPipeline( device, device.pipelines[i] );
	}

	for (u16 i = 0; i < HandleCount(device.bufferViewPool); ++i)
	{
		DestroyBufferView( device, device.bufferViews[i] );
	}

	for (u16 i = 0; i < HandleCount(device.bufferPool); ++i)
	{
		DestroyBuffer( device, device.buffers[i] );
	}

	for (u16 i = 0; i < HandleCount(device.imagePool); ++i)
	{
		DestroyImage( device, device.images[i] );
	}

	for (u16 i = 0; i < HandleCount(device.samplerPool); ++i)
	{
		DestroySampler( device, device.samplers[i] );
	}

	for (u16 i = 0; i < HandleCount(device.bindGroupAllocatorPool); ++i)
	{
		vkDestroyDescriptorPool( device.handle, device.bindGroupAllocators[i].handle, VULKAN_ALLOCATORS );
	}

	for (u16 i = 0; i < HandleCount(device.bindGroupLayoutPool); ++i)
	{
		vkDestroyDescriptorSetLayout( device.handle, device.bindGroupLayouts[i].handle, VULKAN_ALLOCATORS );
	}

#if DEVELOPMENT_BUILD
	for (u16 i = 0; i < HandleCount(device.timestampPoolPool); ++i)
	{
		vkDestroyQueryPool( device.handle, device.timestampPools[i].handle, VULKAN_ALLOCATORS );
	}
#endif

	vkDestroyPipelineCache( device.handle, device.pipelineCache, VULKAN_ALLOCATORS );

	for ( u32 i = 0; i < MAX_SWAPCHAIN_IMAGE_COUNT; ++i )
	{
		vkDestroySemaphore( device.handle, device.imageAcquiredSemaphores[i], VULKAN_ALLOCATORS );
		vkDestroyFence( device.handle, device.imageAcquiredFences[i], VULKAN_ALLOCATORS );
		vkDestroySemaphore( device.handle, device.renderFinishedSemaphores[i], VULKAN_ALLOCATORS );
	}

	for ( u32 i = 0; i < MAX_FENCES; ++i )
	{
		vkDestroyFence( device.handle, device.fences[i], VULKAN_ALLOCATORS );
	}

	vkDestroyCommandPool( device.handle, device.transientCommandPool, VULKAN_ALLOCATORS );

	for ( u32 i = 0; i < ARRAY_COUNT(device.commandPools); ++i )
	{
		vkDestroyCommandPool( device.handle, device.commandPools[i], VULKAN_ALLOCATORS );
	}

	for (u32 i = 0; i < HeapType_COUNT; ++i)
	{
		DestroyHeap(device, device.heaps[i]);
	}

	vkDestroyDevice(device.handle, VULKAN_ALLOCATORS);
}

void CleanupGraphicsSurface(const GraphicsDevice &device)
{
	vkDestroySurfaceKHR(device.instance, device.surface, VULKAN_ALLOCATORS);
}

void CleanupGraphicsDriver(GraphicsDevice &device)
{
#if DEVELOPMENT_BUILD
	if ( device.support.debugUtils )
	{
		vkDestroyDebugUtilsMessengerEXT( device.instance, device.debugUtilsMessenger, VULKAN_ALLOCATORS );
	}
#endif

	vkDestroyInstance(device.instance, VULKAN_ALLOCATORS);

	MemSet(&device, sizeof(device), 0); // IsValidGraphicsDevice is false from now on
}

//////////////////////////////
// Device accessors
//////////////////////////////

const DeviceCaps &GetDeviceCaps(const GraphicsDevice &device)
{
	return device.caps;
}

u32 GetFrameSlot(const GraphicsDevice &device)
{
	return device.frameSlot;
}

u32 GetHeapSize(const GraphicsDevice &device, HeapType heapType)
{
	return device.heaps[heapType].size;
}

void ResetHeap(GraphicsDevice &device, HeapType heapType)
{
	ASSERT(heapType != HeapType_General && "The general heap is managed by its offset allocator");
	device.heaps[heapType].used = 0;
}

RenderPassH FindRenderPass(const GraphicsDevice &device, const char *name)
{
	for (u16 i = 0; i < HandleCount(device.renderPassPool); ++i) {
		const Handle handle = GetHandleAt(device.renderPassPool, i);
		if ( !IsPendingRemoval(device.renderPassPool, handle) && StrEq(device.renderPasses[i].name, name) ) {
			return handle;
		}
	}
	return {};
}

uint2 GetSwapchainSize(const GraphicsDevice &device)
{
	return { device.swapchain.extent.width, device.swapchain.extent.height };
}

Format GetSwapchainFormat(const GraphicsDevice &device)
{
	return device.swapchainInfo.format;
}

float GetSwapchainPreRotation(const GraphicsDevice &device)
{
	return device.swapchain.preRotationDegrees;
}

u32 GetSwapchainImageCount(const GraphicsDevice &device)
{
	return device.swapchain.imageCount;
}

u32 GetSwapchainImageIndex(const GraphicsDevice &device)
{
	return device.swapchain.currentImageIndex;
}

ImageH GetSwapchainImage(const GraphicsDevice &device, u32 index)
{
	ASSERT(index < device.swapchain.imageCount);
	return device.swapchain.imageHandles[index];
}


//////////////////////////////
// Resource accessors
//////////////////////////////

u32 GetBufferSize(const GraphicsDevice &device, BufferH handle)
{
	return GetBufferConst(device, handle).size;
}

uint2 GetImageSize(const GraphicsDevice &device, ImageH handle)
{
	const Image &image = GetImageConst(device, handle);
	return { image.width, image.height };
}

Format GetImageFormat(const GraphicsDevice &device, ImageH handle)
{
	return GetImageConst(device, handle).format;
}

u32 GetImageMipLevels(const GraphicsDevice &device, ImageH handle)
{
	return GetImageConst(device, handle).mipLevels;
}

BindGroupLayoutH GetPipelineBindGroupLayout(const GraphicsDevice &device, PipelineH handle, u32 bindGroupIndex)
{
	ASSERT(bindGroupIndex < MAX_BIND_GROUPS);
	return GetPipeline(device, handle).layout.bindGroupLayouts[bindGroupIndex];
}

//void FlushBufferMemory(GraphicsDevice &device, BufferH bufferHandle)
//{
//	Buffer &buffer = GetBuffer(device, bufferHandle);
//	const Heap &heap = device.heaps[buffer.alloc.heap];
//	const VkMappedMemoryRange range[] = { {
//		.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
//		.memory = heap.memory,
//		.offset = buffer.alloc.offset,
//		.size = buffer.alloc.size,
//	} };
//	VK_CALL( vkFlushMappedMemoryRanges(device.handle, 1, range) );
//}

#include "libs/offset_allocator/offsetAllocator.cpp"

#define SPV_IMPLEMENTATION
#define SPV_ASSERT ASSERT
#define SPV_PRINTF(...) LOG(Info, ##__VA_ARGS__)
#include "ilu_spirv.h"

#endif // TOOLS_GFX_IMPLEMENTATION_INCLUDED
