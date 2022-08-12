#pragma once

#include "vulkan/vulkan.h"

class ShaderModule {
private:
  VkShaderModule module;
public:
  ShaderModule();
  ShaderModule(const char*, VkDevice);
  void Destroy(VkDevice);

  VkShaderModule& GetModule();
};