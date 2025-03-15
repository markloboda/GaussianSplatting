#pragma once

#include <webgpu/webgpu.h>

#include <Core/Core.h>

class Camera;
struct GLFWwindow;

struct ShaderUniforms {
   alignas(64) mat4x4 model;
   alignas(64) mat4x4 view;
   alignas(64) mat4x4 projection;
   alignas(4) float splatScale;
};

struct SortSplatsData {
   alignas(4) u32 index;
   alignas(4) f32 z;
};

struct PerformanceData {
   uint32 pointCount = 0;
   float frameTime = 0.0f;
   float sortTime = 0.0f;
   float renderTime = 0.0f;
};

struct Splat;

class Renderer {
private:
   WGPUTextureFormat _surfaceFormat = WGPUTextureFormat_RGBA8Unorm;

   WGPUDevice _wgpuDevice = nullptr;
   WGPUQueue _wgpuQueue = nullptr;
   WGPUSurface _wgpuSurface = nullptr;
   WGPUComputePipeline _wgpuTransformComputePipeline = nullptr;
   WGPUComputePipeline _wgpuSortComputePipeline = nullptr;
   WGPURenderPipeline _wgpuRenderPipeline = nullptr;
   WGPUBuffer _splatsBuffer = nullptr;
   WGPUBuffer _sortedSplatsBuffer = nullptr;
   WGPUBuffer _sortSplatsParamsDataBuffer = nullptr;
   WGPUBuffer _sortSplatsParamsUniform = nullptr;
   WGPUBuffer _uniformBuffer = nullptr;
   WGPUPipelineLayout _wgpuPipelineLayout = nullptr;
   WGPUBindGroupLayout _sceneBindGroupLayout = nullptr;
   WGPUBindGroup _sceneBindGroup = nullptr;
   WGPUBindGroupLayout _stateBindGroupLayout = nullptr;
   WGPUBindGroup _stateBindGroup = nullptr;

   ///////////////////////////
   /// Are released after initialization but are needed during initialization.
   WGPUInstance _wgpuInstance = nullptr;
   WGPUAdapter _wgpuAdapter = nullptr;
   ///////////////////////////

   std::vector<Splat> _rawSplatsData;
   std::vector<Splat> _splatsData;
   mat4x4 _modelMatrix = identity<mat4x4>();

   u32vec2 _viewPortSize = u32vec2{0, 0};

   std::vector<uvec2> _sortSplatsParamsData;

   // Settings:
   int _workGroupSize = 256;
   float _splatScale = 0.15f;
   PerformanceData _performanceData;
public:
   bool FreeCamera = false;
   bool ChangeSplatsFlag = false;
   std::string SelectedFile;
   int SelectedFileIndex = 0;

public:
   bool Initialize(GLFWwindow *window, int windowWidth, int windowHeight, const char* filename);

   void Terminate();

   void Render(const Camera &camera);

   vec4 GetModelPosition() const;

private:
   bool InitializeImGui(GLFWwindow* window);
   void ReleaseImGui();
   void ImGuiBeginFrame();
   void ImGuiEndFrame(WGPURenderPassEncoder encoder);
   void RenderImGuiUI();

   // Initialization functions.
   bool CreateWGPUInstance();
   bool CreateWGPUSurface(GLFWwindow* window);
   bool GetAdapterAndDevice();
   void RequestQueue();
   void ConfigureSurface();
   void InitializeBuffers();
   void InitializeComputePipelines(WGPUShaderModule shaderModule);
   void InitializeRenderPipeline(WGPUShaderModule shaderModule);

   // Rendering functions.
   void UpdateUniforms(const Camera& camera) const;
   WGPUCommandEncoder* CreateCommandEncoder() const;

   // Compute pass functions.
   WGPUComputePassEncoder* BeginComputePass(WGPUCommandEncoder encoder) const;

   // Render pass functions.
   WGPUSurfaceTexture* GetNextSurfaceTexture() const;
   WGPUTextureView* CreateTextureView(WGPUTexture texture) const;
   WGPURenderPassEncoder* BeginRenderPass(WGPUCommandEncoder encoder, WGPUTextureView textureView) const;

   // Release functions.
   WGPUCommandBuffer* FinishAndReleaseCommandEncoder(WGPUCommandEncoder encoder) const;
};
