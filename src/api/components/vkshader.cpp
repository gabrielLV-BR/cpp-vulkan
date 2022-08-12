#include "vkshader.hpp"
#include "utils/file.hpp"
#include "utils/debug.hpp"

ShaderModule::ShaderModule() {}

ShaderModule::ShaderModule(const char* path, VkDevice device) {
  // auto source = FileUtils::ReadBinary(path);
  std::vector<char> source = {};

  ASSERT(source.size() > 0, "Invalid shader source");
  // if(source.size() == 0) 
  //   throw std::runtime_error("Error creating Shader: Invalid source");

  VkShaderModuleCreateInfo shaderInfo{};

  shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shaderInfo.codeSize = source.size();
  // reinterpret_cast<>() -> Basically just a pointer cast, does not 
  // change the data.
  shaderInfo.pCode = reinterpret_cast<const uint32_t*>(source.data());

  VK_ASSERT(
    vkCreateShaderModule(
      device, 
      &shaderInfo, 
      nullptr, 
      &module
    )
  );
}

void ShaderModule::Destroy(VkDevice device) {
  vkDestroyShaderModule(device, module, nullptr);
}

VkShaderModule& ShaderModule::GetModule() {
  return module;
}