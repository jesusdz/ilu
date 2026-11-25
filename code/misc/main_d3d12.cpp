#include "../ilu_core.h"

// The min/max macros conflict with like-named member functions.
// Only use std::min and std::max defined in <algorithm>.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

// In order to define a function called CreateWindow, the Windows macro needs to
// be undefined.
#if defined(CreateWindow)
#undef CreateWindow
#endif

#include <wrl.h>
using Microsoft::WRL::ComPtr;

#define D3DX12_NO_STATE_OBJECT_HELPERS
#include "d3dx12.h"

#include <dxgi1_6.h>        // DirectX graphics infrastructure (adapters, presentation, etc)
#include <d3dcompiler.h>    // Utilities to compile HLSL code at runtime

/*
NOTE:
When using runtime compiled HLSL shaders using any of the D3DCompiler functions, do not
forget to link against the d3dcompiler.lib library and copy the D3dcompiler_47.dll to
the same folder as the binary executable when distributing your project.

A redistributable version of the D3dcompiler_47.dll file can be found in the Windows 10
SDK installation folder at C:\Program Files (x86)\Windows Kits\10\Redist\D3D\.

For more information, refer to the MSDN blog post at:
https://blogs.msdn.microsoft.com/chuckw/2012/05/07/hlsl-fxc-and-d3dcompile/
 */

static const UINT FRAME_COUNT = 2;

struct GfxDevice
{
	ComPtr<ID3D12Device2> device;
	ComPtr<IDXGISwapChain4> swapChain;
	ComPtr<ID3D12Resource> backBuffers[FRAME_COUNT];
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ComPtr<ID3D12CommandAllocator> commandAllocators[FRAME_COUNT];
	ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap; // Render Target Views (RTV)
	UINT rtvDescriptorSize;
	UINT currentBackbufferIndex;

	// Synchronization objects
	ComPtr<ID3D12Fence> fence;
	u64 fenceValue = 0;
	u64 frameFenceValues[FRAME_COUNT];
	HANDLE fenceEvent;

	// By default, enable V-Sync.
	// Can be toggled with the V key.
	bool vsyncEnabled = true;
	bool allowTearing = false;
	bool fullscreen = false;

	// Vertex buffer
	ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	// Index buffer
	//ComPtr<ID3D12Resource> indexBuffer;
	//D3D12_INDEX_BUFFER_VIEW indexBufferView;

	// Depth buffer: only one?
	ComPtr<ID3D12Resource> depthBuffer;
	ComPtr<ID3D12DescriptorHeap> depthStencilViewHeap;

	// Root signature
	ComPtr<ID3D12RootSignature> rootSignature;

	// Pipeline state object (PSO)
	ComPtr<ID3D12PipelineState> pipelineState;

	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;

	f32 fov;
	float4x4 modelMatrix;
	float4x4 viewMatrix;
	float4x4 projectionMatrix;

	bool contentLoaded;
};

struct Vertex
{
	float2 Position;
	float3 Color;
};

static const Vertex vertices[] = {
	{{ 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{ 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
	{{-0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}
};


#define ThrowIfFailed(hRes) if( FAILED( hRes ) ) { return false; }
#define NoThrowIfFailed(hRes) hRes


void SendFenceValue(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, u64 fenceValue)
{
	NoThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValue));
}


void WaitForFenceValue(ComPtr<ID3D12Fence> fence, u64 fenceValue, HANDLE fenceEvent, u64 duration = UINT64_MAX)
{
	if (fence->GetCompletedValue() < fenceValue)
	{
		HRESULT hRes = fence->SetEventOnCompletion(fenceValue, fenceEvent);
		ASSERT(hRes == S_OK && "ID3D12Fence::SetEventOnCompletion() failed.");
		::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration));
	}
}


void Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, u64 fenceValue, HANDLE fenceEvent )
{
	SendFenceValue(commandQueue, fence, fenceValue);
	WaitForFenceValue(fence, fenceValue, fenceEvent);
}


HRESULT D3D12CreateDXGIFactory(ComPtr<IDXGIFactory4> &dxgiFactory)
{
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
	HRESULT res = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory));
	return res;
}


