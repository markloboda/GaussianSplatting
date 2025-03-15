#include <GaussianSplatting.h>
#include <Application/Renderer.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_wgpu.h"

#include <chrono>

#include <webgpu/webgpu.hpp>

#include "glfw/deps/glad/gl.h"
#ifdef WEBGPU_BACKEND_WGPU
#include <webgpu/wgpu.h>
using namespace wgpu;
#endif // WEBGPU_BACKEND_WGPU
#include <glfw3webgpu.h>
#include <Utils/FileReader.h>

#include <Application/Camera.h>

void setDefault(WGPUBindGroupLayoutEntry &bindingLayout) {
   bindingLayout.buffer.nextInChain = nullptr;
   bindingLayout.buffer.type = WGPUBufferBindingType_Undefined;
   bindingLayout.buffer.hasDynamicOffset = false;

   bindingLayout.sampler.nextInChain = nullptr;
   bindingLayout.sampler.type = WGPUSamplerBindingType_Undefined;

   bindingLayout.storageTexture.nextInChain = nullptr;
   bindingLayout.storageTexture.access = WGPUStorageTextureAccess_Undefined;
   bindingLayout.storageTexture.format = WGPUTextureFormat_Undefined;
   bindingLayout.storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;

   bindingLayout.texture.nextInChain = nullptr;
   bindingLayout.texture.multisampled = false;
   bindingLayout.texture.sampleType = WGPUTextureSampleType_Undefined;
   bindingLayout.texture.viewDimension = WGPUTextureViewDimension_Undefined;
}

void setDefault(WGPULimits &limits) {
   limits.maxTextureDimension1D = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxTextureDimension2D = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxTextureDimension3D = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxTextureArrayLayers = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxBindGroups = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxBindGroupsPlusVertexBuffers = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxBindingsPerBindGroup = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxDynamicUniformBuffersPerPipelineLayout = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxDynamicStorageBuffersPerPipelineLayout = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxSampledTexturesPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxSamplersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxStorageBuffersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxStorageTexturesPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxUniformBuffersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxUniformBufferBindingSize = WGPU_LIMIT_U64_UNDEFINED;
   limits.maxStorageBufferBindingSize = WGPU_LIMIT_U64_UNDEFINED;
   limits.minUniformBufferOffsetAlignment = WGPU_LIMIT_U32_UNDEFINED;
   limits.minStorageBufferOffsetAlignment = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxVertexBuffers = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxBufferSize = WGPU_LIMIT_U64_UNDEFINED;
   limits.maxVertexAttributes = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxVertexBufferArrayStride = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxInterStageShaderComponents = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxInterStageShaderVariables = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxColorAttachments = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxColorAttachmentBytesPerSample = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxComputeWorkgroupStorageSize = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxComputeInvocationsPerWorkgroup = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxComputeWorkgroupSizeX = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxComputeWorkgroupSizeY = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxComputeWorkgroupSizeZ = WGPU_LIMIT_U32_UNDEFINED;
   limits.maxComputeWorkgroupsPerDimension = WGPU_LIMIT_U32_UNDEFINED;
}

void SetImGuiStyle() {
   ImGuiStyle& style = ImGui::GetStyle();
   style.WindowRounding = 5.0f;
   style.FrameRounding = 4.0f;
   style.GrabRounding = 3.0f;
   style.WindowBorderSize = 1.0f;
   style.FrameBorderSize = 1.0f;

   ImVec4 bgColor = ImVec4(0.1f, 0.1f, 0.1f, 0.85f);  // Semi-transparent dark
   ImVec4 accentColor = ImVec4(0.9f, 0.3f, 0.3f, 1.0f); // Red accent

   style.Colors[ImGuiCol_WindowBg] = bgColor;
   style.Colors[ImGuiCol_TitleBg] = accentColor;
   style.Colors[ImGuiCol_TitleBgActive] = accentColor;
   style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
   style.Colors[ImGuiCol_Button] = accentColor;
   style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
}

