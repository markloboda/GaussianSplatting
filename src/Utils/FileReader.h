#pragma once

#include <webgpu/webgpu.hpp>

struct Splat {
   alignas(16) f32vec4 position;
   alignas(16) f32vec4 scale;
   alignas(4) u32 color;
   alignas(4) u32 rotation;
};

class FileReader {
public:
   static bool LoadSplatData(const std::filesystem::path& path, std::vector<Splat>& splats);

   static wgpu::ShaderModule LoadShaderModule(const std::filesystem::path& path, wgpu::Device device);

   static void GetFilesInDirectory(const std::filesystem::path& path, std::vector<char*>& files);
};