bool CreateSwapchain(GfxDevice &gfxDevice, Window &window)
{
	// Create a DXGI factory
	ComPtr<IDXGIFactory4> dxgiFactory;
	ThrowIfFailed(D3D12CreateDXGIFactory(dxgiFactory));

	u32 width = 0;
	u32 height = 0;
	u32 bufferCount = FRAME_COUNT;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = bufferCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	// It is recommended to always allow tearing if tearing support is available.
	swapChainDesc.Flags = gfxDevice.allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
				gfxDevice.commandQueue.Get(),
				window.hWnd,
				&swapChainDesc,
				nullptr,
				nullptr,
				&swapChain1));

	// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen will be handled manually.
	ThrowIfFailed(dxgiFactory->MakeWindowAssociation(window.hWnd, DXGI_MWA_NO_ALT_ENTER));

	ComPtr<IDXGISwapChain4> swapChain;
	ThrowIfFailed(swapChain1.As(&swapChain));


	// Descriptor heap
	ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
	D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = FRAME_COUNT;
		desc.Type = descriptorHeapType;

		ThrowIfFailed(gfxDevice.device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&rtvDescriptorHeap)));
	}


	// Render target views (RTV)
	ComPtr<ID3D12Resource> backBuffers[FRAME_COUNT] = {};
	UINT rtvDescriptorSize = gfxDevice.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

		for (int i = 0; i < FRAME_COUNT; ++i)
		{
			ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i])));
			gfxDevice.device->CreateRenderTargetView(backBuffers[i].Get(), nullptr, rtvHandle);
			rtvHandle.ptr += rtvDescriptorSize;
		}
	}


	// Populate device
	gfxDevice.swapChain = swapChain;
	gfxDevice.rtvDescriptorHeap = rtvDescriptorHeap;
	gfxDevice.rtvDescriptorSize = rtvDescriptorSize;
	for (u32 i = 0; i < FRAME_COUNT; ++i) { gfxDevice.backBuffers[i] = backBuffers[i]; }
	gfxDevice.currentBackbufferIndex = swapChain->GetCurrentBackBufferIndex();

	return true;
}


bool RecreateSwapchain(GfxDevice &gfxDevice, Window &window)
{
	++gfxDevice.fenceValue;
	Flush(gfxDevice.commandQueue, gfxDevice.fence, gfxDevice.fenceValue, gfxDevice.fenceEvent);

	for ( u32 i = 0; i < FRAME_COUNT; ++i )
	{
		gfxDevice.backBuffers[i].Reset();
		gfxDevice.frameFenceValues[i] = gfxDevice.frameFenceValues[gfxDevice.currentBackbufferIndex];
	}

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	ThrowIfFailed(gfxDevice.swapChain->GetDesc(&swapChainDesc));
	ThrowIfFailed(gfxDevice.swapChain->ResizeBuffers(
				FRAME_COUNT,
				window.width, window.height,
				swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));


	// Render target views (RTV)
	ComPtr<ID3D12Resource> backBuffers[FRAME_COUNT] = {};
	UINT rtvDescriptorSize = gfxDevice.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(gfxDevice.rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

		for (int i = 0; i < FRAME_COUNT; ++i)
		{
			ThrowIfFailed(gfxDevice.swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i])));
			gfxDevice.device->CreateRenderTargetView(backBuffers[i].Get(), nullptr, rtvHandle);
			rtvHandle.ptr += rtvDescriptorSize;
		}
	}


	gfxDevice.rtvDescriptorSize = rtvDescriptorSize;
	for (u32 i = 0; i < FRAME_COUNT; ++i) { gfxDevice.backBuffers[i] = backBuffers[i]; }
	gfxDevice.currentBackbufferIndex = gfxDevice.swapChain->GetCurrentBackBufferIndex();

	return true;
}