bool Renderer::Initialize(GLFWwindow *window, int windowWidth, int windowHeight, const char* filename) {
   _viewPortSize = vec2{static_cast<float>(windowWidth), static_cast<float>(windowHeight)};

   if (!CreateWGPUInstance())
   {
      return false;
   }

   if (!CreateWGPUSurface(window))
   {
      return false;
   }

   if (!GetAdapterAndDevice())
   {
      return false;
   }

   RequestQueue();
   ConfigureSurface();

   // Load splat data.
   SelectedFile = filename;
   if (!FileReader::LoadSplatData(SelectedFile, _rawSplatsData)) {
      std::cerr << "Failed to load splat data!" << std::endl;
      return false;
   }

   // Pre compute sort params for data size.
   for (u32 k = 2; k / 2 <= _rawSplatsData.size(); k *= 2)
   {
      for (u32 j = k / 2; j > 0; j /= 2) {
         _sortSplatsParamsData.push_back({ k,j });
      }
   }

   InitializeBuffers();

   // Load shader source.
   WGPUShaderModule shaderModule = FileReader::LoadShaderModule("../../../assets/shaders/gaussian_splatting.wgsl", _wgpuDevice);
   if (shaderModule == nullptr) {
      std::cerr << "Failed to load shader module!" << std::endl;
      __debugbreak();
      return false;
   }
   _performanceData.pointCount = static_cast<uint32>(_rawSplatsData.size());

   // Create a pipeline layout for all pipelines to use.
   WGPUBindGroupLayout bgLayouts[2] = { _sceneBindGroupLayout, _stateBindGroupLayout };

   WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {};
   pipelineLayoutDesc.nextInChain = nullptr;
   pipelineLayoutDesc.label = "WGPU Pipeline Layout";
   pipelineLayoutDesc.bindGroupLayoutCount = 2;
   pipelineLayoutDesc.bindGroupLayouts = bgLayouts;
   _wgpuPipelineLayout = wgpuDeviceCreatePipelineLayout(_wgpuDevice, &pipelineLayoutDesc);

   InitializeComputePipelines(shaderModule);
   InitializeRenderPipeline(shaderModule);

   // Release adapter and instance as they are not needed anymore.
   wgpuInstanceRelease(_wgpuInstance);
   wgpuAdapterRelease(_wgpuAdapter);

   InitializeImGui(window);

   return true;
}

void Renderer::Terminate() {
   ReleaseImGui();
   wgpuBindGroupRelease(_stateBindGroup);
   wgpuBindGroupLayoutRelease(_stateBindGroupLayout);
   wgpuBindGroupRelease(_sceneBindGroup);
   wgpuBindGroupLayoutRelease(_sceneBindGroupLayout);
   wgpuPipelineLayoutRelease(_wgpuPipelineLayout);
   wgpuBufferRelease(_uniformBuffer);
   wgpuBufferRelease(_sortSplatsParamsUniform);
   wgpuBufferRelease(_sortSplatsParamsDataBuffer);
   wgpuBufferRelease(_sortedSplatsBuffer);
   wgpuBufferRelease(_splatsBuffer);
   wgpuRenderPipelineRelease(_wgpuRenderPipeline);
   wgpuComputePipelineRelease(_wgpuSortComputePipeline);
   wgpuComputePipelineRelease(_wgpuTransformComputePipeline);
   wgpuSurfaceUnconfigure(_wgpuSurface);
   wgpuSurfaceRelease(_wgpuSurface);
   wgpuQueueRelease(_wgpuQueue);
   wgpuDeviceRelease(_wgpuDevice);
}

void Renderer::Render(const Camera &camera) {

   std::chrono::high_resolution_clock::time_point start, end, startSort, endSort, startRender, endRender;

   start = std::chrono::high_resolution_clock::now();

   startSort = std::chrono::high_resolution_clock::now();
   UpdateUniforms(camera);

   WGPUCommandEncoder encoder = *CreateCommandEncoder();

   int workGroups = (_splatsData.size() + _workGroupSize - 1) / _workGroupSize;

   // Transform compute pass.
   WGPUComputePassEncoder computePassEncoder = *BeginComputePass(encoder);
   wgpuComputePassEncoderSetPipeline(computePassEncoder, _wgpuTransformComputePipeline);
   wgpuComputePassEncoderSetBindGroup(computePassEncoder, 0, _sceneBindGroup, 0, nullptr);
   wgpuComputePassEncoderSetBindGroup(computePassEncoder, 1, _stateBindGroup, 0, nullptr);
   wgpuComputePassEncoderDispatchWorkgroups(computePassEncoder, workGroups, 1, 1);
   wgpuComputePassEncoderEnd(computePassEncoder);
   wgpuComputePassEncoderRelease(computePassEncoder);

   // Sort compute pass.
   for (uint i = 0; i < _sortSplatsParamsData.size(); ++i)
   {
      uint32_t offset = i * sizeof(uvec2);

      wgpuCommandEncoderCopyBufferToBuffer(encoder, _sortSplatsParamsDataBuffer, offset, _sortSplatsParamsUniform, 0, sizeof(uvec2));
      computePassEncoder = *BeginComputePass(encoder);
      wgpuComputePassEncoderSetPipeline(computePassEncoder, _wgpuSortComputePipeline);
      wgpuComputePassEncoderSetBindGroup(computePassEncoder, 0, _sceneBindGroup, 0, nullptr);
      wgpuComputePassEncoderSetBindGroup(computePassEncoder, 1, _stateBindGroup, 0, nullptr);
      wgpuComputePassEncoderDispatchWorkgroups(computePassEncoder, workGroups, 1, 1);
      wgpuComputePassEncoderEnd(computePassEncoder);
      wgpuComputePassEncoderRelease(computePassEncoder);
   }
   endSort = std::chrono::high_resolution_clock::now();

   startRender = std::chrono::high_resolution_clock::now();
   // Render pass.
   WGPUSurfaceTexture surfaceTexture = *GetNextSurfaceTexture();
   WGPUTextureView textureView = *CreateTextureView(surfaceTexture.texture);
   WGPURenderPassEncoder renderPassEncoder = *BeginRenderPass(encoder, textureView);

   wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 0, _sceneBindGroup, 0, nullptr);
   wgpuRenderPassEncoderSetBindGroup(renderPassEncoder, 1, _stateBindGroup, 0, nullptr);
   wgpuRenderPassEncoderSetPipeline(renderPassEncoder, _wgpuRenderPipeline);
   wgpuRenderPassEncoderDraw(renderPassEncoder, 4, static_cast<int>(_splatsData.size()), 0, 0);

   // Render ImGui UI
   ImGuiBeginFrame();
   RenderImGuiUI();
   ImGuiEndFrame(renderPassEncoder);

   wgpuRenderPassEncoderEnd(renderPassEncoder);
   wgpuRenderPassEncoderRelease(renderPassEncoder);
   // Submit command buffer and release resources.
   WGPUCommandBuffer commandBuffer = *FinishAndReleaseCommandEncoder(encoder);
   wgpuQueueSubmit(_wgpuQueue, 1, &commandBuffer); // Submit command buffer.
   wgpuCommandBufferRelease(commandBuffer);
   wgpuTextureViewRelease(textureView);
   wgpuSurfacePresent(_wgpuSurface);
   wgpuDevicePoll(_wgpuDevice, false, nullptr);
   wgpuTextureRelease(surfaceTexture.texture); // Release the surface texture as it was causing a memory leak.
   endRender = std::chrono::high_resolution_clock::now();
   end = std::chrono::high_resolution_clock::now();

   _performanceData.sortTime = std::chrono::duration<float, std::milli>(endSort - startSort).count();
   _performanceData.renderTime = std::chrono::duration<float, std::milli>(endRender - startRender).count();
   _performanceData.frameTime = std::chrono::duration<float, std::milli>(end - start).count();
}

