#include "FontRenderer.h"
#include "FontRenderer.h"
#include "FontRenderer.h"
#include "FontRenderer.h"
#include "FontRenderer.h"

#include <fstream>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H




void ThrowIfFailed(HRESULT hr);


FontRenderer::FontRenderer()
{

}

bool FontRenderer::Initialize(::ID3D12Device* device, glm::ivec2 windowSize)
{
	WindowSize = windowSize;
	Device = device;

	FT_Library ft;
	if (FT_Init_FreeType(&ft))
	{
		std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
		return false;
	}

	FT_Face face;
	if (FT_New_Face(ft, "../Fonts/arial.ttf", 0, &face))
	{
		std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
		return false;
	}

	UINT fontSize = 25;
	FT_Set_Pixel_Sizes(face, 0, fontSize);

	assert(FT_IS_SCALABLE(face));

	UINT imageWidth = (fontSize) * 16;
	UINT imageHeight = (fontSize) * 8;

	unsigned char* buffer = new unsigned char[imageWidth * imageHeight * 4];
	memset(buffer, 0, imageHeight * imageWidth * 4);

	//for debug purposes write it to file
	std::ofstream image("image.ppm");
	image << "P3\n" << imageWidth << " " << imageHeight << "\n255\n";

	for (int i = 0; i < 128; ++i)
	{
		auto glyphIndex = FT_Get_Char_Index(face, i);

		auto error = FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT);
		if (error)
		{
			std::cout << "BitmapFontGenerator > failed to load glyph, error code: " << error << std::endl;
		}

		error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
		if (error)
		{
			std::cout << "BitmapFontGenerator > failed to render glyph, error code: " << error << std::endl;
		}

		int x = (i % 16) * (fontSize);
		int y = (i / 16) * (fontSize);

		const FT_Bitmap& bitmap = face->glyph->bitmap;
		for (int yy = 0; yy < bitmap.rows; yy++)
		{
			for (int xx = 0; xx < bitmap.width; xx++)
			{
				unsigned char r = bitmap.buffer[(yy * (bitmap.width) + xx)];
				buffer[(y + yy) * imageWidth * 4 + (x + xx) * 4 + 0] = r;
				buffer[(y + yy) * imageWidth * 4 + (x + xx) * 4 + 1] = r;
				buffer[(y + yy) * imageWidth * 4 + (x + xx) * 4 + 2] = r;
				buffer[(y + yy) * imageWidth * 4 + (x + xx) * 4 + 3] = r;
			}
		}
		Character character = {};
		character.Projection = glm::ortho(0.0f, static_cast<float>(windowSize.x), 0.0f, static_cast<float>(windowSize.y));
		character.AtlasPos = glm::ivec2(x, y);
		character.AtlasResolution = glm::ivec2(imageWidth, imageHeight);
		character.Size = glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
		character.Bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
		character.Advance = face->glyph->advance.x;

		Characters[static_cast<char>(i)] = character;
	}
	for(int i = 0; i < imageHeight * imageWidth; i++)
	{
		image << (int)buffer[i * 4  + 0] << ' '
			  << (int)buffer[i * 4  + 1] << ' '
			  << (int)buffer[i * 4  + 2] << std::endl;
	}

	image.close();

	//create the directx texture
    ID3D12Resource* textureBufferUploadHeap;

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    sampler.MipLODBias = 0;
    sampler.MaxAnisotropy = 0;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_RESOURCE_DESC textureDescription = {};
    textureDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDescription.Alignment = 0;
	textureDescription.Width = imageWidth;
	textureDescription.Height = imageHeight;
    textureDescription.DepthOrArraySize = 1;
    textureDescription.MipLevels = 1;
    textureDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDescription.SampleDesc.Count = 1;
    textureDescription.SampleDesc.Quality = 0;
    textureDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    textureDescription.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES textureBufferHeapProperties = {};
	textureBufferHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	textureBufferHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	textureBufferHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	textureBufferHeapProperties.CreationNodeMask = 1;
	textureBufferHeapProperties.VisibleNodeMask = 1;


	ThrowIfFailed(device->CreateCommittedResource(
			&textureBufferHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&textureDescription,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&textureBuffer)));
    textureBuffer->SetName(L"Font Texture Buffer Resource Heap");

    UINT64 textureUploadBufferSize;
    device->GetCopyableFootprints(&textureDescription, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

	D3D12_HEAP_PROPERTIES textureBufferUploadHeapProperties = textureBufferHeapProperties;
	textureBufferUploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC bufferDescription = {};
    bufferDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDescription.Alignment = 0;
    bufferDescription.Width = textureUploadBufferSize;
    bufferDescription.Height = 1;
    bufferDescription.DepthOrArraySize = 1;
    bufferDescription.MipLevels = 1;
    bufferDescription.Format = DXGI_FORMAT_UNKNOWN;
    bufferDescription.SampleDesc.Count = 1;
    bufferDescription.SampleDesc.Quality = 0;
    bufferDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDescription.Flags = D3D12_RESOURCE_FLAG_NONE;

	ThrowIfFailed(device->CreateCommittedResource(
			&textureBufferUploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&bufferDescription,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&textureBufferUploadHeap)));
    textureBufferUploadHeap->SetName(L"Font Texture Buffer Upload Resource Heap");


    ID3D12CommandQueue* commandQueue;
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));
	commandQueue->SetName(L"Upload command queue");

    ID3D12CommandAllocator* commandAllocator;
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));


	D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = &buffer[0];
    textureData.RowPitch = imageWidth * 4;
    textureData.SlicePitch = imageWidth * 4 * imageHeight;

	ID3D12GraphicsCommandList* uploadCommandList;
	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
											commandAllocator, nullptr,
											IID_PPV_ARGS(&uploadCommandList)));
    UpdateSubresources(uploadCommandList, textureBuffer, textureBufferUploadHeap, 0, 0, 1, &textureData);
	auto transition = CD3DX12_RESOURCE_BARRIER::Transition(textureBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    uploadCommandList->ResourceBarrier(1, &transition);
    uploadCommandList->Close();

    ID3D12CommandList* ppCommandLists[] = {uploadCommandList};
    commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	HANDLE fenceEvent;
	ID3D12Fence* fence;
	UINT64 fenceValue = 1;

	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fenceEvent == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}

	ThrowIfFailed(commandQueue->Signal(fence, fenceValue++));

	// Wait until the previous frame is finished.
	if (fence->GetCompletedValue() < fenceValue - 1)
	{
		ThrowIfFailed(fence->SetEventOnCompletion(fenceValue - 1, fenceEvent));
		WaitForSingleObject(fenceEvent, INFINITE);
	}


	D3D12_DESCRIPTOR_RANGE1 range1[1];
    range1[0].BaseShaderRegister = 0;
    range1[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    range1[0].NumDescriptors = 1;
    range1[0].RegisterSpace = 0;
    range1[0].OffsetInDescriptorsFromTableStart = 0;
    range1[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

	D3D12_DESCRIPTOR_RANGE1 range2[1];
    range2[0].BaseShaderRegister = 0;
    range2[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range2[0].NumDescriptors = 1;
    range2[0].RegisterSpace = 0;
    range2[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    range2[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

    D3D12_ROOT_PARAMETER1 rootParameters[2];
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[0].DescriptorTable.pDescriptorRanges = range1;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[1].DescriptorTable.pDescriptorRanges = range2;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootSignatureDesc.Desc_1_1.NumParameters = 2;
    rootSignatureDesc.Desc_1_1.pParameters = rootParameters;
    rootSignatureDesc.Desc_1_1.NumStaticSamplers = 1;
    rootSignatureDesc.Desc_1_1.pStaticSamplers = &sampler;

    ID3DBlob* signature;
    ID3DBlob* error;

    try
    {
        ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error));
        ThrowIfFailed(
            device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
        rootSignature->SetName(L"Font Renderer Root Signature");
    }
    catch (std::exception e)
    {
        const char* errStr = (const char*)error->GetBufferPointer();
        std::cout << errStr;
        error->Release();
        error = nullptr;
		return false;
    }

    if(signature)
    {
        signature->Release();
        signature = nullptr;
    }

	// Vertex data for the triangle
    float vertexData[12] =
    {
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
    };


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
	
	D3D12_DESCRIPTOR_HEAP_DESC shaderDescHeapDesc = {};
	shaderDescHeapDesc.NumDescriptors = 257;
	shaderDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	shaderDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ThrowIfFailed(device->CreateDescriptorHeap(&shaderDescHeapDesc,
											   IID_PPV_ARGS(&shaderDescriptorHeap)));
	shaderDescriptorHeap->SetName(L"Descriptor Heap For SRV (font bitmap atlas)");

	D3D12_HEAP_PROPERTIES cbHeapProperties;
	cbHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	cbHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	cbHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	cbHeapProperties.CreationNodeMask = 1;
	cbHeapProperties.VisibleNodeMask = 1;


	D3D12_RESOURCE_DESC cbResourceDesc;
	cbResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbResourceDesc.Alignment = 0;
	cbResourceDesc.Width = (sizeof(Character) + 255) & ~255;
	cbResourceDesc.Height = 1;
	cbResourceDesc.DepthOrArraySize = 1;
	cbResourceDesc.MipLevels = 1;
	cbResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	cbResourceDesc.SampleDesc.Count = 1;
	cbResourceDesc.SampleDesc.Quality = 0;
	cbResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	cbResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	constantBuffersSize = 256;
	constantBuffers = new ID3D12Resource*[constantBuffersSize];
	for(int i = 0; i < constantBuffersSize; i++)
	{
		ThrowIfFailed(device->CreateCommittedResource(
			&cbHeapProperties, D3D12_HEAP_FLAG_NONE, &cbResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constantBuffers[i])));
	}

	int defaultChar = static_cast<int>('!');
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constantBuffers[defaultChar]->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = (sizeof(Character) + 255) & ~255; // CB size is required to be 256-byte aligned.

	D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle(shaderDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	cbvHandle.ptr = cbvHandle.ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 1;

	device->CreateConstantBufferView(&cbvDesc, cbvHandle);

	D3D12_RANGE CBReadRange;
	CBReadRange.Begin = 0;
	CBReadRange.End = 0;

	Character charac = Characters[defaultChar];

	ThrowIfFailed(constantBuffers[defaultChar]->Map(
		0, &CBReadRange, reinterpret_cast<void**>(&mappedConstantBuffer)));
	memcpy(mappedConstantBuffer, &charac, sizeof(Character));


	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureDescription.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle(shaderDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	srvHandle.ptr = srvHandle.ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 0;

	device->CreateShaderResourceView(textureBuffer, &srvDesc, srvHandle);

    ShaderCompiler compiler;

    CComPtr<ID3DBlob> vsShader, pxShader;
    bool v = compiler.CompileVertexShader(L"../Shaders/font.vert.hlsl", vsShader);
    bool p = compiler.CompilePixelShader(L"../Shaders/font.px.hlsl", pxShader);
    if(!v || !p)
    {
        return false;
    }

	////////////////////////////////////
	
	D3D12_SHADER_BYTECODE vsBytecode;
	vsBytecode.BytecodeLength = vsShader->GetBufferSize();
	vsBytecode.pShaderBytecode = vsShader->GetBufferPointer();

	D3D12_SHADER_BYTECODE psBytecode;
	psBytecode.BytecodeLength = pxShader->GetBufferSize();
	psBytecode.pShaderBytecode = pxShader->GetBufferPointer();

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
	//psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
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
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;

	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;


	D3D12_BLEND_DESC AlphaBlend =
	{
		FALSE, // AlphaToCoverageEnable
		FALSE, // IndependentBlendEnable
		{
			{
				TRUE, // BlendEnable
				FALSE, // LogicOpEnable
				D3D12_BLEND_ONE, // SrcBlend
				D3D12_BLEND_INV_SRC_ALPHA, // DestBlend
				D3D12_BLEND_OP_ADD, // BlendOp
				D3D12_BLEND_ONE, // SrcBlendAlpha
				D3D12_BLEND_INV_SRC_ALPHA, // DestBlendAlpha
				D3D12_BLEND_OP_ADD, // BlendOpAlpha
				D3D12_LOGIC_OP_NOOP,
				D3D12_COLOR_WRITE_ENABLE_ALL
			}
		}
	};
	psoDesc.BlendState = AlphaBlend;

	try
	{
		ThrowIfFailed(device->CreateGraphicsPipelineState(
			&psoDesc, IID_PPV_ARGS(&pipelineState)));
	}
	catch (std::exception e)
	{
		std::cout << "Failed to create Graphics Pipeline! " << e.what();
		return false;
	}

	return true;
}

void FontRenderer::RenderText(ID3D12GraphicsCommandList* commandList, std::string text, glm::ivec2 startCanvasPos)
{
	commandList->SetPipelineState(pipelineState);

	commandList->SetGraphicsRootSignature(rootSignature);

	commandList->SetDescriptorHeaps(1, &shaderDescriptorHeap);

	D3D12_GPU_DESCRIPTOR_HANDLE srvHandle(shaderDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	commandList->SetGraphicsRootDescriptorTable(1, srvHandle);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	//commandList->IASetIndexBuffer(&indexBufferView);

	UINT descriptorHeapIncrementSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for(int i = 0; i < text.size(); i++)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = constantBuffers[i]->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = (sizeof(Character) + 255) & ~255; // CB size is required to be 256-byte aligned.

		D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle(shaderDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		cbvHandle.ptr = cbvHandle.ptr + (descriptorHeapIncrementSize * (i + 1));

		Device->CreateConstantBufferView(&cbvDesc, cbvHandle);

		D3D12_GPU_DESCRIPTOR_HANDLE constantBufferHandle(shaderDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		constantBufferHandle.ptr = constantBufferHandle.ptr + (descriptorHeapIncrementSize * (i + 1));
		commandList->SetGraphicsRootDescriptorTable(0, constantBufferHandle);

		D3D12_RANGE CBReadRange;
		CBReadRange.Begin = 0;
		CBReadRange.End = 0;

		char c = text[i];
		Character charac = Characters[c];
		charac.CanvasPos = startCanvasPos;

		ThrowIfFailed(constantBuffers[i]->Map(
			0, &CBReadRange, reinterpret_cast<void**>(&mappedConstantBuffer)));
		memcpy(mappedConstantBuffer, &charac, sizeof(Character));

		commandList->DrawInstanced(4, 1, 0, 0);
		startCanvasPos.x += charac.Advance >> 6;
	}

}
