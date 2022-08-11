#pragma once

#include <vulkan/vulkan.h>
#include "vkshader.hpp"
#include "../vkutils.hpp"

class Pipeline {
private:
  ShaderModule vertexShaderModule;
  ShaderModule fragmentShaderModule;
  VkPipeline pipeline;
  VkPipelineLayout layout;

public:
  Pipeline();
  Pipeline(VkDevice);
  void Destroy(VkDevice);

  void CreatePipeline(VkDevice, VkViewport, VkRect2D);

private:
  std::vector<VkPipelineShaderStageCreateInfo> CreateShaderStages();  
};