// Calculate avg position of all splats.
vec4 Renderer::GetModelPosition() const {
   vec4 avgPosition(0.0f);
   for (const auto &splat: _rawSplatsData) {
      avgPosition += _modelMatrix * splat.position;
   }
   avgPosition /= static_cast<float>(_rawSplatsData.size());
   return avgPosition;
}

bool Renderer::InitializeImGui(GLFWwindow* window)
{
   ImGui::CreateContext();
   ImGui_ImplGlfw_InitForOther(window, true);
   ImGui::StyleColorsDark();

   WGPUMultisampleState multiSampleState = {};
   multiSampleState.count = 1;
   multiSampleState.mask = UINT32_MAX;

   ImGui_ImplWGPU_InitInfo initInfo = {};
   initInfo.Device = _wgpuDevice;
   initInfo.NumFramesInFlight = 3;
   initInfo.RenderTargetFormat = _surfaceFormat;
   initInfo.DepthStencilFormat = WGPUTextureFormat_Undefined;
   initInfo.PipelineMultisampleState = multiSampleState;

   ImGui_ImplWGPU_Init(&initInfo);

   SetImGuiStyle();

   return true;
}

void Renderer::ReleaseImGui()
{
   ImGui_ImplWGPU_Shutdown();
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();
}

void Renderer::ImGuiBeginFrame()
{
   ImGui_ImplWGPU_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();
}

void Renderer::ImGuiEndFrame(WGPURenderPassEncoder encoder)
{
   ImGui::Render();
   ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), encoder);
}

