#include <iostream>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_syswm.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <fstream>
#include <vector>
#include <atlbase.h>
#include <chrono>
#include <dxcapi.h>
#include <filesystem>

#include "ShaderCompiler.h"

SDL_Window* GWindow = nullptr;
HWND GWindowHandle = nullptr;


class com_exception : public std::exception
{
public:
    com_exception(HRESULT hr) noexcept : result(hr) {}

    const char* what() const noexcept override
    {
        static char s_str[64] = {};
        sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
        return s_str;
    }

    HRESULT get_result() const noexcept { return result; }

private:
    HRESULT result;
};

inline void ThrowIfFailed(HRESULT hr) noexcept(false)
{
    if (FAILED(hr))
    {
        throw com_exception(hr);
    }
}

bool InitializeWindow(int width, int height)
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cout << "Failed to initialize SDL" << std::endl;
        return false;
    }

    // Create window
    GWindow = SDL_CreateWindow("DirectX12 Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
    if (GWindow == nullptr)
    {
        std::cout << "Failed to create SDL window" << std::endl;
        return false;
    }
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(GWindow, &wmInfo);
    GWindowHandle = wmInfo.info.win.window;

    //SDL_SetWindowGrab(GWindow, SDL_TRUE);
	return true;
}

inline std::vector<char> readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    bool exists = (bool)file;

    if (!exists || !file.is_open())
    {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
};

struct Uniforms
{
    float iResolution[3];
    float iTime;
    float iTimeDelta;
    float iFrameRate;
    int iFrame;
    float iMouse[4];
    float iDate[4];
};

int windowWidth = 800;
int windowHeight  = 600;

std::chrono::time_point<std::chrono::system_clock> startTime;
std::chrono::time_point<std::chrono::system_clock> lastTime;
int frameCount = 0;

void UpdateUniforms(Uniforms& cbVS)
{
    cbVS.iResolution[0] = windowWidth;
    cbVS.iResolution[1] = windowHeight;
    cbVS.iResolution[2] = 1.0f; // pixel aspect ratio

	auto now = std::chrono::system_clock::now();
    std::chrono::duration<float, std::ratio<1,1>> diff = now - startTime;
    cbVS.iTime = diff.count();
;
    std::chrono::duration<float, std::ratio<1,1>> delta = now - lastTime;
    cbVS.iTimeDelta = delta.count();
    cbVS.iFrameRate = 1.0f / cbVS.iTimeDelta;
    cbVS.iFrame = frameCount++;
    lastTime = now;

}



