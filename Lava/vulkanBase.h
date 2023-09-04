#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include <windows.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan_win32.h>
#include <vulkan/vk_layer.h>
#include <vector>
//Link for Vulkan library
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
	void createInstance(const char* appTitle);
	void selectPhysicalDevices();
	void createDevice();
	vector<const char*>errors;
};
