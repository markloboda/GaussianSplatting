#include <GaussianSplatting.h>
#include <Utils/FileReader.h>

uint32_t ABGRtoRGBA(uint32_t abgr) {
   uint8_t a = (abgr >> 24) & 0xFF;
   uint8_t b = (abgr >> 16) & 0xFF;
   uint8_t g = (abgr >> 8) & 0xFF;
   uint8_t r = abgr & 0xFF;

   return (r << 24) | (g << 16) | (b << 8) | a;
}

bool FileReader::LoadSplatData(const std::filesystem::path& path, std::vector<Splat>& splats) {
   // Open the file in binary mode
   std::ifstream file(path, std::ios::binary);
   if (!file.is_open()) {
      std::cerr << "Failed to open geometry file: " << path << std::endl;
      return false;
   }

   // Get the file size
   file.seekg(0, std::ios::end);
   size_t fileSize = file.tellg();
   file.seekg(0, std::ios::beg);

   // Calculate the number of splats
   size_t numSplats = fileSize / 32;
   splats.resize(numSplats);

   // Read the splats from the file
   for (size_t i = 0; i < numSplats; ++i) {
      // Position (3 * float32)
      file.read(reinterpret_cast<char*>(&splats[i].position.x), sizeof(splats[i].position.x));
      file.read(reinterpret_cast<char*>(&splats[i].position.y), sizeof(splats[i].position.y));
      file.read(reinterpret_cast<char*>(&splats[i].position.z), sizeof(splats[i].position.z));
      splats[i].position.w = 1.0f;

      // Scale (3 * float32)
      file.read(reinterpret_cast<char*>(&splats[i].scale.x), sizeof(splats[i].scale.x));
      file.read(reinterpret_cast<char*>(&splats[i].scale.y), sizeof(splats[i].scale.y));
      file.read(reinterpret_cast<char*>(&splats[i].scale.z), sizeof(splats[i].scale.z));
      splats[i].position.w = 1.0f;

      // Color (4 * uint8 -> uint32)
      file.read(reinterpret_cast<char*>(&splats[i].color), sizeof(splats[i].color));

      // ABGR -> RGBA
      splats[i].color = ABGRtoRGBA(splats[i].color);

      // Rotation (4 * uint8 -> uint32)
      file.read(reinterpret_cast<char*>(&splats[i].rotation), sizeof(splats[i].rotation));
   }
    
   file.close();
   return true;
}

wgpu::ShaderModule FileReader::LoadShaderModule(const std::filesystem::path& path, wgpu::Device device) {
   // Open the file in binary mode and position at the end to get its size
   std::ifstream file(path, std::ios::ate | std::ios::binary);
   if (!file.is_open()) {
      std::cerr << "Failed to open shader file: " << path << std::endl;
      return nullptr;
   }

   // Get the file size
   size_t fileSize = file.tellg();
   // Create a buffer with an extra byte for a null terminator
   std::vector<char> buffer(fileSize + 1);

   // Go back to the beginning and read the entire file
   file.seekg(0);
   file.read(buffer.data(), fileSize);
   file.close();

   // Null-terminate the shader source code
   buffer[fileSize] = '\0';

   const char* shaderSource = buffer.data();

   // Load shader module.
   WGPUShaderModuleDescriptor shaderDesc = {};
#ifdef WEBGPU_BACKEND_WGPU
   shaderDesc.hintCount = 0;
   shaderDesc.hints = nullptr;
#endif
   WGPUShaderModuleWGSLDescriptor shaderCodeDesc{};
   shaderCodeDesc.chain.next = nullptr; // Set the chained struct's header
   shaderCodeDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
   shaderDesc.nextInChain = &shaderCodeDesc.chain; // Connect the chain
   shaderCodeDesc.code = shaderSource;
   WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(device, &shaderDesc);

   return shaderModule;
}

void FileReader::GetFilesInDirectory(const std::filesystem::path& path, std::vector<char*>& files)
{
   for (const auto& entry : std::filesystem::directory_iterator(path))
   {
      std::string filename = entry.path().filename().string();
      files.push_back(strdup(filename.c_str()));
   }
}