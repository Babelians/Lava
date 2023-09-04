#include "vulkanBase.h"
#include <iostream>

void VulkanBase::errorLog()
{
	if (errors.size())
	{
		for (const auto* i : errors)
		{
			OutputDebugStringA(i);
		}
	}
}

VulkanBase::VulkanBase()
{

}

void VulkanBase::initialize(GLFWwindow* window, const char* appTitle)
{
	createInstance(appTitle);
	selectPhysicalDevices();
	createDevice();
}

void VulkanBase::createInstance(const char* appTitle)
{
	VkApplicationInfo applicationInfo{};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pNext = nullptr;
	applicationInfo.pApplicationName = appTitle;
	applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.pEngineName = "Lava";
	applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.apiVersion = VK_API_VERSION_1_2;

	vector<const char*> layers 
	{
		"VK_LAYER_KHRONOS_validation"
	};

	vector<const char*> extensions;
	vector<VkExtensionProperties> properties;
	{
		uint32_t count;
		vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
		properties.resize(count);
		vkEnumerateInstanceExtensionProperties(nullptr, &count, properties.data());

		for (const auto& v : properties)
		{
			extensions.push_back(v.extensionName);
		}
	}

	VkInstanceCreateInfo instanceCreateInfo{};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = nullptr;
	instanceCreateInfo.flags = 0;
	instanceCreateInfo.pApplicationInfo = &applicationInfo;
	instanceCreateInfo.enabledLayerCount = uint32_t(layers.size());
	instanceCreateInfo.ppEnabledLayerNames = layers.data();
	instanceCreateInfo.enabledExtensionCount = uint32_t(extensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

	if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance) != VK_SUCCESS)
	{
		errors.push_back("Create instance failed");
	}
}

void VulkanBase::selectPhysicalDevices()
{

	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		physicalDevices.resize(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());
	}

	//Confirm GPU supporting for Vulkan
	for (const auto& pDevice : physicalDevices)
	{
		VkPhysicalDeviceProperties2 props{};
		VkPhysicalDeviceVulkan11Properties props11{};
		props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		props.pNext = &props11;
		props11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
		props11.pNext = nullptr;
		vkGetPhysicalDeviceProperties2(pDevice, &props); 
		VkPhysicalDeviceFeatures2 features;
		VkPhysicalDeviceVulkan11Features features11;
		features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		features.pNext = &features11;
		features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
		features11.pNext = nullptr;
		vkGetPhysicalDeviceFeatures2(pDevice, &features);
	}

	//Select GPU
	physicalDevice = physicalDevices[0];
}

void VulkanBase::createDevice()
{
	vector<VkQueueFamilyProperties> queueProps;
	//GPUに備わっているキューをを調べる。
	{   
		uint32_t queuePropCount;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queuePropCount, nullptr);
		queueProps.resize(queuePropCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queuePropCount, queueProps.data());
	}

	uint32_t queueFamilyIndex = 0u;
	//配列の中から望むGPUキューファミリーを選択。
	for (uint32_t i = 0; i < queueProps.size(); i++)
	{
		if (queueProps[i].queueFlags & VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT)
		{
			queueFamilyIndex = i;
			break;
		}
	}

	const float priority = 0.0f;
	VkDeviceQueueCreateInfo devQueueCreateInfo{};
	devQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	devQueueCreateInfo.pNext = nullptr;
	devQueueCreateInfo.flags = 0;
	devQueueCreateInfo.queueFamilyIndex = queueFamilyIndex;
	devQueueCreateInfo.queueCount = 1;
	devQueueCreateInfo.pQueuePriorities = &priority;

	vector<const char*> extension;
	vector<VkExtensionProperties> devExtensionProps;
	{
		uint32_t count = 0;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, nullptr);
		devExtensionProps.resize(count);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, devExtensionProps.data());
	}

	for (const auto& v : devExtensionProps)
	{
		extension.push_back(v.extensionName);
	}

	VkDeviceCreateInfo devCreateInfo{};
	devCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	devCreateInfo.pNext = nullptr;
	devCreateInfo.flags = 0;
	devCreateInfo.queueCreateInfoCount = 1;
	devCreateInfo.pQueueCreateInfos = &devQueueCreateInfo;
	devCreateInfo.enabledLayerCount = 0;
	devCreateInfo.ppEnabledLayerNames = nullptr;
	devCreateInfo.enabledExtensionCount = uint32_t(extension.size());
	devCreateInfo.ppEnabledExtensionNames = extension.data();
	devCreateInfo.pEnabledFeatures = nullptr;
	
	if (vkCreateDevice(physicalDevice, &devCreateInfo, nullptr, &device) != VK_SUCCESS)
	{
		errors.push_back("Create device failed");
		errorLog();
	}
}

void VulkanBase::terminate()
{

}