#include <slang-com-ptr.h>
#include "../Common/Config.h"

#include <d3dcompiler.h>
#include <d3dx12.h>
#include <dxcapi.h>
#include <slang-com-helper.h>
#include <slang.h>
#include <windows.h>
#include <wrl.h>
#include "SlangCompiler.h"
#include "Util/Logger.h"

#pragma comment(lib, "slang.lib")
#pragma comment(lib, "d3dcompiler.lib")

static Slang::ComPtr<slang::IGlobalSession> gSlangGlobalSession;

void initSlang() {
    if (!gSlangGlobalSession) {
        SlangResult result = slang::createGlobalSession(gSlangGlobalSession.writeRef());
        if (SLANG_FAILED(result)) {
            VT_ERROR("Failed to create Slang");
            exit(1);
        }
    }
}

void shutdownSlang() { gSlangGlobalSession = nullptr; }

void load_slang_shader(const char* pFileName, ShaderStage stage, ID3DBlob** ppBlob) {
    *ppBlob = nullptr;
    if (!gSlangGlobalSession) {
        VT_ERROR("Call initSlang() first");
        return;
    }

    Slang::ComPtr<slang::ISession> session;
    gSlangGlobalSession->createSession({}, session.writeRef());

    Slang::ComPtr<slang::IBlob> diagnosticsBlob;
    slang::IModule* module = session->loadModule(pFileName, diagnosticsBlob.writeRef());

    if (diagnosticsBlob && diagnosticsBlob->getBufferSize() > 0) {
        VT_WARN("Slang compilation %s:\n%s",
                pFileName,
                (char*) diagnosticsBlob->getBufferPointer());
    }
    if (!module)
        return;

    const char* entryPointName = (stage == SHADER_STAGE_VERTEX) ? "VS" : "PS";
    Slang::ComPtr<slang::IEntryPoint> entryPoint;
    SlangResult result = module->findEntryPointByName(entryPointName, entryPoint.writeRef());
    if (SLANG_FAILED(result)) {
        VT_ERROR("Failed to find entry point '%s' in %s", entryPointName, pFileName);
        return;
    }

    slang::IComponentType* components[] = { module, entryPoint.get() };
    Slang::ComPtr<slang::IComponentType> program;
    result = session->createCompositeComponentType(components,
                                                   2,
                                                   program.writeRef(),
                                                   diagnosticsBlob.writeRef());

    if (diagnosticsBlob && diagnosticsBlob->getBufferSize() > 0) {
        VT_ERROR("Slang link error %s:\n%s",
                 pFileName,
                 (char*) diagnosticsBlob->getBufferPointer());
    }
    if (SLANG_FAILED(result))
        return;

    Slang::ComPtr<slang::IBlob> codeBlob;
    result = program->getEntryPointCode(0, 0, codeBlob.writeRef(), diagnosticsBlob.writeRef());
    if (diagnosticsBlob && diagnosticsBlob->getBufferSize() > 0) {
        VT_ERROR("Slang codegen error %s:\n%s",
                 pFileName,
                 (char*) diagnosticsBlob->getBufferPointer());
    }
    if (SLANG_FAILED(result))
        return;

    D3DCreateBlob(codeBlob->getBufferSize(), ppBlob);
    memcpy((*ppBlob)->GetBufferPointer(), codeBlob->getBufferPointer(), codeBlob->getBufferSize());
}
