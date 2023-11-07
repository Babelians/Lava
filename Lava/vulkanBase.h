#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include <windows.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_layer.h>
#include <vector>
#include <fstream>
#include <iostream>
#include "vk_mem_alloc.h"

#pragma comment(lib, "vulkan-1.lib")

using namespace std;

class VulkanBase
{
public:
	VulkanBase();
	virtual ~VulkanBase() {};
	void initialize(GLFWwindow* window, const char* appTitle);
	void terminate();
	void errorLog();
protected:
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	vector<VkPhysicalDevice> physicalDevices;
	VkDevice device;
	uint32_t queueFamilyIndex;
	VmaAllocator allocator;
	VkBuffer stagingBuffer;
	VkBuffer deviceLocalBuffer;
	VmaAllocation stagingBufferAllocation;
	VmaAllocation deviceLocalBufferAllocation;
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
	VkFence fence;
	VkShaderModule shaderModule;
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	void createInstance(const char* appTitle);
	void selectPhysicalDevices();
	void createDevice();
	void createMemoryAllocator();
	void createStagingBuffer();
	void createDeviceLocalBuffer();
	void createCommandPool();
	void createCommandBuffer();
	void copyBuffer();
	void createFence();
	void flowQueue(VkQueue queue);
	void createDescriptorPool();
	void createDescriptorSetLayout();
	void createDescriptorSet();
	void updateDescriptorSet();
	VkShaderModule createShaderModule(const char* fileName);
	vector<const char*>errors;
};