void Renderer::RenderImGuiUI()
{
   ImVec2 screenSize = ImGui::GetIO().DisplaySize;
   ImVec2 panelSize(250, 150);

   ImGui::SetNextWindowPos(ImVec2(screenSize.x - panelSize.x - 10, 10), ImGuiCond_Always);
   ImGui::SetNextWindowSize(panelSize, ImGuiCond_Always);

   ImGui::Begin("Performance Stats", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

   ImGui::Text("Point count: %d", static_cast<int>(_performanceData.pointCount));

   ImGui::Separator();

   ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
   ImGui::Text("Frame time: %.2f ms", _performanceData.frameTime);
   ImGui::Text("Sort time: %.2f ms", _performanceData.sortTime);
   ImGui::Text("Render time: %.2f ms", _performanceData.renderTime);


   ImGui::End();

   ImGui::SetNextWindowPos(ImVec2(screenSize.x - panelSize.x - 10, 170), ImGuiCond_Always);
   ImGui::SetNextWindowSize(panelSize, ImGuiCond_Always);

   ImGui::Begin("Renderer Settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

   // Get files in directory ../../../assets/.
   std::vector<char*> files;
   FileReader::GetFilesInDirectory("../../../assets/splats", files);

   if (ImGui::Combo("Splat file name:", &SelectedFileIndex, files.data(), static_cast<int>(files.size())))
   {
      ChangeSplatsFlag = true;
      SelectedFile = files[SelectedFileIndex];
   }

   ImGui::SliderFloat("Splat Size", &_splatScale, 0.02f, 1.2f, "%.2f");
   ImGui::Checkbox("Free camera", &FreeCamera);

   ImGui::End();
}

bool Renderer::CreateWGPUInstance()
{
   WGPUInstanceDescriptor instanceDesc = {};
   instanceDesc.nextInChain = nullptr;
   _wgpuInstance = wgpuCreateInstance(&instanceDesc);
   if (!_wgpuInstance) {
      std::cerr << "Could not initialize WebGPU!" << std::endl;
      return false;
   }
   return true;
}

bool Renderer::CreateWGPUSurface(GLFWwindow* window)
{
   _wgpuSurface = glfwCreateWindowWGPUSurface(_wgpuInstance, window);
   if (!_wgpuSurface) {
      std::cerr << "Could not create WGPU surface!" << std::endl;
      return false;
   }
   return true;
}

bool Renderer::GetAdapterAndDevice() {
   // Request adapter.
   struct AdapterRequestData {
      WGPUAdapter adapter;
      bool requestEnded;
   };
   auto adapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const *message, void *userData) {
      auto &data = *static_cast<AdapterRequestData *>(userData);
      if (status == WGPURequestAdapterStatus_Success) {
         std::cout << "Adapter requested successfully: " << adapter << std::endl;
         data.adapter = adapter;
      } else {
         std::cerr << "Adapter request failed: " << message << std::endl;
      }

      data.requestEnded = true;
   };

   WGPURequestAdapterOptions adapterOpts = {};
   adapterOpts.nextInChain = nullptr;
   adapterOpts.compatibleSurface = _wgpuSurface;

   AdapterRequestData adapterRequestData = {};
   adapterRequestData.adapter = nullptr;
   adapterRequestData.requestEnded = false;
   wgpuInstanceRequestAdapter(_wgpuInstance, &adapterOpts, adapterRequestEnded, &adapterRequestData);
   while (!adapterRequestData.requestEnded) {
      wgpuInstanceProcessEvents(_wgpuInstance);
   }
   _wgpuAdapter = adapterRequestData.adapter;

   // Request device.
   struct DeviceRequestData {
      WGPUDevice device;
      bool requestEnded;
   };
   auto deviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, char const *message, void *userData) {
      auto &data = *static_cast<DeviceRequestData *>(userData);
      if (status == WGPURequestDeviceStatus_Success) {
         std::cout << "Device requested successfully: " << device << std::endl;
         data.device = device;
      } else {
         std::cerr << "Device request failed: " << message << std::endl;
      }

      data.requestEnded = true;
   };

   WGPUFeatureName requiredFeatures[] = { (WGPUFeatureName)WGPUNativeFeature_VertexWritableStorage };
   for (auto feature : requiredFeatures) {
      if (!wgpuAdapterHasFeature(_wgpuAdapter, feature)) {
         std::cerr << "Adapter does not support required feature!" << std::endl;
         return false;
      }
   }

   auto deviceLostCallback = [](WGPUDeviceLostReason reason, char const* message, void*) {
      std::cout << "Device lost: reason " << reason;
      if (message) std::cout << " (" << message << ")";
      std::cout << std::endl;
      __debugbreak();
      };

   DeviceRequestData deviceRequestData = {};
   deviceRequestData.device = nullptr;
   deviceRequestData.requestEnded = false;
   WGPUDeviceDescriptor deviceDesc = {};
   deviceDesc.nextInChain = nullptr;
   deviceDesc.label = "WGPU Device";
   deviceDesc.requiredFeatureCount = 1;
   deviceDesc.requiredFeatures = requiredFeatures;
   deviceDesc.defaultQueue.nextInChain = nullptr;
   deviceDesc.defaultQueue.label = "Default queue";
   deviceDesc.deviceLostCallback = deviceLostCallback;

   WGPUSupportedLimits supportedLimits = {};
   supportedLimits.nextInChain = nullptr;
   wgpuAdapterGetLimits(_wgpuAdapter, &supportedLimits);

   WGPURequiredLimits requiredLimits = {};
   setDefault(requiredLimits.limits);
   requiredLimits.limits.maxVertexAttributes = 4;
   requiredLimits.limits.maxVertexBuffers = 1;
   requiredLimits.limits.maxBufferSize = 512 * 1024 * 1024; // 512MB
   requiredLimits.limits.maxVertexBufferArrayStride = sizeof(float) * 6 + 2 * sizeof(uint32_t);
   requiredLimits.limits.maxInterStageShaderComponents = 8;
   requiredLimits.limits.maxBindGroups = 2;
   requiredLimits.limits.maxUniformBuffersPerShaderStage = 2;
   requiredLimits.limits.maxUniformBufferBindingSize = sizeof(ShaderUniforms);
   requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
   requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;

   deviceDesc.requiredLimits = &requiredLimits;

   wgpuAdapterRequestDevice(adapterRequestData.adapter, &deviceDesc, deviceRequestEnded, &deviceRequestData);
   while (!deviceRequestData.requestEnded) {
      wgpuInstanceProcessEvents(_wgpuInstance);
   }
   _wgpuDevice = deviceRequestData.device;
   auto onDeviceError = [](WGPUErrorType type, char const* message, void*) {
      std::cout << "Uncaptured device error: type " << type;
      if (message) std::cout << " (" << message << ")";
      std::cout << std::endl;
      __debugbreak();
      };

   wgpuDeviceSetUncapturedErrorCallback(_wgpuDevice, onDeviceError, nullptr);
   return true;
}

