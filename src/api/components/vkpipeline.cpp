#include "vkpipeline.hpp"
#include <vector>

Pipeline:: Pipeline() {}

Pipeline:: Pipeline(VkDevice device)
  : vertexShaderModule(RESOURCES"shaders/basic.vert.spv", device),
    fragmentShaderModule(RESOURCES"shaders/basic.frag.spv", device) 
  {
}

std::vector<VkPipelineShaderStageCreateInfo> Pipeline::CreateShaderStages() {
  auto vertexModule = vertexShaderModule.GetModule();
  auto fragmentModule = fragmentShaderModule.GetModule();

  VkPipelineShaderStageCreateInfo vertexInfo{};
  vertexInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertexInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
  vertexInfo.pName  = "main";
  vertexInfo.module = vertexModule;

  VkPipelineShaderStageCreateInfo fragInfo{};
  fragInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragInfo.pName  = "main";
  fragInfo.module = fragmentModule;

  return { vertexInfo, fragInfo };
}

void Pipeline::CreateRenderPass(VkDevice device, VkFormat format) {

  // We need to create some attachments first,
  // namely: `color` and `depth`
  VkAttachmentDescription colorDescriptor{};
  colorDescriptor.format = format;
  // No multisampling yet, so just 1 sample for now
  colorDescriptor.samples = VK_SAMPLE_COUNT_1_BIT;

  // When attachment is loaded, clear it (could ignore or 
  // load from somewhere else)
  // We're clearing so there's no ghost images
  colorDescriptor.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  // When storing attachment, just store (could also discard)
  colorDescriptor.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  // Since we won't be using the Stencil for now, we don't
  // care about the data
  colorDescriptor.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorDescriptor.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

  // This tells in which way are the layout of the pixels
  // We don't care about the initial layout because we are
  // overriting it. If we had specified different operations,
  // we would've had to be more mindful about the initial state
  //* Images are transitioned `from` and `to` different layouts, 
  //* depending on their usage
  colorDescriptor.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  // Same thing applies for the final layout. In this case,
  // this layout is required for images that are going to be
  // presented, so the attachment will be stored this way
  colorDescriptor.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  /*
    Attachments are described with the `VkAttachmentDescription` struct.
    Subpasses reference these attachments with the `VkAttachmentReference`
    struct.
  */

  VkAttachmentReference colorAttachRef{};
  // This is the index of the attachment
  // We only have one right now, so it's 0
  colorAttachRef.attachment = 0;
  // We specify the layout of a Color Image, with the OPTIMAL keyword
  // This will make sure Vulkan will optimize the layout for color attachments
  colorAttachRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  // Will extend as we add more attachments
  using std:: vector;

  vector<VkAttachmentDescription> attachmentDescriptions {
    colorDescriptor
  };

  vector<VkAttachmentReference> attachmentReferences {
    colorAttachRef
  };

  VkSubpassDescription subpassDescription{};
  // We bind this Subpass to the Graphics operations (could bind to compute, etc.) 
  subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

  // The index of the attachment is directly referend by the fragment shader

  /* Example:
   ## In the fragment shader...
  *## layout(location=0) out vec4 outColor;

  The "location=0" part is directly referencing the attachment 
  with index 0, which, in this case, is the  
  */
  subpassDescription.colorAttachmentCount = attachmentReferences.size();
  subpassDescription.pColorAttachments    = attachmentReferences.data();

  vector<VkSubpassDescription> subpasses {
    subpassDescription
  };

  // FINALLY, we create our RenderPass.
  // We're ever so slightly closer to our triangle
  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

  renderPassInfo.attachmentCount = attachmentDescriptions.size();
  renderPassInfo.pAttachments    = attachmentDescriptions.data();

  renderPassInfo.subpassCount = subpasses.size();
  renderPassInfo.pSubpasses   = subpasses.data();

  if(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
    throw std:: runtime_error("Failure on creating Render Pass");
  }
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
  dynamicStateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicStateInfo.pDynamicStates    = dynamicStates.data();
  dynamicStateInfo.dynamicStateCount = dynamicStates.size();

  // Describes vertex data
  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  // Copied directly from `vulkan-tutorial.com`
  // Bindings: spacing between data and whether the data is per-vertex or per-instance (see instancing)
  // Attribute descriptions: type of the attributes passed to the vertex shader, which binding to load them from and at which offset
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.vertexBindingDescriptionCount   = 0;
  vertexInputInfo.pVertexAttributeDescriptions    = nullptr;
  vertexInputInfo.pVertexBindingDescriptions      = nullptr;

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
  inputAssemblyInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssemblyInfo.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewportInfo{};
  viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  // Since we're using VIEWPORT and SCISSOR as dynamic states,
  // we only pass in the count
  viewportInfo.viewportCount = 1;
  viewportInfo.scissorCount  = 1;


  /* 
  * We'd need to set them up on creation like this if we weren't using
  * them as DynamicStates.
  * We will need to set them before drawing, however.

  ? VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports    = &viewport;
  viewportState.scissorCount  = 1;
  viewportState.pScissors     = &scissor;
  */

  VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
  rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

  // If enabled, fragments that don't fall into the near and far plane are clamped
  // If disabled, they are discarted
  rasterizerInfo.depthClampEnable = VK_FALSE;

  // If enabled, we basically disable the output buffer, as no data gets
  // passed to the rasterization stage
  rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;

  rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;  // other modes depend on GPU feature
  rasterizerInfo.lineWidth   = 1.0f;                  // lines thicker depend on a GPU feature

  rasterizerInfo.cullMode  = VK_CULL_MODE_BACK_BIT;    // cull back face
  rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;  // how to determine front face

  // Depth Bias is sometimes used for shadowmapping, but we wont't need it now
  // Since we've disabled it, the other parameters referring to Depth Bias 
  // are ignored
  rasterizerInfo.depthBiasEnable = VK_FALSE;

  // Deals with Multisampling. We'll come back for it, so for now
  // I'll leave it disabled
  VkPipelineMultisampleStateCreateInfo multisampleInfo{};
  multisampleInfo.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampleInfo.sampleShadingEnable  = VK_FALSE;
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

    Have at it -> https: //registry.khronos.org/vulkan/specs/1.2/html/chap8.html#_framebuffers
  */
 
  // I still don't understand every option of both Attachment
  // and Create Info of Color Blending, but this should
  // give us regular alpha blending.
  VkPipelineColorBlendAttachmentState blendingAttachment{};
  blendingAttachment.blendEnable         = VK_TRUE;
  blendingAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  blendingAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  blendingAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
  blendingAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  blendingAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  blendingAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo blendingInfo{};
  blendingInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  blendingInfo.logicOpEnable     = VK_FALSE; // overrides blendEnable if 
  blendingInfo.logicOp           = VK_LOGIC_OP_COPY;
  blendingInfo.attachmentCount   = 1;
  blendingInfo.pAttachments      = &blendingAttachment;
  blendingInfo.blendConstants[0] = 0.0f;
  blendingInfo.blendConstants[1] = 0.0f;
  blendingInfo.blendConstants[2] = 0.0f;
  blendingInfo.blendConstants[3] = 0.0f;

  // Pipeline Layout specifies uniforms in our shaders
  // For now, we'll leave everything at zero
  VkPipelineLayoutCreateInfo layoutInfo{};
  layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount         = 0;
  layoutInfo.pSetLayouts            = nullptr;
  layoutInfo.pushConstantRangeCount = 0;
  layoutInfo.pPushConstantRanges    = nullptr;

  if(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
    throw std:: runtime_error("Error creating Pipeline Layout");
  }

  auto shaderStages = CreateShaderStages();

  // FINALLY WE CREATE THE PIPELINE
  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

  pipelineInfo.pViewportState      = &viewportInfo;
  pipelineInfo.pDynamicState       = &dynamicStateInfo;
  pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
  pipelineInfo.pVertexInputState   = &vertexInputInfo;
  pipelineInfo.pRasterizationState = &rasterizerInfo;
  pipelineInfo.pMultisampleState   = &multisampleInfo;
  pipelineInfo.pColorBlendState    = &blendingInfo;

  pipelineInfo.pDepthStencilState = nullptr;
  pipelineInfo.pTessellationState = nullptr;

  // Shaders
  pipelineInfo.stageCount = shaderStages.size();
  pipelineInfo.pStages    = shaderStages.data();

  pipelineInfo.layout = layout;

  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass    = 0;

// Vulkan allows us to create pipelines based on other ones,
// to make the process easier. We can then specify the base
// pipeline handle, which will serve as a base (duh) for the
// new one
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex  = -1;

  // After all that, we can finally create our dreamed pipeline

  if(
    vkCreateGraphicsPipelines(
      device, 
      VK_NULL_HANDLE, 
      1,
      &pipelineInfo,
      nullptr,
      &pipeline
    ) != VK_SUCCESS) {
    throw std::runtime_error("Failure on creating Graphics Pipeline");
  }
}

void Pipeline::Destroy(VkDevice device) {
  vkDestroyRenderPass(device, renderPass, nullptr);
  vkDestroyPipelineLayout(device, layout, nullptr);
  vkDestroyPipeline(device, pipeline, nullptr);
  vertexShaderModule.Destroy(device);
  fragmentShaderModule.Destroy(device);
}