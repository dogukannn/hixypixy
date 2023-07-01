#pragma once
#include <map>
#include "directx/d3dx12.h"
#include "glm/glm.hpp"
#include "ShaderCompiler.h"

struct Character
{
	glm::mat4 Projection;
	glm::ivec2 AtlasPos;
	glm::ivec2 AtlasResolution;
	glm::ivec2 Size;
	glm::ivec2 Bearing;
	glm::ivec2 CanvasPos;
	UINT Advance;
};

class FontRenderer
{
public:
	FontRenderer();

	bool Initialize(ID3D12Device* device, glm::ivec2 windowSize);
	void RenderText(::ID3D12GraphicsCommandList* commandList, ::std::string text, glm::ivec2 startCanvasPos);

	ID3D12Device* Device;
    ID3D12Resource* textureBuffer;

    ID3D12RootSignature* rootSignature;

    ID3D12Resource* vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	ID3D12Resource** constantBuffers;
	UINT constantBuffersSize;
	UINT8* mappedConstantBuffer;

	ID3D12DescriptorHeap* shaderDescriptorHeap;
	ID3D12DescriptorHeap* charDescriptorHeap;

	ID3D12PipelineState* pipelineState;

	glm::ivec2 WindowSize;

	std::map<char, Character> Characters;
};
