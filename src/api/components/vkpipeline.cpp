#include "vkpipeline.hpp"
#include <vector>

Pipeline::Pipeline() {}

Pipeline::Pipeline(VkDevice device)
  : vertexShaderModule(RESOURCES"shaders/basic.frag.spv", device),
    fragmentShaderModule(RESOURCES"shaders/basic.vert.spv", device) 
  {
}

std::vector<VkPipelineShaderStageCreateInfo> Pipeline::CreateShaderStages() {
  VkPipelineShaderStageCreateInfo vertexInfo{};
  vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertexInfo.pName = "main";
  vertexInfo.module = vertexShaderModule.GetModule();
  vertexInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

  VkPipelineShaderStageCreateInfo fragInfo{};
  fragInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragInfo.pName = "main";
  fragInfo.module = fragmentShaderModule.GetModule();
  fragInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

  return { vertexInfo, fragInfo };
}

void Pipeline::CreatePipeline(VkDevice device, VkViewport viewport, VkRect2D scissor) {

  /*
    Dynamic states allows us to specifies certain states
    of the pipeline that can be dynamically changed at runtime.
    Here, we've specified the VIEWPORT (which describes a cartesian
    plane, of which we'll use its coordenates) and SCISSOR (specifies
    the visible area of the viewport, allows us to "cut" the screen as
    to not show a certain region).
    If we didn't specify any dynamic states, we'd have to set them up
    right now, on pipeline creation.
    This creates a problem: if we were to change these values, for any
    reason, we'd have to recreate the entire pipeline just for that.
    Although we WILL have to create many different pipelines, this would
    make it so we have many extremely similar pipelines, only differing
    in tiny aspects.
  */
  std::vector<VkDynamicState> dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };

  /*
    To create the pipeline, we must create a few other structs that
    describe every "piece" of the entire pipeline.
    These will all be linked together into one final struct.
  */

  // Describes dynamic states
  VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
  dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicStateInfo.pDynamicStates = dynamicStates.data();
  dynamicStateInfo.dynamicStateCount = dynamicStates.size();

  // Describes vertex data
  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  // Copied directly from `vulkan-tutorial.com`
  // Bindings: spacing between data and whether the data is per-vertex or per-instance (see instancing)
  // Attribute descriptions: type of the attributes passed to the vertex shader, which binding to load them from and at which offset
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions = nullptr;
  vertexInputInfo.pVertexBindingDescriptions = nullptr;

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
  inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssemblyInfo.primitiveRestartEnable = VK_TRUE;

  VkPipelineViewportStateCreateInfo viewportInfo{};
  viewportInfo.sType= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  // Since we're using VIEWPORT and SCISSOR as dynamic states,
  // we only pass in the count
  viewportInfo.viewportCount = 1;
  viewportInfo.scissorCount = 1;


  /* 
  * We'd need to set them up on creation like this if we weren't using
  * them as DynamicStates.
  * We will need to set them before drawing, however.

  ? VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;
  */

  VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
  rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

  // If enabled, fragments that don't fall into the near and far plane are clamped
  // If disabled, they are discarted
  rasterizerInfo.depthClampEnable = VK_FALSE;

  // If enabled, we basically disable the output buffer, as no data gets
  // passed to the rasterization stage
  rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;

  rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL; // other modes depend on GPU feature
  rasterizerInfo.lineWidth = 1.0f; // lines thicker depend on a GPU feature

  rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT; // cull back face
  rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE; // how to determine front face

  // Depth Bias is sometimes used for shadowmapping, but we wont't need it now
  // Since we've disabled it, the other parameters referring to Depth Bias 
  // are ignored
  rasterizerInfo.depthBiasEnable = VK_FALSE;

  // Deals with Multisampling. We'll come back for it, so for now
  // I'll leave it disabled
  VkPipelineMultisampleStateCreateInfo multisampleInfo{};
  multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampleInfo.sampleShadingEnable = VK_FALSE;
  multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  // We could create a struct for Depth Stencil, but it won't
  // be necessary for now, and we can just pass in a nullptr
  //? VkPipelineDepthStencilStateCreateInfo stencilInfo{};

  /*
    Ok, let's go.
    Attachments can be of two types:
      - input (e.g.: color blending) -> render passes reads them
      - output (e.g.: color, depth) -> render passes write into them

    Attachments compose the Framebuffer, and are read and written to
    during multiple sub-passes of the active render pass.

    In here, we are declaring a COlorBlendAttachment, that will blend
    color and write it to the color attachment of the image we'll show
    to the surface.

    I don't understand it enough to have an intuition on what it actually
    does, but a great resource for anyone who's reading this (me), is the
    official Vulkan Glossary about Framebuffers.

    Have at it -> https://registry.khronos.org/vulkan/specs/1.2/html/chap8.html#_framebuffers
  */
 
  // I still don't understand every option of both Attachment
  // and Create Info of Color Blending, but this should
  // give us regular alpha blending.
  VkPipelineColorBlendAttachmentState blendingAttachment{};
  blendingAttachment.blendEnable = VK_TRUE;
  blendingAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  blendingAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  blendingAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  blendingAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  blendingAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  blendingAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo blendingInfo{};
  blendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  blendingInfo.logicOpEnable = VK_FALSE; // overrides blendEnable if 
  blendingInfo.logicOp = VK_LOGIC_OP_COPY;
  blendingInfo.attachmentCount = 1;
  blendingInfo.pAttachments = &blendingAttachment;
  blendingInfo.blendConstants[0] = 0.0f;
  blendingInfo.blendConstants[1] = 0.0f;
  blendingInfo.blendConstants[2] = 0.0f;
  blendingInfo.blendConstants[3] = 0.0f;

  // Pipeline Layout specifies uniforms in our shaders
  // For now, we'll leave everything at zero
  VkPipelineLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount = 0;
  layoutInfo.pSetLayouts = nullptr;
  layoutInfo.pushConstantRangeCount = 0;
  layoutInfo.pPushConstantRanges = nullptr;

  if(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
    throw std::runtime_error("Error creating Pipeline Layout");
  }
}

void Pipeline::Destroy(VkDevice device) {
  vkDestroyPipelineLayout(device, layout, nullptr);
  vkDestroyPipeline(device, pipeline, nullptr);
  vertexShaderModule.Destroy(device);
  fragmentShaderModule.Destroy(device);
}