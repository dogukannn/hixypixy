#pragma once
#include <atlbase.h>
#include <d3dcommon.h>
#include <dxcapi.h>

class ShaderCompiler
{
public:
	ShaderCompiler();

	bool CompileVertexShader(LPCWSTR shaderPath, CComPtr<ID3DBlob>& outShader, LPCWSTR shaderName = L"") const;
	bool CompilePixelShader(LPCWSTR shaderPath, CComPtr<ID3DBlob>& outShader, LPCWSTR shaderName = L"") const;

	CComPtr<IDxcUtils> Utils;
	CComPtr<IDxcCompiler3> Compiler;
	CComPtr<IDxcIncludeHandler> IncludeHandler;

private:
	bool CompileShader(LPCWSTR* args, UINT argSize, LPCWSTR shaderPath, LPCWSTR shaderName, CComPtr<ID3DBlob>& outShader, CComPtr<
	                   struct IDxcBlobUtf16>& outShaderName) const;
};

