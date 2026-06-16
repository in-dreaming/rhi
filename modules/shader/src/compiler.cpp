#include "shader/compiler.h"
#include <slang.h>
#include <slang-com-ptr.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

using Slang::ComPtr;

struct ShaderCompiler {
    slang::IGlobalSession *globalSession;
};

static void setResultError(ShaderCompileResult *result, const char *msg)
{
    result->success = false;
    size_t len = strlen(msg);
    result->errorMessage = (char *)malloc(len + 1);
    if (result->errorMessage) {
        memcpy(result->errorMessage, msg, len + 1);
    }
}

static void writeBlobToResult(slang::IBlob *blob, uint8_t **outData, size_t *outSize)
{
    if (!blob) return;
    size_t sz = blob->getBufferSize();
    if (sz == 0) return;
    void *buf = malloc(sz);
    if (!buf) return;
    memcpy(buf, blob->getBufferPointer(), sz);
    *outData = (uint8_t *)buf;
    *outSize = sz;
}

ShaderCompiler *ShaderCompiler_Create(void)
{
    ShaderCompiler *compiler = (ShaderCompiler *)calloc(1, sizeof(ShaderCompiler));
    if (!compiler) return NULL;

    SlangResult res = slang::createGlobalSession(&compiler->globalSession);
    if (SLANG_FAILED(res)) {
        free(compiler);
        return NULL;
    }

    return compiler;
}

void ShaderCompiler_Destroy(ShaderCompiler *compiler)
{
    if (!compiler) return;
    if (compiler->globalSession) {
        compiler->globalSession->release();
    }
    free(compiler);
}

static ShaderCompileResult compileInternal(ShaderCompiler *compiler,
                                            const char *source,
                                            size_t sourceSize,
                                            const char *pathHint,
                                            const char *entryPoint)
{
    ShaderCompileResult result = {0};

    slang::SessionDesc sessionDesc = {};
    slang::TargetDesc targetDescs[4] = {};

    targetDescs[0].format = SLANG_SPIRV;
    targetDescs[0].profile = compiler->globalSession->findProfile("spirv_1_5");

    targetDescs[1].format = SLANG_DXIL;
    targetDescs[1].profile = compiler->globalSession->findProfile("sm_6_6");

    targetDescs[2].format = SLANG_METAL_LIB;
    targetDescs[2].profile = compiler->globalSession->findProfile("metallib_3_0");

    targetDescs[3].format = SLANG_WGSL;
    targetDescs[3].profile = compiler->globalSession->findProfile("wgsl");

    sessionDesc.targets = targetDescs;
    sessionDesc.targetCount = 4;

    ComPtr<slang::ISession> session;
    SlangResult res = compiler->globalSession->createSession(sessionDesc, session.writeRef());
    if (SLANG_FAILED(res)) {
        setResultError(&result, "Failed to create Slang session");
        return result;
    }

    ComPtr<slang::IModule> module;
    ComPtr<slang::IBlob> loadDiagnostics;
    if (pathHint && pathHint[0] != '\0') {
        module = session->loadModuleFromSourceString(
            pathHint, pathHint, source, loadDiagnostics.writeRef());
    } else {
        module = session->loadModuleFromSourceString(
            "shader", "shader", source, loadDiagnostics.writeRef());
    }

    if (loadDiagnostics) {
        const char *diagStr = (const char *)loadDiagnostics->getBufferPointer();
        if (diagStr && diagStr[0] != '\0') {
            setResultError(&result, diagStr);
            return result;
        }
    }

    if (!module) {
        setResultError(&result, "Failed to load Slang module");
        return result;
    }

    SlangInt entryPointCount = module->getDefinedEntryPointCount();
    ComPtr<slang::IEntryPoint> entryPointObj;
    bool found = false;

    for (SlangInt i = 0; i < entryPointCount; i++) {
        SlangResult epRes = module->getDefinedEntryPoint(i, entryPointObj.writeRef());
        if (SLANG_SUCCEEDED(epRes)) {
            found = true;
            break;
        }
    }

    if (!found) {
        setResultError(&result, "No entry point found");
        return result;
    }

    slang::IComponentType *components[] = { module, entryPointObj };
    ComPtr<slang::IComponentType> program;
    ComPtr<slang::IBlob> compositeDiag;
    session->createCompositeComponentType(components, 2, program.writeRef(), compositeDiag.writeRef());

    ComPtr<slang::IComponentType> linked;
    ComPtr<slang::IBlob> linkDiagnostics;
    program->link(linked.writeRef(), linkDiagnostics.writeRef());

    if (linkDiagnostics) {
        const char *diagStr = (const char *)linkDiagnostics->getBufferPointer();
        if (diagStr && diagStr[0] != '\0') {
            setResultError(&result, diagStr);
            return result;
        }
    }

    for (SlangInt t = 0; t < 4; t++) {
        slang::IBlob *pTargetBlob = nullptr;
        slang::IBlob *pTargetDiag = nullptr;
        SlangResult codeRes = linked->getTargetCode(t, &pTargetBlob, &pTargetDiag);

        if (SLANG_SUCCEEDED(codeRes) && pTargetBlob) {
            switch (t) {
            case 0: writeBlobToResult(pTargetBlob, &result.spvData, &result.spvSize); break;
            case 1: writeBlobToResult(pTargetBlob, &result.dxilData, &result.dxilSize); break;
            case 2: writeBlobToResult(pTargetBlob, &result.mslData, &result.mslSize); break;
            case 3: writeBlobToResult(pTargetBlob, &result.wgslData, &result.wgslSize); break;
            }
            pTargetBlob->release();
        }
        if (pTargetDiag) pTargetDiag->release();
    }

    result.success = true;
    return result;
}

ShaderCompileResult ShaderCompiler_CompileFile(ShaderCompiler *compiler,
                                                const char *slangPath,
                                                const char *entryPoint)
{
    ShaderCompileResult result = {0};
    if (!compiler || !slangPath) {
        setResultError(&result, "Invalid parameters");
        return result;
    }

    FILE *f = fopen(slangPath, "rb");
    if (!f) {
        setResultError(&result, "Cannot open file");
        return result;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *src = (char *)malloc(sz + 1);
    if (!src) {
        fclose(f);
        setResultError(&result, "Out of memory");
        return result;
    }

    fread(src, 1, sz, f);
    src[sz] = '\0';
    fclose(f);

    result = compileInternal(compiler, src, sz, slangPath, entryPoint);
    free(src);
    return result;
}

ShaderCompileResult ShaderCompiler_CompileSource(ShaderCompiler *compiler,
                                                  const char *source,
                                                  size_t sourceSize,
                                                  const char *pathHint,
                                                  const char *entryPoint)
{
    ShaderCompileResult result = {0};
    if (!compiler || !source) {
        setResultError(&result, "Invalid parameters");
        return result;
    }
    return compileInternal(compiler, source, sourceSize, pathHint ? pathHint : "", entryPoint);
}

void ShaderCompileResult_Free(ShaderCompileResult *result)
{
    if (!result) return;
    free(result->errorMessage);
    free(result->spvData);
    free(result->dxilData);
    free(result->mslData);
    free(result->wgslData);
    free(result->reflectionJson);
    memset(result, 0, sizeof(*result));
}
