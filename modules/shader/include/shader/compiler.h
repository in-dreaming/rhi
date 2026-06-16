#ifndef SHADER_COMPILER_H
#define SHADER_COMPILER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ShaderCompiler ShaderCompiler;

typedef struct ShaderCompileResult {
    bool    success;
    char   *errorMessage;
    uint8_t *spvData;
    size_t  spvSize;
    uint8_t *dxilData;
    size_t  dxilSize;
    uint8_t *mslData;
    size_t  mslSize;
    uint8_t *wgslData;
    size_t  wgslSize;
    char   *reflectionJson;
    size_t  reflectionJsonSize;
} ShaderCompileResult;

ShaderCompiler   *ShaderCompiler_Create(void);
void              ShaderCompiler_Destroy(ShaderCompiler *compiler);

ShaderCompileResult ShaderCompiler_CompileFile(ShaderCompiler *compiler,
                                               const char *slangPath,
                                               const char *entryPoint);

ShaderCompileResult ShaderCompiler_CompileSource(ShaderCompiler *compiler,
                                                  const char *source,
                                                  size_t sourceSize,
                                                  const char *path_hint,
                                                  const char *entryPoint);

void ShaderCompileResult_Free(ShaderCompileResult *result);

#ifdef __cplusplus
}
#endif

#endif