void Renderer::RequestQueue()
{
   auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void*) {
      std::cout << "Queued work finished with status: " << status << std::endl;
      };

   _wgpuQueue = wgpuDeviceGetQueue(_wgpuDevice);
   wgpuQueueOnSubmittedWorkDone(_wgpuQueue, onQueueWorkDone, nullptr);
}

void Renderer::ConfigureSurface()
{
   WGPUSurfaceConfiguration surfaceConfig = {};
   surfaceConfig.nextInChain = nullptr;
   surfaceConfig.width = _viewPortSize.x;
   surfaceConfig.height = _viewPortSize.y;
   surfaceConfig.format = _surfaceFormat;
   surfaceConfig.viewFormatCount = 0;
   surfaceConfig.viewFormats = nullptr;
   surfaceConfig.usage = WGPUTextureUsage_RenderAttachment; // Swap chain textures as targets.
   surfaceConfig.device = _wgpuDevice;
   surfaceConfig.presentMode = WGPUPresentMode_Immediate;
   surfaceConfig.alphaMode = WGPUCompositeAlphaMode_Auto;
   wgpuSurfaceConfigure(_wgpuSurface, &surfaceConfig);
}

void Renderer::InitializeBuffers()
{
   _splatsData = _rawSplatsData;

   // Splat buffer.
   WGPUBufferDescriptor splatsBufferDesc = {};
   splatsBufferDesc.nextInChain = nullptr;
   splatsBufferDesc.label = "Splat Buffer";
   splatsBufferDesc.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst;
   splatsBufferDesc.size = sizeof(Splat) * _splatsData.size();
   _splatsBuffer = wgpuDeviceCreateBuffer(_wgpuDevice, &splatsBufferDesc);
   wgpuQueueWriteBuffer(_wgpuQueue, _splatsBuffer, 0, _splatsData.data(), splatsBufferDesc.size);

   // Sorted splat buffer.
   WGPUBufferDescriptor sortedSplatsBufferDesc = {};
   sortedSplatsBufferDesc.nextInChain = nullptr;
   sortedSplatsBufferDesc.label = "Sorted Splat Buffer";
   sortedSplatsBufferDesc.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst;
   sortedSplatsBufferDesc.size = sizeof(SortSplatsData) * _splatsData.size();
   _sortedSplatsBuffer = wgpuDeviceCreateBuffer(_wgpuDevice, &sortedSplatsBufferDesc);
   wgpuQueueWriteBuffer(_wgpuQueue, _sortedSplatsBuffer, 0, _splatsData.data(), sortedSplatsBufferDesc.size);

   // Sort splats params data.
   WGPUBufferDescriptor sortSplatsParamsBufferDesc = {};
   sortSplatsParamsBufferDesc.label = "Sort Splats params array";
   sortSplatsParamsBufferDesc.size = _sortSplatsParamsData.size() * sizeof(uvec2);
   sortSplatsParamsBufferDesc.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst;
   sortSplatsParamsBufferDesc.mappedAtCreation = false;
   _sortSplatsParamsDataBuffer = wgpuDeviceCreateBuffer(_wgpuDevice, &sortSplatsParamsBufferDesc);
   wgpuQueueWriteBuffer(_wgpuQueue, _sortSplatsParamsDataBuffer, 0, _sortSplatsParamsData.data(), _sortSplatsParamsData.size() * sizeof(uvec2));

   // Sort splats params uniform.
   WGPUBufferDescriptor sortSplatsParamsUniformBufferDesc = {};
   sortSplatsParamsUniformBufferDesc.label = "Sort Splats params uniform";
   sortSplatsParamsUniformBufferDesc.size = sizeof(uvec2);
   sortSplatsParamsUniformBufferDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
   sortSplatsParamsUniformBufferDesc.mappedAtCreation = false;
   _sortSplatsParamsUniform = wgpuDeviceCreateBuffer(_wgpuDevice, &sortSplatsParamsUniformBufferDesc);

   // Uniform buffer.
   WGPUBufferDescriptor uniformBufferDesc = {};
   uniformBufferDesc.label = "WGPU Uniform Buffer";
   uniformBufferDesc.size = sizeof(ShaderUniforms);
   uniformBufferDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
   uniformBufferDesc.mappedAtCreation = false;
   _uniformBuffer = wgpuDeviceCreateBuffer(_wgpuDevice, &uniformBufferDesc);
   ShaderUniforms uniforms = {};
   uniforms.model = _modelMatrix;
   uniforms.view = mat4x4(1.0f);
   uniforms.projection = mat4x4(1.0f);
   uniforms.splatScale = _splatScale;
   wgpuQueueWriteBuffer(_wgpuQueue, _uniformBuffer, 0, &uniforms, uniformBufferDesc.size);

   // Scene bind group layout entries.
   WGPUBindGroupLayoutEntry sceneBGLEntries = {};
   setDefault(sceneBGLEntries);
   sceneBGLEntries.binding = 0;
   sceneBGLEntries.visibility = WGPUShaderStage_Compute | WGPUShaderStage_Vertex;
   sceneBGLEntries.buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
   sceneBGLEntries.buffer.minBindingSize = sizeof(Splat) * _splatsData.size();
   sceneBGLEntries.buffer.hasDynamicOffset = false;

   // Scene bind group layout.
   WGPUBindGroupLayoutDescriptor sceneBGLDesc = {};
   sceneBGLDesc.nextInChain = nullptr;
   sceneBGLDesc.label = "Scene Bind Group Layout";
   sceneBGLDesc.entryCount = 1;
   sceneBGLDesc.entries = &sceneBGLEntries;
   _sceneBindGroupLayout = wgpuDeviceCreateBindGroupLayout(_wgpuDevice, &sceneBGLDesc);

   // Scene binding.
   WGPUBindGroupEntry sceneBGEntries = {};
   sceneBGLEntries.nextInChain = nullptr;
   sceneBGEntries.binding = 0;
   sceneBGEntries.buffer = _splatsBuffer;
   sceneBGEntries.offset = 0;
   sceneBGEntries.size = sizeof(Splat) * _splatsData.size();

   // Scene bind group.
   WGPUBindGroupDescriptor sceneBGDesc = {};
   sceneBGDesc.nextInChain = nullptr;
   sceneBGDesc.label = "Scene Bind Group";
   sceneBGDesc.layout = _sceneBindGroupLayout;
   sceneBGDesc.entryCount = 1;
   sceneBGDesc.entries = &sceneBGEntries;
   _sceneBindGroup = wgpuDeviceCreateBindGroup(_wgpuDevice, &sceneBGDesc);

   // State bind group layout entries.
   WGPUBindGroupLayoutEntry stateBGLEntries[4] = {};
   setDefault(stateBGLEntries[0]);
   stateBGLEntries[0].binding = 0;
   stateBGLEntries[0].visibility = WGPUShaderStage_Compute | WGPUShaderStage_Vertex;
   stateBGLEntries[0].buffer.type = WGPUBufferBindingType_Storage;
   stateBGLEntries[0].buffer.minBindingSize = sizeof(SortSplatsData) * _splatsData.size();
   stateBGLEntries[0].buffer.hasDynamicOffset = false;
   setDefault(stateBGLEntries[1]);
   stateBGLEntries[1].binding = 1;
   stateBGLEntries[1].visibility = WGPUShaderStage_Compute | WGPUShaderStage_Vertex;
   stateBGLEntries[1].buffer.type = WGPUBufferBindingType_Uniform;
   stateBGLEntries[1].buffer.minBindingSize = sizeof(ShaderUniforms);
   setDefault(stateBGLEntries[2]);
   stateBGLEntries[2].binding = 2;
   stateBGLEntries[2].visibility = WGPUShaderStage_Compute;
   stateBGLEntries[2].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
   stateBGLEntries[2].buffer.minBindingSize = _sortSplatsParamsData.size() * sizeof(uvec2);
   setDefault(stateBGLEntries[3]);
   stateBGLEntries[3].binding = 3;
   stateBGLEntries[3].visibility = WGPUShaderStage_Compute;
   stateBGLEntries[3].buffer.type = WGPUBufferBindingType_Uniform;
   stateBGLEntries[3].buffer.minBindingSize = sizeof(uvec2);

   // State bind group layout.
   WGPUBindGroupLayoutDescriptor stateBGLDesc = {};
   stateBGLDesc.nextInChain = nullptr;
   stateBGLDesc.label = "State Bind Group Layout";
   stateBGLDesc.entryCount = 4;
   stateBGLDesc.entries = stateBGLEntries;
   _stateBindGroupLayout = wgpuDeviceCreateBindGroupLayout(_wgpuDevice, &stateBGLDesc);

   // State binding.
   WGPUBindGroupEntry stateBGEntries[4] = {};
   stateBGEntries[0].nextInChain = nullptr;
   stateBGEntries[0].binding = 0;
   stateBGEntries[0].buffer = _sortedSplatsBuffer;
   stateBGEntries[0].offset = 0;
   stateBGEntries[0].size = sizeof(SortSplatsData) * _splatsData.size();
   stateBGEntries[1].nextInChain = nullptr;
   stateBGEntries[1].binding = 1;
   stateBGEntries[1].buffer = _uniformBuffer;
   stateBGEntries[1].offset = 0;
   stateBGEntries[1].size = sizeof(ShaderUniforms);
   stateBGEntries[2].nextInChain = nullptr;
   stateBGEntries[2].binding = 2;
   stateBGEntries[2].buffer = _sortSplatsParamsDataBuffer;
   stateBGEntries[2].offset = 0;
   stateBGEntries[2].size = _sortSplatsParamsData.size() * sizeof(uvec2);
   stateBGEntries[3].nextInChain = nullptr;
   stateBGEntries[3].binding = 3;
   stateBGEntries[3].buffer = _sortSplatsParamsUniform;
   stateBGEntries[3].offset = 0;
   stateBGEntries[3].size = sizeof(uvec2);

   // State bind group.
   WGPUBindGroupDescriptor stateBGDesc = {};
   stateBGDesc.nextInChain = nullptr;
   stateBGDesc.label = "State Bind Group";
   stateBGDesc.layout = _stateBindGroupLayout;
   stateBGDesc.entryCount = 4;
   stateBGDesc.entries = stateBGEntries;
   _stateBindGroup = wgpuDeviceCreateBindGroup(_wgpuDevice, &stateBGDesc);
}