int main(int argc, char* argv[])
{
	auto now = std::chrono::system_clock::now();
	startTime = now;
    lastTime = startTime;
	
    char* givenShaderName;
	LPCWSTR shaderFilePath = L"../Shaders/triangle.px.hlsl";
    if (argc > 1)
    {
        givenShaderName = argv[1];
		wchar_t* wtext = new wchar_t[strlen(givenShaderName) + 1];
		mbstowcs(wtext, givenShaderName, strlen(givenShaderName) + 1);
        shaderFilePath = wtext;
    }

    if (!InitializeWindow(windowWidth, windowHeight))
    {
        return 1;
    }

	ID3D12Debug* debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();

    // Select DirectX12 Physical Adapter
    IDXGIFactory4* factory = nullptr;
    ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)));

    IDXGIAdapter1* adapter = nullptr;
    for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            continue;
        }
        if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
        {
            break;
        }
        adapter->Release();
    }

    // Create DirectX12 device
    ID3D12Device* device = nullptr;
    ThrowIfFailed(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));
    
    DXGI_ADAPTER_DESC1 desc;
    adapter->GetDesc1(&desc);
    
    std::cout << "DirectX12 device created" << std::endl;
    std::wcout << "Adapter: " << desc.Description << std::endl;
    std::cout << "Vendor ID: " << desc.VendorId << std::endl;
    std::cout << "Device ID: " << desc.DeviceId << std::endl;
    std::cout << "Dedicated Video Memory: " << desc.DedicatedVideoMemory << std::endl;

    // Create the command queue
    ID3D12CommandQueue* commandQueue;
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

    ID3D12CommandAllocator* commandAllocator;
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));

    UINT frameIndex = 0;
    HANDLE fenceEvent;
    ID3D12Fence* fence;
    UINT64 fenceValue = 0;

    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
	 fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fenceEvent == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}

    static const UINT backbufferCount = 2;
    ID3D12DescriptorHeap* renderTargetViewHeap;
    ID3D12Resource* renderTargets[backbufferCount];
    UINT rtvDescriptorSize;

    IDXGISwapChain1* swapchain1;
    IDXGISwapChain3* swapchain;

    D3D12_VIEWPORT viewport;
    D3D12_RECT surfaceSize;

    surfaceSize.left = 0;
    surfaceSize.top = 0;
    surfaceSize.right = windowWidth;
    surfaceSize.bottom = windowHeight;

    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = windowWidth;
    viewport.Height = windowHeight;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
    swapchainDesc.BufferCount = backbufferCount;
    swapchainDesc.Width = windowWidth;
    swapchainDesc.Height = windowHeight;
    swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapchainDesc.SampleDesc.Count = 1;

    ThrowIfFailed(factory->CreateSwapChainForHwnd(commandQueue, GWindowHandle, &swapchainDesc, NULL, NULL, &swapchain1));

    ThrowIfFailed(swapchain1->QueryInterface(IID_PPV_ARGS(&swapchain)));

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = backbufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&renderTargetViewHeap)));

    rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart());

    for(int i = 0; i < backbufferCount; i++)
    {
		ThrowIfFailed(swapchain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
		device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);
        rtvHandle.ptr += rtvDescriptorSize;
    }

    ID3D12RootSignature* rootSignature;

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData;
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

	D3D12_DESCRIPTOR_RANGE1 ranges[1];
    ranges[0].BaseShaderRegister = 0;
    ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    ranges[0].NumDescriptors = 1;
    ranges[0].RegisterSpace = 0;
    ranges[0].OffsetInDescriptorsFromTableStart = 0;
    ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

    D3D12_ROOT_PARAMETER1 rootParameters[1];
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[0].DescriptorTable.pDescriptorRanges = ranges;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootSignatureDesc.Desc_1_1.NumParameters = 1;
    rootSignatureDesc.Desc_1_1.pParameters = rootParameters;

    ID3DBlob* signature;
    ID3DBlob* error;

    try
    {
        ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error));
        ThrowIfFailed(
            device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
        rootSignature->SetName(L"hixypixy root signature");
    }
    catch (std::exception e)
    {
        const char* errStr = (const char*)error->GetBufferPointer();
        std::cout << errStr;
        error->Release();
        error = nullptr;
    }

    if(signature)
    {
        signature->Release();
        signature = nullptr;
    }

    
    // Vertex data for the triangle
    float vertexData[9] =
    {
        -3.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        1.0f, 3.0f, 0.0f,
    };

    ID3D12Resource* vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

    const UINT vertexBufferSize =  _countof(vertexData) * sizeof(float);

    D3D12_HEAP_PROPERTIES heapProps;
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC vertexBufferResourceDesc;
    vertexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    vertexBufferResourceDesc.Alignment = 0;
    vertexBufferResourceDesc.Width = vertexBufferSize;
    vertexBufferResourceDesc.Height = 1;
    vertexBufferResourceDesc.DepthOrArraySize = 1;
    vertexBufferResourceDesc.MipLevels = 1;
    vertexBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    vertexBufferResourceDesc.SampleDesc.Count = 1;
    vertexBufferResourceDesc.SampleDesc.Quality = 0;
    vertexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    vertexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer)));

    UINT8* pVertexDataBegin;

    D3D12_RANGE readRange;
    readRange.Begin = 0;
    readRange.End = 0;


    ThrowIfFailed(vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
    memcpy(pVertexDataBegin, vertexData, vertexBufferSize);
    vertexBuffer->Unmap(0, nullptr);

    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = sizeof(float) * 3;
    vertexBufferView.SizeInBytes = vertexBufferSize;

	Uniforms cbVS;
    UpdateUniforms(cbVS);

	ID3D12Resource* constantBuffer;
	UINT8* mappedConstantBuffer;

	ID3D12DescriptorHeap* mainDescriptorHeap;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.NumDescriptors = 2;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ThrowIfFailed(device->CreateDescriptorHeap(&descHeapDesc,
											   IID_PPV_ARGS(&mainDescriptorHeap)));
	mainDescriptorHeap->SetName(L"Descriptor Heap For CBV + SRV");


	D3D12_HEAP_PROPERTIES cbHeapProperties;
	cbHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	cbHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	cbHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	cbHeapProperties.CreationNodeMask = 1;
	cbHeapProperties.VisibleNodeMask = 1;


	frameIndex = swapchain->GetCurrentBackBufferIndex();
	D3D12_RESOURCE_DESC cbResourceDesc;
	cbResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbResourceDesc.Alignment = 0;
	cbResourceDesc.Width = (sizeof(cbVS) + 255) & ~255;
	cbResourceDesc.Height = 1;
	cbResourceDesc.DepthOrArraySize = 1;
	cbResourceDesc.MipLevels = 1;
	cbResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	cbResourceDesc.SampleDesc.Count = 1;
	cbResourceDesc.SampleDesc.Quality = 0;
	cbResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	cbResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ThrowIfFailed(device->CreateCommittedResource(
		&cbHeapProperties, D3D12_HEAP_FLAG_NONE, &cbResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constantBuffer)));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constantBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = (sizeof(cbVS) + 255) & ~255; // CB size is required to be 256-byte aligned.

	D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle(mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	cbvHandle.ptr = cbvHandle.ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 0;

	device->CreateConstantBufferView(&cbvDesc, cbvHandle);

	D3D12_RANGE readRange3;
	readRange3.Begin = 0;
	readRange3.End = 0;

	ThrowIfFailed(constantBuffer->Map(
		0, &readRange3, reinterpret_cast<void**>(&mappedConstantBuffer)));
	memcpy(mappedConstantBuffer, &cbVS, sizeof(cbVS));

    ///////////////////////////////////
    
    ShaderCompiler compiler;

    CComPtr<ID3DBlob> vsShader, pxShader;
    bool v = compiler.CompileVertexShader(L"../Shaders/triangle.vert.hlsl", vsShader);
    bool p = compiler.CompilePixelShader(shaderFilePath, pxShader);
    if(!v || !p)
    {
        return 2;
    }

    auto pixelShaderLastWriteTime = std::filesystem::last_write_time(shaderFilePath);

	////////////////////////////////////
	
	D3D12_SHADER_BYTECODE vsBytecode;
	vsBytecode.BytecodeLength = vsShader->GetBufferSize();
	vsBytecode.pShaderBytecode = vsShader->GetBufferPointer();

	D3D12_SHADER_BYTECODE psBytecode;
	psBytecode.BytecodeLength = pxShader->GetBufferSize();
	psBytecode.pShaderBytecode = pxShader->GetBufferPointer();

    ID3D12PipelineState* pipelineState;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

    D3D12_INPUT_ELEMENT_DESC inputElements[] = {{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
    											D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};
    psoDesc.InputLayout = { inputElements, _countof(inputElements) };
	psoDesc.pRootSignature = rootSignature;

	psoDesc.VS = vsBytecode;
	psoDesc.PS = psBytecode;

	D3D12_RASTERIZER_DESC rasterDesc;
	rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterDesc.CullMode = D3D12_CULL_MODE_NONE;
	rasterDesc.FrontCounterClockwise = FALSE;
	rasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterDesc.DepthClipEnable = TRUE;
	rasterDesc.MultisampleEnable = FALSE;
	rasterDesc.AntialiasedLineEnable = FALSE;
	rasterDesc.ForcedSampleCount = 0;
	rasterDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    psoDesc.RasterizerState = rasterDesc;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	D3D12_BLEND_DESC blendDesc;
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
		FALSE,
		FALSE,
		D3D12_BLEND_ONE,
		D3D12_BLEND_ZERO,
		D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE,
		D3D12_BLEND_ZERO,
		D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL,
	};
    
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		blendDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;

	psoDesc.BlendState = blendDesc;
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;

	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	try
	{
		ThrowIfFailed(device->CreateGraphicsPipelineState(
			&psoDesc, IID_PPV_ARGS(&pipelineState)));
	}
	catch (com_exception e)
	{
		std::cout << "Failed to create Graphics Pipeline! " << e.what();
	}

	ID3D12GraphicsCommandList* commandList;
	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
											commandAllocator, pipelineState,
											IID_PPV_ARGS(&commandList)));
    commandList->Close();


	frameIndex = swapchain->GetCurrentBackBufferIndex();
    SDL_Event event;
    bool quit = false;

        std::cerr << "Fail in shader compilation!" << std::endl;
    while (!quit)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                quit = true;
            }
            if (event.type == SDL_KEYDOWN)
            {
	            switch (event.key.keysym.sym)
	            {
	            case SDLK_r:
                    vsShader = NULL;
                    pxShader = NULL;
		            if (!compiler.CompilePixelShader(shaderFilePath, pxShader) || 
                        !compiler.CompileVertexShader(L"../Shaders/triangle.vert.hlsl", vsShader))
		            {
                        break;
		            }

                    vsBytecode.BytecodeLength = vsShader->GetBufferSize();
                    vsBytecode.pShaderBytecode = vsShader->GetBufferPointer();
                    psBytecode.BytecodeLength = pxShader->GetBufferSize();
                    psBytecode.pShaderBytecode = pxShader->GetBufferPointer();

					psoDesc.VS = vsBytecode;
					psoDesc.PS = psBytecode;

		            try
		            {
			            ThrowIfFailed(device->CreateGraphicsPipelineState(
				            &psoDesc, IID_PPV_ARGS(&pipelineState)));
		            }
		            catch (com_exception e)
		            {
			            std::cout << "Failed to create Graphics Pipeline! " << e.what();
		            }
		            break;
	            }
            }
            if(event.type == SDL_MOUSEBUTTONDOWN)
            {

            }
		}

		auto currentPixelShaderLastWriteTime = std::filesystem::last_write_time(shaderFilePath);
        if(currentPixelShaderLastWriteTime != pixelShaderLastWriteTime)
        {
	        vsShader = NULL;
	        pxShader = NULL;
	        if (compiler.CompilePixelShader(shaderFilePath, pxShader) &&
		        compiler.CompileVertexShader(L"../Shaders/triangle.vert.hlsl", vsShader))
	        {
				vsBytecode.BytecodeLength = vsShader->GetBufferSize();
				vsBytecode.pShaderBytecode = vsShader->GetBufferPointer();
				psBytecode.BytecodeLength = pxShader->GetBufferSize();
				psBytecode.pShaderBytecode = pxShader->GetBufferPointer();

				psoDesc.VS = vsBytecode;
				psoDesc.PS = psBytecode;

				try
				{
					ThrowIfFailed(device->CreateGraphicsPipelineState(
						&psoDesc, IID_PPV_ARGS(&pipelineState)));
				}
				catch (com_exception e)
				{
					std::cout << "Failed to create Graphics Pipeline! " << e.what();
				}
	        }
        }


        UpdateUniforms(cbVS);
		memcpy(mappedConstantBuffer, &cbVS, sizeof(cbVS));

		ThrowIfFailed(commandAllocator->Reset());

		ThrowIfFailed(commandList->Reset(commandAllocator, pipelineState));

		commandList->SetGraphicsRootSignature(rootSignature);

		ID3D12DescriptorHeap* pDescriptorHeaps[] = {mainDescriptorHeap};
		commandList->SetDescriptorHeaps(_countof(pDescriptorHeaps), pDescriptorHeaps);

		D3D12_GPU_DESCRIPTOR_HANDLE cbvHandle2(mainDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		commandList->SetGraphicsRootDescriptorTable(0, cbvHandle2);

		D3D12_RESOURCE_BARRIER renderTargetBarrier;
		renderTargetBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		renderTargetBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		renderTargetBarrier.Transition.pResource = renderTargets[frameIndex];
		renderTargetBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		renderTargetBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		renderTargetBarrier.Transition.Subresource =
			D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &renderTargetBarrier);

		D3D12_CPU_DESCRIPTOR_HANDLE
			rtvHandle2(renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart());
		rtvHandle2.ptr = rtvHandle2.ptr + (frameIndex * rtvDescriptorSize);

        commandList->OMSetRenderTargets(1, &rtvHandle2, FALSE, nullptr);

		const float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &surfaceSize);
		commandList->ClearRenderTargetView(rtvHandle2, clearColor, 0, nullptr);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
		//commandList->IASetIndexBuffer(&indexBufferView);
        commandList->DrawInstanced(3, 1, 0, 0);

		D3D12_RESOURCE_BARRIER presentBarrier;
		presentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		presentBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		presentBarrier.Transition.pResource = renderTargets[frameIndex];
		presentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		presentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		presentBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		commandList->ResourceBarrier(1, &presentBarrier);

		ThrowIfFailed(commandList->Close());

		ID3D12CommandList* ppCommandLists[] = {commandList};
		commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		 swapchain->Present(1, 0);

		// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.

		// Signal and increment the fence value.
		ThrowIfFailed(commandQueue->Signal(fence, fenceValue++));

		// Wait until the previous frame is finished.
		if (fence->GetCompletedValue() < fenceValue - 1)
		{
			ThrowIfFailed(fence->SetEventOnCompletion(fenceValue - 1, fenceEvent));
			WaitForSingleObject(fenceEvent, INFINITE);
		}

		frameIndex = swapchain->GetCurrentBackBufferIndex();
    }

    SDL_DestroyWindow(GWindow);
    SDL_Quit();

    // Cleanup
    commandQueue->Release();
    device->Release();
    adapter->Release();
    factory->Release();

    return 0;

}