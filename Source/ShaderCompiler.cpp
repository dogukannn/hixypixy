#include "ShaderCompiler.h"

ShaderCompiler::ShaderCompiler()
{
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&Utils));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&Compiler));
    Utils->CreateDefaultIncludeHandler(&IncludeHandler);
}

bool ShaderCompiler::CompileVertexShader(LPCWSTR shaderPath, CComPtr<ID3DBlob>& outShader, LPCWSTR shaderName) const
{
    LPCWSTR args[] =
    {
        shaderName,            // Optional shader source file name for error reporting and for PIX shader source view.  
        L"-E", L"main",              // Entry point.
        L"-T", L"vs_6_0",            // Target.
    };

    CComPtr<IDxcBlobUtf16> outShaderName;
    return CompileShader(args, _countof(args), shaderPath, shaderName, outShader, outShaderName);
}

bool ShaderCompiler::CompilePixelShader(LPCWSTR shaderPath, CComPtr<ID3DBlob>& outShader, LPCWSTR shaderName) const
{
    LPCWSTR args[] =
    {
        shaderName,                  // Optional shader source file name for error reporting and for PIX shader source view.  
        L"-E", L"main",              // Entry point.
        L"-T", L"ps_6_0",            // Target.
    };

    CComPtr<IDxcBlobUtf16> outShaderName;
    return CompileShader(args, _countof(args), shaderPath, shaderName, outShader, outShaderName);
}

bool ShaderCompiler::CompileShader(LPCWSTR* args, UINT argSize, LPCWSTR shaderPath, LPCWSTR shaderName, CComPtr<ID3DBlob>& outShader,
	CComPtr<IDxcBlobUtf16>& outShaderName) const
{
	CComPtr<IDxcBlobEncoding> pSource = nullptr;
    Utils->LoadFile(shaderPath, nullptr, &pSource);
    if(!pSource)
    {
        return false;
    }
    DxcBuffer Source;
    Source.Ptr = pSource->GetBufferPointer();
    Source.Size = pSource->GetBufferSize();
    Source.Encoding = DXC_CP_ACP;

    CComPtr<IDxcResult> results;
    Compiler->Compile(
        &Source,                // Source buffer.
        args,                // Array of pointers to arguments.
        argSize,      // Number of arguments.
        IncludeHandler,        // User-provided interface to handle #include directives (optional).
        IID_PPV_ARGS(&results) // Compiler output status, buffer, and errors.
    );

	CComPtr<IDxcBlobUtf8> pErrors = nullptr;
    results->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
    if (pErrors != nullptr && pErrors->GetStringLength() != 0)
        wprintf(L"Warnings and Errors:\n%S\n", pErrors->GetStringPointer());

    HRESULT hrStatus;
    results->GetStatus(&hrStatus);
    if (FAILED(hrStatus))
    {
        wprintf(L"Compilation Failed\n");
        return false;
    }

    results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&outShader), &outShaderName);
    return true;
}