void Renderer::InitializeComputePipelines(WGPUShaderModule shaderModule)
{
   // Compute pipeline layout for transform.
   WGPUComputePipelineDescriptor computePipelineDesc = {};
   computePipelineDesc.nextInChain = nullptr;
   computePipelineDesc.label = "Transform Compute Pipeline";
   computePipelineDesc.layout = _wgpuPipelineLayout;
   computePipelineDesc.compute.module = shaderModule;
   computePipelineDesc.compute.entryPoint = "cs_calculate_sort_splats";
   _wgpuTransformComputePipeline = wgpuDeviceCreateComputePipeline(_wgpuDevice, &computePipelineDesc);

   // Compute pipeline layout for sorting splats.
   computePipelineDesc.nextInChain = nullptr;
   computePipelineDesc.label = "Sort Compute Pipeline";
   computePipelineDesc.layout = _wgpuPipelineLayout;
   computePipelineDesc.compute.module = shaderModule;
   computePipelineDesc.compute.entryPoint = "cs_sort_splats";
   _wgpuSortComputePipeline = wgpuDeviceCreateComputePipeline(_wgpuDevice, &computePipelineDesc);
}

void Renderer::InitializeRenderPipeline(WGPUShaderModule shaderModule)
{
   // Blend state
   WGPUBlendState blendState = {};
   blendState.color.operation = WGPUBlendOperation_Add;
   blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
   blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
   blendState.alpha.operation = WGPUBlendOperation_Add;
   blendState.alpha.srcFactor = WGPUBlendFactor_One;
   blendState.alpha.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;

   // Color target state
   WGPUColorTargetState colorTargetState = {};
   colorTargetState.format = _surfaceFormat;
   colorTargetState.blend = &blendState;
   colorTargetState.writeMask = WGPUColorWriteMask_All;

   // Fragment state
   WGPUFragmentState fragmentState = {};
   fragmentState.module = shaderModule;
   fragmentState.entryPoint = "fs_main";
   fragmentState.constantCount = 0;
   fragmentState.constants = nullptr;
   fragmentState.targetCount = 1; // We have only one target because our render pass has only one output color attachment.
   fragmentState.targets = &colorTargetState;

   // Create the render pipeline.
   WGPURenderPipelineDescriptor pipelineDesc = {};
   pipelineDesc.nextInChain = nullptr;
   pipelineDesc.label = "My Render Pipeline";
   pipelineDesc.vertex.module = shaderModule;
   pipelineDesc.vertex.entryPoint = "vs_main";
   pipelineDesc.vertex.bufferCount = 0;
   pipelineDesc.vertex.constantCount = 0;
   pipelineDesc.vertex.constants = nullptr;
   pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleStrip;
   pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
   pipelineDesc.primitive.frontFace = WGPUFrontFace_CW;
   pipelineDesc.primitive.cullMode = WGPUCullMode_Front;
   pipelineDesc.fragment = &fragmentState;
   pipelineDesc.depthStencil = nullptr;
   pipelineDesc.multisample.count = 1; // Samples per pixel
   pipelineDesc.multisample.mask = ~0u; // Default value for the mask, meaning "all bits on"
   pipelineDesc.multisample.alphaToCoverageEnabled = false; // Default value as well (irrelevant for count = 1 anyways)
   pipelineDesc.layout = _wgpuPipelineLayout;
   _wgpuRenderPipeline = wgpuDeviceCreateRenderPipeline(_wgpuDevice, &pipelineDesc);

   wgpuShaderModuleRelease(shaderModule);
}

