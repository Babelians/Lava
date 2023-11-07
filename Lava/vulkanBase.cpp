#include "vulkanBase.h"

void VulkanBase::errorLog()
{
	if (errors.size())
	{
		OutputDebugStringA("=====Error=====\n");
		for (const auto* i : errors)
		{
			OutputDebugStringA(i);
			OutputDebugStringA("\n");
		}
		OutputDebugStringA("===============\n");
	}
	else
	{
		OutputDebugStringA("=====Non Error=====\n");
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
	createMemoryAllocator();
	createStagingBuffer();
	createDeviceLocalBuffer();
	createCommandPool();
	createCommandBuffer();
	createFence();
	shaderModule = createShaderModule("../Lava/SPIR-V/add.comp.spv");
	createDescriptorPool();
	createDescriptorSetLayout();
	createDescriptorSet();
	errorLog();
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

	VkInstanceCreateInfo instanceCI{};
	instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCI.pNext = nullptr;
	instanceCI.flags = 0;
	instanceCI.pApplicationInfo = &applicationInfo;
	instanceCI.enabledLayerCount = uint32_t(layers.size());
	instanceCI.ppEnabledLayerNames = layers.data();
	instanceCI.enabledExtensionCount = uint32_t(extensions.size());
	instanceCI.ppEnabledExtensionNames = extensions.data();

	if (vkCreateInstance(&instanceCI, nullptr, &instance) != VK_SUCCESS)
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

	queueFamilyIndex = 0u;
	//配列の中から望むGPUキューファミリー(汎用計算)を選択。
	for (uint32_t i = 0; i < queueProps.size(); i++)
	{
		if (queueProps[i].queueFlags & VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT)
		{
			queueFamilyIndex = i;
			break;
		}
	}

	const float priority = 0.0f;
	VkDeviceQueueCreateInfo devQueueCI{};
	devQueueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	devQueueCI.pNext = nullptr;
	devQueueCI.flags = 0;
	devQueueCI.queueFamilyIndex = queueFamilyIndex;
	devQueueCI.queueCount = 1;
	devQueueCI.pQueuePriorities = &priority;

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

	VkDeviceCreateInfo devCI{};
	devCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	devCI.pNext = nullptr;
	devCI.flags = 0;
	devCI.queueCreateInfoCount = 1;
	devCI.pQueueCreateInfos = &devQueueCI;
	devCI.enabledLayerCount = 0;
	devCI.ppEnabledLayerNames = nullptr;
	devCI.enabledExtensionCount = uint32_t(extension.size());
	devCI.ppEnabledExtensionNames = extension.data();
	devCI.pEnabledFeatures = nullptr;
	
	if (vkCreateDevice(physicalDevice, &devCI, nullptr, &device) != VK_SUCCESS)
	{
		errors.push_back("Create device failed");
	}
}

void VulkanBase::createMemoryAllocator()
{
	//アロケータを作る
	VmaAllocatorCreateInfo allocatorCI{};
	allocatorCI.instance = instance;
	allocatorCI.physicalDevice = physicalDevice;
	allocatorCI.device = device;
	if (vmaCreateAllocator(&allocatorCI, &allocator) != VK_SUCCESS)
	{
		errors.push_back("vmaCreateAllocator failled in createStagingBuffer");
	}
}

//CPUからもGPUからも見えるメインメモリ。CPUからコピーされたデータをGPUがGPUのVRAMにコピーする。
void VulkanBase::createStagingBuffer()
{
	//ステージングバッファを作る
	VmaAllocationCreateInfo stagingBufferAllocInfo{};
	stagingBufferAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; //CPUとGPUが見れるメモリで、GPUが高速に処理できるもの。
	VkBufferCreateInfo bufferCI{};
	bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCI.pNext = nullptr;
	bufferCI.flags = 0;
	bufferCI.size = 1024u;
	bufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT || VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCI.queueFamilyIndexCount = 0;
	bufferCI.pQueueFamilyIndices = nullptr;
	if (vmaCreateBuffer(allocator, &bufferCI, &stagingBufferAllocInfo, &stagingBuffer, &stagingBufferAllocation, nullptr) != VK_SUCCESS)
	{
		errors.push_back("vmaCreateBuffer failled in createStagingBuffer");
	}
}

//GPUからのみ見えるメインメモリ。CPUからコピーされたデータをGPUがGPUのVRAMにコピーする。
void VulkanBase::createDeviceLocalBuffer()
{
	//デバイスローカルバッファを作る
	VmaAllocationCreateInfo deviceLocalBufferAllocInfo{};
	deviceLocalBufferAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY; //CPUとGPUが見れるメモリで、GPUが高速に処理できるもの。
	VkBufferCreateInfo bufferCI{};
	bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCI.pNext = nullptr;
	bufferCI.flags = 0;
	bufferCI.size = 1024u;
	bufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT || VK_BUFFER_USAGE_TRANSFER_DST_BIT || VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCI.queueFamilyIndexCount = 0;
	bufferCI.pQueueFamilyIndices = nullptr;
	if (vmaCreateBuffer(allocator, &bufferCI, &deviceLocalBufferAllocInfo, &deviceLocalBuffer, &deviceLocalBufferAllocation, nullptr) != VK_SUCCESS)
	{
		errors.push_back("vmaCreateBuffer failled in createDeviceLocalBuffer");
	}
}

//コマンドバッファを割り当てるためのコマンドプールを作る。
void VulkanBase::createCommandPool()
{
	VkCommandPoolCreateInfo commandPoolCI{};
	commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCI.pNext = nullptr;
	commandPoolCI.flags = 0u;
	commandPoolCI.queueFamilyIndex = queueFamilyIndex;
	if (vkCreateCommandPool(device, &commandPoolCI, nullptr, &commandPool) != VK_SUCCESS)
	{
		errors.push_back("vkCreateCommandPool is failed in createCommandPool");
	}
}

void VulkanBase::createCommandBuffer()
{
	VkCommandBufferAllocateInfo commandBufferAllocInfo{};
	commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocInfo.pNext = nullptr;
	commandBufferAllocInfo.commandPool = commandPool;
	commandBufferAllocInfo.commandBufferCount = 1u;
	commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	if (vkAllocateCommandBuffers(device, &commandBufferAllocInfo, &commandBuffer) != VK_SUCCESS)
	{
		errors.push_back("vkAllocateCommandBuffers is failed in createCommandBuffer");
	}
}

void VulkanBase::copyBuffer()
{
	VkCommandBufferBeginInfo commandBufferBeginInfo{};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	commandBufferBeginInfo.pInheritanceInfo = VK_NULL_HANDLE;
	if (vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
	{
		errors.push_back("vkBeginCommandBuffer is failed in recordCommand");
	}
	VkBufferCopy region{};
	region.srcOffset = 0u;
	region.dstOffset = 0u;
	region.size = 1000u;
	vkCmdCopyBuffer(commandBuffer, stagingBuffer, deviceLocalBuffer, 1u, &region);
	vkEndCommandBuffer(commandBuffer);
}

void VulkanBase::createFence()
{
	VkFenceCreateInfo fenceCI{};
	fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCI.pNext = nullptr;
	fenceCI.flags = 0u;
	if (vkCreateFence(device, &fenceCI, nullptr, &fence) != VK_SUCCESS)
	{
		errors.push_back("vkCreateFence is failed");
	}
}

void VulkanBase::flowQueue(VkQueue queue)
{
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 0u;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1u;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.signalSemaphoreCount = 0u;
	submitInfo.pSignalSemaphores = nullptr;
	if (vkQueueSubmit(queue, 1u, &submitInfo, fence))
	{
		errors.push_back("vkQueueSubmit is failed");
	}
}

VkShaderModule VulkanBase::createShaderModule(const char* fileName)
{
	fstream file(fileName, ios::in | ios::binary);
	if (!file.good())
	{
		errors.push_back("opening spv file is failed in createShaderModule");
	}
	vector<uint8_t>code;
	code.assign(
		istreambuf_iterator<char>(file),
		istreambuf_iterator<char>()
	);
	VkShaderModuleCreateInfo shaderModuleCI{};
	shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCI.pNext = nullptr;
	shaderModuleCI.flags = 0u;
	shaderModuleCI.codeSize = code.size();
	shaderModuleCI.pCode = reinterpret_cast<const uint32_t*>(code.data());
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &shaderModuleCI, nullptr, &shaderModule) != VK_SUCCESS)
	{
		errors.push_back("vkCreateShaderModule is failed in createShaderModule");
	}
	return shaderModule;
}

