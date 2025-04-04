#pragma once

#include "point_light_system.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>
#include <cassert>
#include <map>

namespace lve {
	struct PointLightPushConstants {
		glm::vec4 position;
		glm::vec4 color;
		float radius;
	};

	PointLightSystem::PointLightSystem(LveDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : lveDevice(device) {
		createPipelineLayout(globalSetLayout);
		createPipeline(renderPass);
	}
	PointLightSystem::~PointLightSystem() {
		vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
	}

	void PointLightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PointLightPushConstants);

		std::vector<VkDescriptorSetLayout>desciptorSetLayouts{ globalSetLayout };

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(desciptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = desciptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		if (vkCreatePipelineLayout(lveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}
	}

	void PointLightSystem::createPipeline(VkRenderPass renderPass) {
		assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");
		PipelineConfigInfo pipelineConfig{};
		LvePipeline::defaultPipelineConfigInfo(pipelineConfig);
		LvePipeline::enableAlphaBlending(pipelineConfig);
      pipelineConfig.attributeDescriptions.clear();
      pipelineConfig.bindingDescriptions.clear();
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		lvePipeline = std::make_unique<LvePipeline>(
			lveDevice,
			"shaders/point_light.vert.spv",
			"shaders/point_light.frag.spv",
			pipelineConfig
		);
	}
	void PointLightSystem::update(FrameInfo& frameInfo, GlobalUbo& ubo) {
		auto rotateLight = glm::rotate(glm::mat4(1.f), 0.5f * frameInfo.frameTime, { 0.f, -1.f, 0.f });
		int lightIndex = 0;
		for (auto& keyValue : frameInfo.gameObjects) {
			auto& gameObject = keyValue.second;
			if (gameObject.pointLight == nullptr) continue;

			assert(lightIndex < MAX_POINT_LIGHTS && "Point lights exceed maximum specified");
			gameObject.transform.translation = glm::vec3(rotateLight * glm::vec4(gameObject.transform.translation, 1.f));
			ubo.pointLights[lightIndex].position = glm::vec4(gameObject.transform.translation, 1.0f);
			ubo.pointLights[lightIndex].color = glm::vec4(gameObject.color, gameObject.pointLight->lightIntensity);

			lightIndex++;
		}
		ubo.numPointLights = lightIndex;
	}
	void PointLightSystem::render(FrameInfo& frameInfo) {
		std::map<float, LveGameObject::id_t> sortedPointLightsToCameraDistance;
		for (auto& keyValue : frameInfo.gameObjects) {
			auto& gameObject = keyValue.second;
			if (gameObject.pointLight == nullptr) continue;
			float distance = glm::dot(frameInfo.camera.getPosition() - gameObject.transform.translation, frameInfo.camera.getPosition() - gameObject.transform.translation);
			sortedPointLightsToCameraDistance[distance] = gameObject.getId();
		}

		lvePipeline->bind(frameInfo.commandBuffer);

		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&frameInfo.globalDescriptorSet,
			0,
			nullptr
		);

		for (auto it = sortedPointLightsToCameraDistance.rbegin(); it != sortedPointLightsToCameraDistance.rend(); it++) {
			auto& gameObject = frameInfo.gameObjects.at(it->second);
			PointLightPushConstants push{};
			push.position = glm::vec4(gameObject.transform.translation, 1.0f);
			push.color = glm::vec4(gameObject.color, gameObject.pointLight->lightIntensity);
			push.radius = gameObject.transform.scale.x;
			vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PointLightPushConstants), &push);
			vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
		}
	}
}