void Renderer::UpdateUniforms(const Camera& camera) const
{
   ShaderUniforms uniforms;
   uniforms.model = _modelMatrix;
   uniforms.view = camera.GetViewMatrix();
   uniforms.projection = camera.GetProjectionMatrix();
   uniforms.splatScale = _splatScale;
   wgpuQueueWriteBuffer(_wgpuQueue, _uniformBuffer, 0, &uniforms, sizeof(ShaderUniforms));
}

WGPUCommandEncoder* Renderer::CreateCommandEncoder() const
{
   WGPUCommandEncoderDescriptor encoderDesc = {};
   encoderDesc.nextInChain = nullptr;
   encoderDesc.label = "Command Encoder";
   WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(_wgpuDevice, &encoderDesc);
   if (!encoder) {
      std::cerr << "Failed to create command encoder!" << std::endl;
      return nullptr;
   }

   return &encoder;
}

WGPUComputePassEncoder* Renderer::BeginComputePass(WGPUCommandEncoder encoder) const
{
   WGPUComputePassDescriptor computePassDesc = {};
   computePassDesc.nextInChain = nullptr;
   computePassDesc.label = "Compute Pass";
   WGPUComputePassEncoder computePassEncoder = wgpuCommandEncoderBeginComputePass(encoder, &computePassDesc);
   if (!computePassEncoder) {
      std::cerr << "Failed to begin compute pass!" << std::endl;
      wgpuCommandEncoderRelease(encoder);
      return nullptr;
   }
   return &computePassEncoder;
}