void VulkanBase::createDescriptorPool()
{
	VkDescriptorPoolSize descriptorPoolSize{};
	descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorPoolSize.descriptorCount = 5u;

	VkDescriptorPoolCreateInfo descriptorPoolCI{};
	descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCI.pNext = nullptr;
	descriptorPoolCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	descriptorPoolCI.maxSets = 10u;
	descriptorPoolCI.poolSizeCount = 1u;
	descriptorPoolCI.pPoolSizes = &descriptorPoolSize;
	if (vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		errors.push_back("vkCreateDescriptorPool is failed in createDescriptorPool");
	}
}

void VulkanBase::createDescriptorSetLayout()
{
	//必要なデスクリプタを指定
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding{};
	//binding=0に結び付ける
	descriptorSetLayoutBinding.binding = 0;
	descriptorSetLayoutBinding.descriptorCount = 1u;
	descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorSetLayoutBinding.pImmutableSamplers = nullptr;
	descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	//デスクリプタセットレイアウトを作る
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
	descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCI.flags = 0u;
	descriptorSetLayoutCI.pNext = nullptr;
	descriptorSetLayoutCI.bindingCount = 1u;
	descriptorSetLayoutCI.pBindings = &descriptorSetLayoutBinding;
	if (vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayout) != VK_SUCCESS)
	{
		errors.push_back("vkCreateDescriptorSetLayout is failed in createDescriptorSetLayout");
	}
}

