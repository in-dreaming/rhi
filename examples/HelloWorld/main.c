#include <rhi/rhi.h>
#include <rhi/defs.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Vertex
{
    float x, y, z;
    float r, g, b;
} Vertex;

static const Vertex kVertices[] = {
    {  0.0f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f },
    {  0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f },
    { -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f },
};

static RHI_Device           *g_device     = NULL;
static RHI_Swapchain        *g_swapchain  = NULL;
static RHI_Buffer           *g_vertex_buf = NULL;
static RHI_Buffer           *g_upload_buf = NULL;
static RHI_GraphicsPipeline *g_pipeline   = NULL;
static SDL_Window           *g_window     = NULL;

static bool load_shader_code(const char *path, uint8_t **outData, size_t *outSize)
{
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = (uint8_t *)malloc(sz);
    if (!buf) { fclose(f); return false; }
    fread(buf, 1, sz, f);
    fclose(f);
    *outData = buf;
    *outSize = (size_t)sz;
    return true;
}

static const char *shader_ext_for_backend(RHI_Backend backend)
{
    switch (backend) {
    case RHI_BACKEND_VULKAN: return "spv";
    case RHI_BACKEND_D3D12:  return "dxil";
    case RHI_BACKEND_METAL:  return "msl";
    default:                 return "spv";
    }
}

static bool try_load_shader(const char *ext, const char *entry_suffix, uint8_t **outCode, size_t *outSize)
{
    char path[512];
    const char *base = SDL_GetBasePath();

    snprintf(path, sizeof(path), "%s/shaders/tri-%s.%s", base ? base : ".", entry_suffix, ext);
    if (load_shader_code(path, outCode, outSize)) return true;

    snprintf(path, sizeof(path), "examples/HelloWorld/shaders/tri-%s.%s", entry_suffix, ext);
    if (load_shader_code(path, outCode, outSize)) return true;

    snprintf(path, sizeof(path), "shaders/tri-%s.%s", entry_suffix, ext);
    if (load_shader_code(path, outCode, outSize)) return true;

    return false;
}

static RHI_GraphicsPipeline *create_pipeline(RHI_Device *device)
{
    const char *ext = shader_ext_for_backend(RHI_GetDeviceBackend(device));

    uint8_t *vsCode = NULL;
    size_t vsSize = 0;
    if (!try_load_shader(ext, "vert", &vsCode, &vsSize)) {
        fprintf(stderr, "Cannot load vertex shader (.%s)\n", ext);
        return NULL;
    }

    RHI_ShaderModule *vs = RHI_CreateShaderModule(device, &(RHI_ShaderModuleCreateInfo){
        .code = vsCode,
        .codeSize = vsSize,
        .stage = RHI_SHADER_STAGE_VERTEX,
        .entryPoint = "main",
    });
    free(vsCode);
    if (!vs) { fprintf(stderr, "Failed to create vertex shader module\n"); return NULL; }

    uint8_t *fsCode = NULL;
    size_t fsSize = 0;
    if (!try_load_shader(ext, "frag", &fsCode, &fsSize)) {
        fprintf(stderr, "Cannot load fragment shader (.%s)\n", ext);
        RHI_DestroyShaderModule(device, vs);
        free(vsCode);
        return NULL;
    }

    RHI_ShaderModule *fs = RHI_CreateShaderModule(device, &(RHI_ShaderModuleCreateInfo){
        .code = fsCode,
        .codeSize = fsSize,
        .stage = RHI_SHADER_STAGE_FRAGMENT,
        .entryPoint = "main",
    });
    free(fsCode);
    if (!fs) { fprintf(stderr, "Failed to create fragment shader module\n"); RHI_DestroyShaderModule(device, vs); return NULL; }

    RHI_VertexBinding vb = { .binding = 0, .stride = sizeof(Vertex), .perInstance = false };
    RHI_VertexAttribute attrs[2] = {
        { .location = 0, .binding = 0, .format = RHI_VERTEX_FORMAT_FLOAT3, .offset = 0 },
        { .location = 1, .binding = 0, .format = RHI_VERTEX_FORMAT_FLOAT3, .offset = 12 },
    };

    RHI_Format rtFormat = RHI_FORMAT_BGRA8_UNORM;
    RHI_BlendState blend = {0};

    RHI_GraphicsPipeline *pipe = RHI_CreateGraphicsPipeline(device, &(RHI_GraphicsPipelineCreateInfo){
        .vertexShader = vs,
        .fragmentShader = fs,
        .vertexBindings = &vb,
        .vertexBindingCount = 1,
        .vertexAttributes = attrs,
        .vertexAttributeCount = 2,
        .cullMode = RHI_CULL_NONE,
        .frontFace = RHI_FRONT_CCW,
        .depthTestEnable = false,
        .depthWriteEnable = false,
        .depthCompareOp = RHI_COMPARE_ALWAYS,
        .blendState = blend,
        .renderTargetCount = 1,
        .renderTargetFormats = &rtFormat,
        .depthStencilFormat = RHI_FORMAT_UNDEFINED,
    });

    RHI_DestroyShaderModule(device, vs);
    RHI_DestroyShaderModule(device, fs);
    return pipe;
}