bool InitializeGraphics(Arena &arena, Window &window, GfxDevice &gfxDevice)
{
	// Always enable the debug layer before doing anything DX12 related
	// so all possible errors generated while creating DX12 objects
	// are caught by the debug layer.
	ComPtr<ID3D12Debug> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();


	// Create a DXGI factory
	ComPtr<IDXGIFactory4> dxgiFactory;
	ThrowIfFailed(D3D12CreateDXGIFactory(dxgiFactory));


	// Graphics adapter selection
	ComPtr<IDXGIAdapter1> dxgiAdapter1;
	ComPtr<IDXGIAdapter4> dxgiAdapter4;

	const bool useWarp = false;
	if (useWarp)
	{
		ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
		ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
	}
	else
	{
		SIZE_T maxDedicatedVideoMemory = 0;
		for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

			// Check to see if the adapter can create a D3D12 device without actually
			// creating it. The adapter with the largest dedicated video memory
			// is favored.
			if ( (dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
					dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory &&
					SUCCEEDED( D3D12CreateDevice( dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr) ) )
			{
				maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
				ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
			}
		}
	}


	// Device creation
	ComPtr<ID3D12Device2> device;
	ThrowIfFailed(D3D12CreateDevice(dxgiAdapter4.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));


	// Enable debug messages in debug mode.
#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(device.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		// Suppress whole categories of messages
		//D3D12_MESSAGE_CATEGORY Categories[] = {};

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY Severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID DenyIds[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		//NewFilter.DenyList.NumCategories = ARRAY_COUNT(Categories);
		//NewFilter.DenyList.pCategoryList = Categories;
		NewFilter.DenyList.NumSeverities = ARRAY_COUNT(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = ARRAY_COUNT(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;

		ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
	}
#endif


	// Command queue
	ComPtr<ID3D12CommandQueue> commandQueue;
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;

		ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue)));
	}


	// Query tearing support
	BOOL allowTearing = FALSE;
	{
		// Rather than create the DXGI 1.5 factory interface directly, we create the
		// DXGI 1.4 interface and query for the 1.5 interface. This is to enable the
		// graphics debugging tools which will not support the 1.5 factory interface
		// until a future update.
		ComPtr<IDXGIFactory4> factory4;
		if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
		{
			ComPtr<IDXGIFactory5> factory5;
			if (SUCCEEDED(factory4.As(&factory5)))
			{
				if (FAILED(factory5->CheckFeatureSupport(
								DXGI_FEATURE_PRESENT_ALLOW_TEARING,
								&allowTearing, sizeof(allowTearing))))
				{
					allowTearing = FALSE;
				}
			}
		}
	}


	gfxDevice.device = device;
	gfxDevice.commandQueue = commandQueue;
	gfxDevice.allowTearing = allowTearing;


	// Create the swapChain
	CreateSwapchain(gfxDevice, window);


	// Command allocators
	ComPtr<ID3D12CommandAllocator> commandAllocators[FRAME_COUNT];
	{
		D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		for (u32 i = 0; i < FRAME_COUNT; ++i)
		{
			ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocators[i])));
		}
	}


	// Command list
	ComPtr<ID3D12GraphicsCommandList> commandList;
	{
		D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		ThrowIfFailed(device->CreateCommandList(0, type, commandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&commandList)));
		ThrowIfFailed(commandList->Close()); // For conveniency, so the first command in the render loop can be Reset
	}


	// Create fence
	ComPtr<ID3D12Fence> fence;
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	// Fence event
	HANDLE fenceEvent;
	fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	ASSERT(fenceEvent && "Failed to create fence event.");


	// Populate device
	gfxDevice.commandQueue = commandQueue;
	for (u32 i = 0; i < FRAME_COUNT; ++i) { gfxDevice.commandAllocators[i] = commandAllocators[i]; }
	gfxDevice.commandList = commandList;

	// Synchronization objects
	gfxDevice.fence = fence;
	gfxDevice.fenceValue = 0;
	for (u32 i = 0; i < FRAME_COUNT; ++i) { gfxDevice.frameFenceValues[i] = 0; }
	gfxDevice.fenceEvent = fenceEvent;


	// New stuff
	gfxDevice.scissorRect = {0, 0, LONG_MAX, LONG_MAX};
	gfxDevice.viewport = {0.0f, 0.0f, (float)window.width, (float)window.height};
	gfxDevice.fov = 45.0f;
	gfxDevice.contentLoaded = false;

	// Upload vertex buffer
	u32 vertexBufferSize = sizeof(vertices);

	ComPtr<ID3D12Resource> intermediateVertexBuffer;
	D3D12_HEAP_PROPERTIES intermediateHeapProperties =
		device->GetCustomHeapProperties(0, D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC iResourceDesc = {};
	iResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	iResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	iResourceDesc.Width = vertexBufferSize;
	iResourceDesc.Height = 1;
	iResourceDesc.DepthOrArraySize = 1;
	iResourceDesc.MipLevels = 1;
	iResourceDesc.SampleDesc.Count = 1;
	iResourceDesc.SampleDesc.Quality = 0;
	iResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	iResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	ThrowIfFailed(device->CreateCommittedResource(
				&intermediateHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&iResourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				NULL,
				IID_PPV_ARGS(&intermediateVertexBuffer)));

	D3D12_HEAP_PROPERTIES destinationHeapProperties =
		device->GetCustomHeapProperties(0, D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC dResourceDesc = {};
	dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	dResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	dResourceDesc.Width = vertexBufferSize;
	dResourceDesc.Height = 1;
	dResourceDesc.DepthOrArraySize = 1;
	dResourceDesc.MipLevels = 1;
	dResourceDesc.SampleDesc.Count = 1;
	dResourceDesc.SampleDesc.Quality = 0;
	dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	dResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	ThrowIfFailed(device->CreateCommittedResource(
				&destinationHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&dResourceDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				NULL,
				IID_PPV_ARGS(&gfxDevice.vertexBuffer)));

	D3D12_SUBRESOURCE_DATA subresourceData = {};
	subresourceData.pData = &vertices;
	subresourceData.RowPitch = vertexBufferSize;
	subresourceData.SlicePitch = vertexBufferSize;
 
	UpdateSubresources(commandList.Get(),
		gfxDevice.vertexBuffer.Get(), intermediateVertexBuffer.Get(),
		0, 0, 1, &subresourceData);

	// Vertex buffer view
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	vertexBufferView.BufferLocation = gfxDevice.vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = vertexBufferSize;
	vertexBufferView.StrideInBytes = sizeof(Vertex);
	gfxDevice.vertexBufferView = vertexBufferView;

	// Upload index buffer
	//ComPtr<ID3D12Resource> intermediateIndexBuffer;
	//UpdateBufferResource(commandList.Get(), &gfxDevice.indexBuffer, &intermediateIndexBuffer, ARRAY_COUNT(indices), sizeof(WORD), indices);
	// TODO: Move all the previous vertex buffer upload stuff into a new function
	// and reuse the function to upload an index buffer... but for now, we will
	// go forward without indices


	// Depth stencil view
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&gfxDevice.depthStencilViewHeap)));


	// Load the vertex shader.
	ComPtr<ID3DBlob> vertexShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"shaders/vertex.cso", &vertexShaderBlob));

	// Load the pixel shader.
	ComPtr<ID3DBlob> pixelShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"shaders/fragment.cso", &pixelShaderBlob));


	// Create the vertex input layout
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};


	// Create a root signature.
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Allow input layout and deny unnecessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	// A single 32-bit constant root parameter that is used by the vertex shader.
	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	rootParameters[0].InitAsConstants(sizeof(float4), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(ARRAY_COUNT(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

	// Serialize the root signature.
    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
        featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
    // Create the root signature.
    ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), 
        rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&gfxDevice.rootSignature)));


	// TODO: Pipeline State Object (PSO)

	return true;
}

