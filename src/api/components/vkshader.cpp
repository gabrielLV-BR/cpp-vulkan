#include "vkshader.hpp"
#include "utils/file.hpp"

ShaderModule::ShaderModule() {}

ShaderModule::ShaderModule(const char* path, VkDevice device) {
  auto source = FileUtils::ReadBinary(path);

  if(source.size() == 0) throw std::runtime_error("Invalid source");

  VkShaderModuleCreateInfo shaderInfo{};

  shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shaderInfo.codeSize = source.size();
  // reinterpret_cast<>() -> Basically just a pointer cast, does not 
  // change the data.
  shaderInfo.pCode = reinterpret_cast<const uint32_t*>(source.data());

  if(
    vkCreateShaderModule(
      device, 
      &shaderInfo, 
      nullptr, 
      &module
    ) != VK_SUCCESS) {
    throw std::runtime_error("Failure on creating shader module");
  }
}

void ShaderModule::Destroy(VkDevice device) {
  vkDestroyShaderModule(device, module, nullptr);
}

VkShaderModule& ShaderModule::GetModule() {
  return module;
}