void VulkanBase::createDescriptorSet()
{
	VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
	descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocInfo.pNext = nullptr;
	descriptorSetAllocInfo.descriptorPool = descriptorPool;
	descriptorSetAllocInfo.descriptorSetCount = 1u;
	descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
	if (vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &descriptorSet) != VK_SUCCESS)
	{
		errors.push_back("vkAllocateDescriptorSets is failled in createDescriptorSet");
	}
	//作成したデスクリプタセットはデスクリプタセットレイアウトが同じなら使いまわせる。
}

void VulkanBase::updateDescriptorSet()
{
	//更新するデスクリプタの内容
	VkDescriptorBufferInfo descriptorBufferInfo{};
	descriptorBufferInfo.buffer = deviceLocalBuffer;
	descriptorBufferInfo.offset = 0u;
	descriptorBufferInfo.range = 1024u;

	//デスクリプタの内容を更新
	VkWriteDescriptorSet writeDescriptorSet{};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.pNext = nullptr;
	writeDescriptorSet.dstSet = descriptorSet; //このデスクリプタセットの
	writeDescriptorSet.dstBinding = 0u; //binding=0の
	writeDescriptorSet.dstArrayElement = 0u; //0番目の
	writeDescriptorSet.descriptorCount = 1u; //1個の
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; //ストレージバッファのデスクリプタを
	writeDescriptorSet.pImageInfo = nullptr;
	writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;
	writeDescriptorSet.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(device, 1u, &writeDescriptorSet, 0u, nullptr);
}

void VulkanBase::terminate()
{
	//ステージングバッファを破棄
	vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAllocation);
}