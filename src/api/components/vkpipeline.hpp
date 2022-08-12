#pragma once

#include <vulkan/vulkan.h>

#include "vkshader.hpp"
#include "api/vkutils.hpp"

class Pipeline {
private:
  ShaderModule vertexShaderModule;
  ShaderModule fragmentShaderModule;
  

public:
  VkPipeline pipeline;
  VkRenderPass renderPass;
  VkPipelineLayout layout;
  
  Pipeline();
  Pipeline(VkDevice);
  void Destroy(VkDevice);

  void CreatePipeline(VkDevice, VkViewport, VkRect2D);
  void CreateRenderPass(VkDevice, VkFormat);

private:
  std::vector<VkPipelineShaderStageCreateInfo> CreateShaderStages();  
};