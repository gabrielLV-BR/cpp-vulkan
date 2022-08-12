#pragma once

#include "vulkan/vulkan.h"

class ShaderModule {
private:
  VkShaderModule handle;
public:
  ShaderModule();
  ShaderModule(const char*, VkDevice);
  void Destroy(VkDevice);

  VkShaderModule& GetModule();
};