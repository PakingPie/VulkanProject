#include "lve_pipeline.hpp"
#include "lve_model.hpp"

#include <vector>
#include <fstream>
#include <iostream>
#include <cassert>

#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif

namespace lve {
	LvePipeline::LvePipeline(LveDevice& device, const std::string& vert_file_path, const std::string& frag_file_path, const PipelineConfigInfo& configInfo) : lveDevice(device) {
		createGraphicsPipeline(vert_file_path, frag_file_path, configInfo);
	}

	LvePipeline::~LvePipeline() {
		vkDestroyShaderModule(lveDevice.device(), fragShaderModule, nullptr);
		vkDestroyShaderModule(lveDevice.device(), vertShaderModule, nullptr);
		vkDestroyPipeline(lveDevice.device(), graphicsPipeline, nullptr);
	}

	void LvePipeline::bind(VkCommandBuffer commandBuffer) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
	}

	std::vector<char> LvePipeline::readFile(const std::string& filename) {
		std::ifstream file(ENGINE_DIR + filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}

	void LvePipeline::createGraphicsPipeline(const std::string& vert_file_path, const std::string& frag_file_path, const PipelineConfigInfo& configInfo) {
		
		assert(configInfo.pipelineLayout != VK_NULL_HANDLE && "Cannot create graphics pipeline:: no pipelineLayout provided in configInfo");
		assert(configInfo.renderPass != VK_NULL_HANDLE && "Cannot create graphics pipeline:: no renderPass provided in configInfo");
		
		auto vertShaderCode = readFile(vert_file_path);
		auto fragShaderCode = readFile(frag_file_path);

		createShaderModule(vertShaderCode, &vertShaderModule);
		createShaderModule(fragShaderCode, &fragShaderModule);

		VkPipelineShaderStageCreateInfo shaderStages[2];
		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStages[0].module = vertShaderModule;
		shaderStages[0].pName = "main";
		shaderStages[0].flags = 0;
		shaderStages[0].pNext = nullptr;
		shaderStages[0].pSpecializationInfo = nullptr;

		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStages[1].module = fragShaderModule;
		shaderStages[1].pName = "main";
		shaderStages[1].flags = 0;
		shaderStages[1].pNext = nullptr;
		shaderStages[1].pSpecializationInfo = nullptr;

		auto& bindingDescriptions = configInfo.bindingDescriptions;
		auto& attributeDescriptions = configInfo.attributeDescriptions;

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data(); // Optional
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // Optional

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
		pipelineInfo.pViewportState = &configInfo.viewportInfo;
		pipelineInfo.pRasterizationState = &configInfo.rasterizerInfo;
		pipelineInfo.pMultisampleState = &configInfo.multisamplInfo;
		pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
		pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo; // Optional
		pipelineInfo.pDynamicState = &configInfo.dynamicStateInfo;

		pipelineInfo.layout = configInfo.pipelineLayout;
		pipelineInfo.renderPass = configInfo.renderPass;
		pipelineInfo.subpass = configInfo.subpass;

		pipelineInfo.basePipelineIndex = -1; // Optional
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional

		if (vkCreateGraphicsPipelines(lveDevice.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}
	}

	VkShaderModule LvePipeline::createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		if (vkCreateShaderModule(lveDevice.device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}
	}

	void LvePipeline::defaultPipelineConfigInfo(PipelineConfigInfo& configInfo) {
		configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

		configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		configInfo.viewportInfo.viewportCount = 1;
		configInfo.viewportInfo.pViewports = nullptr;
		configInfo.viewportInfo.scissorCount = 1;
		configInfo.viewportInfo.pScissors = nullptr;

		configInfo.rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		configInfo.rasterizerInfo.depthClampEnable = VK_FALSE;
		configInfo.rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
		configInfo.rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
		configInfo.rasterizerInfo.lineWidth = 1.0f;
		configInfo.rasterizerInfo.cullMode = VK_CULL_MODE_NONE;
		configInfo.rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		configInfo.rasterizerInfo.depthBiasEnable = VK_FALSE;
		configInfo.rasterizerInfo.depthBiasConstantFactor = 0.0f;						// Optional
		configInfo.rasterizerInfo.depthBiasClamp = 0.0f;								// Optional
		configInfo.rasterizerInfo.depthBiasSlopeFactor = 0.0f;							// Optional

		configInfo.multisamplInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		configInfo.multisamplInfo.sampleShadingEnable = VK_FALSE;
		configInfo.multisamplInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		configInfo.multisamplInfo.minSampleShading = 1.0f;								// Optional
		configInfo.multisamplInfo.pSampleMask = nullptr;								// Optional
		configInfo.multisamplInfo.alphaToCoverageEnable = VK_FALSE;						// Optional
		configInfo.multisamplInfo.alphaToOneEnable = VK_FALSE;							// Optional

		configInfo.colorBlendAttachment.colorWriteMask = 
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
		configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;		// Optional
		configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;		// Optional
		configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;					// Optional
		configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;		// Optional
		configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;		// Optional
		configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;					// Optional

		configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
		configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
		configInfo.colorBlendInfo.attachmentCount = 1;
		configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
		configInfo.colorBlendInfo.blendConstants[0] = 0.0f;								// Optional
		configInfo.colorBlendInfo.blendConstants[1] = 0.0f;								// Optional
		configInfo.colorBlendInfo.blendConstants[2] = 0.0f;								// Optional
		configInfo.colorBlendInfo.blendConstants[3] = 0.0f;								// Optional

		configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
		configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
		configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
		configInfo.depthStencilInfo.minDepthBounds = 0.0f;
		configInfo.depthStencilInfo.maxDepthBounds = 1.0f;
		configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
		configInfo.depthStencilInfo.front = {};											// Optional
		configInfo.depthStencilInfo.back = {};											// Optional

		configInfo.dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();
		configInfo.dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
		configInfo.dynamicStateInfo.flags = 0;

		configInfo.bindingDescriptions = LveModel::Vertex::getBindingDescriptions();
		configInfo.attributeDescriptions = LveModel::Vertex::getAttributeDescriptions();
	}

	void LvePipeline::enableAlphaBlending(PipelineConfigInfo& configInfo) {
		configInfo.colorBlendAttachment.blendEnable = VK_TRUE;

		configInfo.colorBlendAttachment.colorWriteMask = 
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;				// Optional
		configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;		// Optional
		configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;									// Optional
		configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;		// Optional
		configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;		// Optional
		configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;					// Optional
	}
}