void CleanupGraphics(GfxDevice &gfxDevice)
{
	++gfxDevice.fenceValue;
	Flush(gfxDevice.commandQueue, gfxDevice.fence, gfxDevice.fenceValue, gfxDevice.fenceEvent);

	::CloseHandle(gfxDevice.fenceEvent);
}

void RenderGraphics(GfxDevice &gfxDevice)
{
	// Get this frame resources
	ComPtr<ID3D12CommandAllocator> commandAllocator = gfxDevice.commandAllocators[gfxDevice.currentBackbufferIndex];
	ComPtr<ID3D12Resource> backBuffer = gfxDevice.backBuffers[gfxDevice.currentBackbufferIndex];

	// Get the command list ready to work
	commandAllocator->Reset();
	gfxDevice.commandList->Reset(commandAllocator.Get(), nullptr);

	// NOTE: This allows retrieving the command allocator from the command list later (if we need it)
	//NoThrowIfFailed(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));

	// Clear the render target
	{
		D3D12_RESOURCE_BARRIER barrier = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION };
		barrier.Transition.pResource = backBuffer.Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		gfxDevice.commandList->ResourceBarrier(1, &barrier);

		FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

		D3D12_CPU_DESCRIPTOR_HANDLE rtv = gfxDevice.rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		rtv.ptr += gfxDevice.currentBackbufferIndex * gfxDevice.rtvDescriptorSize;

		gfxDevice.commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
	}

	// Present
	{
		D3D12_RESOURCE_BARRIER barrier = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION };
		barrier.Transition.pResource = backBuffer.Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		gfxDevice.commandList->ResourceBarrier(1, &barrier);

		NoThrowIfFailed(gfxDevice.commandList->Close());

		// NOTE: Here we have the command allocator backing this command list
		//ID3D12CommandAllocator* commandAllocator;
		//UINT dataSize = sizeof(commandAllocator);
		//NoThrowIfFailed(commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator));

		ID3D12CommandList* const commandLists[] = { gfxDevice.commandList.Get() };
		gfxDevice.commandQueue->ExecuteCommandLists(ARRAY_COUNT(commandLists), commandLists);

		UINT syncInterval = gfxDevice.vsyncEnabled ? 1 : 0;
		UINT presentFlags = gfxDevice.allowTearing && !gfxDevice.vsyncEnabled ? DXGI_PRESENT_ALLOW_TEARING : 0;
		NoThrowIfFailed(gfxDevice.swapChain->Present(syncInterval, presentFlags));

		// After present, send a new fence value
		++gfxDevice.fenceValue;
		SendFenceValue(gfxDevice.commandQueue, gfxDevice.fence, gfxDevice.fenceValue);
		gfxDevice.frameFenceValues[gfxDevice.currentBackbufferIndex] = gfxDevice.fenceValue;

		// Update backbuffer index and wait until it's ready
		gfxDevice.currentBackbufferIndex = gfxDevice.swapChain->GetCurrentBackBufferIndex();
		WaitForFenceValue(gfxDevice.fence, gfxDevice.frameFenceValues[gfxDevice.currentBackbufferIndex], gfxDevice.fenceEvent);
	}

	static Clock clock0 = GetClock();
	Clock clock1 = GetClock();
	float deltaSeconds = GetSecondsElapsed(clock0, clock1);
	clock0 = clock1;

	static float elapsedSeconds = 0.0f;
	static u64 frameCount = 0;
	elapsedSeconds += deltaSeconds;
	frameCount++;
	if ( elapsedSeconds > 1.0f )
	{
		float framesPerSecond = frameCount / elapsedSeconds;
		LOG(Info, "FPS: %f\n", framesPerSecond);
		elapsedSeconds = 0.0f;
		frameCount = 0;
	}
}

int main()
{
	// Create Window
	Window window = {};
	if ( !InitializeWindow(window) )
	{
		LOG(Error, "InitializeWindow failed!\n");
		return -1;
	}

	// Allocate base memory
	u32 baseMemorySize = MB(64);
	byte *baseMemory = (byte*)AllocateVirtualMemory(baseMemorySize);
	Arena arena = MakeArena(baseMemory, baseMemorySize, "Main arena");

	// Initialize graphics
	GfxDevice gfxDevice = {};
	if ( !InitializeGraphics(arena, window, gfxDevice) )
	{
		LOG(Error, "InitializeGraphics failed!\n");
		return -1;
	}

	// Application loop
	while ( 1 )
	{
		ProcessWindowEvents(window);

		if (window.flags & WindowFlags_Resized)
		{
			RecreateSwapchain(gfxDevice, window);
		}
		if (window.flags & WindowFlags_Exiting)
		{
			break;
		}
		if (window.keyboard.keys[KEY_ESCAPE] == KEY_STATE_PRESS)
		{
			break;
		}

		RenderGraphics(gfxDevice);
	}

	CleanupGraphics(gfxDevice);

	CleanupWindow(window);

	PrintArenaUsage(arena);

	return 0;
}