static bool init(void)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    RHI_DeviceCreateInfo devInfo = {
        .backend = RHI_BACKEND_VULKAN,
        .enableValidation = true,
        .enableBindless = false,
        .enableMeshShader = false,
        .enableRayTracing = false,
        .applicationName = "HelloWorld",
        .applicationVersion = 1,
    };

    g_device = RHI_CreateDevice(&devInfo);
    if (!g_device) {
        fprintf(stderr, "Vulkan not available, trying D3D12...\n");
        devInfo.backend = RHI_BACKEND_D3D12;
        g_device = RHI_CreateDevice(&devInfo);
    }
    if (!g_device) {
        fprintf(stderr, "Failed to create RHI device\n");
        return false;
    }
    printf("RHI device created: backend=%d\n", RHI_GetDeviceBackend(g_device));

    SDL_Window *window = SDL_CreateWindow("RHI HelloWorld", 800, 600, 0);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }
    g_window = window;

    RHI_SwapchainCreateInfo scInfo = {
        .window = window,
        .width = 800,
        .height = 600,
        .vsync = true,
        .format = RHI_FORMAT_BGRA8_UNORM,
    };
    g_swapchain = RHI_CreateSwapchain(g_device, &scInfo);
    if (!g_swapchain) return false;

    g_upload_buf = RHI_CreateBuffer(g_device, &(RHI_BufferCreateInfo){
        .size = sizeof(kVertices),
        .usage = 0,
        .memoryType = RHI_MEMORY_HOST_VISIBLE,
        .name = "Upload",
    });
    if (!g_upload_buf) return false;

    void *mapped = RHI_MapBuffer(g_upload_buf, 0, sizeof(kVertices));
    if (mapped) {
        memcpy(mapped, kVertices, sizeof(kVertices));
        RHI_UnmapBuffer(g_upload_buf);
    }

    g_vertex_buf = RHI_CreateBuffer(g_device, &(RHI_BufferCreateInfo){
        .size = sizeof(kVertices),
        .usage = RHI_BUFFER_USAGE_VERTEX,
        .memoryType = RHI_MEMORY_DEVICE_LOCAL,
        .name = "Vertices",
    });
    if (!g_vertex_buf) return false;

    {
        RHI_CommandBuffer *cmd = RHI_AcquireCommandBuffer(g_device, RHI_QUEUE_GRAPHICS);
        if (!cmd) return false;
        RHI_BeginCommandBuffer(cmd);

        RHI_TransferBufferLocation src = { .transferBuffer = g_upload_buf, .offset = 0 };
        RHI_BufferRegion dst = { .buffer = g_vertex_buf, .offset = 0, .size = (uint32_t)sizeof(kVertices) };
        RHI_CmdUploadBuffer(cmd, &src, &dst);

        RHI_EndCommandBuffer(cmd);
        RHI_SubmitInfo submitInfo = {
            .commandBuffers = &cmd,
            .commandBufferCount = 1,
        };
        RHI_Queue *queue = RHI_GetQueue(g_device, RHI_QUEUE_GRAPHICS, 0);
        RHI_QueueSubmit(queue, &submitInfo, NULL);
        RHI_DeviceWaitIdle(g_device);
        free(cmd);
    }

    g_pipeline = create_pipeline(g_device);
    if (!g_pipeline) {
        fprintf(stderr, "Warning: pipeline creation failed\n");
    }

    return true;
}

static void shutdown(void)
{
    RHI_DeviceWaitIdle(g_device);
    if (g_pipeline)    RHI_DestroyGraphicsPipeline(g_device, g_pipeline);
    if (g_vertex_buf)  RHI_DestroyBuffer(g_device, g_vertex_buf);
    if (g_upload_buf)  RHI_DestroyBuffer(g_device, g_upload_buf);
    if (g_swapchain)   RHI_DestroySwapchain(g_device, g_swapchain);
    if (g_window)      SDL_DestroyWindow(g_window);
    if (g_device)      RHI_DestroyDevice(g_device);
    SDL_Quit();
}

int main(int argc, char *argv[])
{
    if (!init()) {
        fprintf(stderr, "Init failed\n");
        shutdown();
        return 1;
    }

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) running = false;
        }

        RHI_CommandBuffer *cmd = RHI_AcquireCommandBuffer(g_device, RHI_QUEUE_GRAPHICS);
        if (!cmd) continue;

        RHI_BeginCommandBuffer(cmd);

        RHI_SetSwapchainCommandBuffer(g_swapchain, cmd);
        RHI_Texture *swapTex = RHI_GetSwapchainTexture(g_swapchain);
        if (swapTex && g_pipeline) {
            RHI_ColorAttachmentDesc colorTarget = {0};
            colorTarget.texture = swapTex;
            colorTarget.loadOp  = RHI_LOAD_OP_CLEAR;
            colorTarget.storeOp = RHI_STORE_OP_STORE;
            colorTarget.clearR  = 0.1f;
            colorTarget.clearG  = 0.1f;
            colorTarget.clearB  = 0.1f;
            colorTarget.clearA  = 1.0f;

            RHI_CmdBeginRenderPass(cmd, &colorTarget, 1, NULL);
            RHI_CmdBindGraphicsPipeline(cmd, g_pipeline);
            RHI_CmdBindVertexBuffer(cmd, 0, g_vertex_buf, 0);
            RHI_CmdDraw(cmd, 3, 1, 0, 0);
            RHI_CmdEndRenderPass(cmd);
        }

        RHI_EndCommandBuffer(cmd);

        RHI_SubmitInfo submitInfo = {
            .commandBuffers = &cmd,
            .commandBufferCount = 1,
        };
        RHI_Queue *queue = RHI_GetQueue(g_device, RHI_QUEUE_GRAPHICS, 0);
        RHI_QueueSubmit(queue, &submitInfo, NULL);
        RHI_Present(queue, g_swapchain);

        free(cmd);
        free(swapTex);
    }

    shutdown();
    return 0;
}