WGPUSurfaceTexture* Renderer::GetNextSurfaceTexture() const
{
   WGPUSurfaceTexture surfaceTexture;
   wgpuSurfaceGetCurrentTexture(_wgpuSurface, &surfaceTexture);
   if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
      __debugbreak();
      return nullptr;
   }

   return &surfaceTexture;
}

WGPUTextureView* Renderer::CreateTextureView(WGPUTexture texture) const
{
   WGPUTextureViewDescriptor viewDesc;
   viewDesc.nextInChain = nullptr;
   viewDesc.label = "Surface texture view";
   viewDesc.format = _surfaceFormat;
   viewDesc.dimension = WGPUTextureViewDimension_2D;
   viewDesc.baseMipLevel = 0;
   viewDesc.mipLevelCount = 1;
   viewDesc.baseArrayLayer = 0;
   viewDesc.arrayLayerCount = 1;
   viewDesc.aspect = WGPUTextureAspect_All;
   WGPUTextureView textureView = wgpuTextureCreateView(texture, &viewDesc);
   if (!textureView) {
      std::cerr << "Failed to get next surface texture view!" << std::endl;
      return nullptr;
   }

   return &textureView;
}

WGPURenderPassEncoder* Renderer::BeginRenderPass(WGPUCommandEncoder encoder, WGPUTextureView textureView) const
{
   WGPURenderPassDescriptor renderPassDesc = {};
   renderPassDesc.nextInChain = nullptr;
   renderPassDesc.label = "Render Pass";

   WGPURenderPassColorAttachment renderPassColorAttachment = {};
   renderPassColorAttachment.view = textureView;
   renderPassColorAttachment.resolveTarget = nullptr;
   renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
   renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
   renderPassColorAttachment.clearValue = WGPUColor{ 1.0, 1.0, 1.0, 1.0 };

   renderPassDesc.colorAttachmentCount = 1;
   renderPassDesc.colorAttachments = &renderPassColorAttachment;
   renderPassDesc.depthStencilAttachment = nullptr;
   renderPassDesc.timestampWrites = nullptr;

   WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
   if (!renderPassEncoder) {
      std::cerr << "Failed to begin render pass!" << std::endl;
      wgpuCommandEncoderRelease(encoder);
      wgpuTextureViewRelease(textureView);
      return nullptr;
   }

   return &renderPassEncoder;
}

WGPUCommandBuffer* Renderer::FinishAndReleaseCommandEncoder(WGPUCommandEncoder encoder) const
{
   WGPUCommandBufferDescriptor cmdBufferDesc = {};
   cmdBufferDesc.nextInChain = nullptr;
   cmdBufferDesc.label = "Command Buffer";
   WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);
   wgpuCommandEncoderRelease(encoder);
   if (!commandBuffer) {
      std::cerr << "Failed to finish command buffer!" << std::endl;
      wgpuCommandBufferRelease(commandBuffer);
      return nullptr;
   }

   return &commandBuffer;
}