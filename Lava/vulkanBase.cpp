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
	createStagingBuffer();
	createCommandPool();
	createCommandBuffer();
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
	//GPU�ɔ�����Ă���L���[���𒲂ׂ�B
	{   
		uint32_t queuePropCount;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queuePropCount, nullptr);
		queueProps.resize(queuePropCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queuePropCount, queueProps.data());
	}

	queueFamilyIndex = 0u;
	//�z��̒�����]��GPU�L���[�t�@�~���[(�ėp�v�Z)��I���B
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

//CPU�����GPU����������郁�C���������BCPU����R�s�[���ꂽ�f�[�^��GPU��GPU��VRAM�ɃR�s�[����B
void VulkanBase::createStagingBuffer()
{
	//�A���P�[�^�����
	VmaAllocatorCreateInfo allocatorCI{};
	allocatorCI.instance = instance;
	allocatorCI.physicalDevice = physicalDevice;
	allocatorCI.device = device;
	if (vmaCreateAllocator(&allocatorCI, &allocator) != VK_SUCCESS)
	{
		errors.push_back("vmaCreateAllocator failled in createStagingBuffer");
	}
	//�X�e�[�W���O�o�b�t�@�����
	VmaAllocationCreateInfo stagingBufferAllocInfo{};
	stagingBufferAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; //CPU��GPU������郁�����ŁAGPU�������ɏ����ł�����́B
	VkBufferCreateInfo bufferCI;
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

//�R�}���h�o�b�t�@�����蓖�Ă邽�߂̃R�}���h�v�[�������B
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

/*void VulkanBase::createStagingBuffer()
{
	//�X�e�[�W���O�o�b�t�@
	VkBuffer hostBuffer;
	VkBufferCreateInfo hostBufferCI{};
	hostBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	hostBufferCI.pNext = nullptr;
	hostBufferCI.flags = 0;
	hostBufferCI.size = 1000;
	hostBufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT || VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	hostBufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	hostBufferCI.queueFamilyIndexCount = 0;
	hostBufferCI.pQueueFamilyIndices = nullptr;
	if (vkCreateBuffer(device, &hostBufferCI, nullptr, &hostBuffer) != VK_SUCCESS)
	{
		errors.push_back("Create stagingBuffer failed");
		errorLog();
	}

	//�o�b�t�@�ɕK�v�ȃ������v���𒲂ׂ�
	VkMemoryRequirements hostBufferMemoryReqs;
	vkGetBufferMemoryRequirements(device, hostBuffer, &hostBufferMemoryReqs);

	//GPU�Ŏg���郁�C���������[�v�����擾
	VkPhysicalDeviceMemoryProperties pMemoryProps;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &pMemoryProps);

	//GPU���A�N�Z�X����̂ɓK�����q�[�v�C���f�b�N�X(�f�o�C�X���[�J��������)��T���B
	uint32_t hostHeapIndex = 0u;
	for (uint32_t i = 0; i < pMemoryProps.memoryHeapCount; i++)
	{
		if (pMemoryProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
		{
			hostHeapIndex = i;
			break;
		}
	}

	//��̃q�[�v�C���f�b�N�X���X�e�[�W���O�o�b�t�@�Ƃ��Ďg���邩�m�F����
	uint32_t hostMemoryIndex = 0u;
	for (uint32_t i = 0; i < pMemoryProps.memoryTypeCount; i++)
	{
		if (pMemoryProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  //���̐U�镑�������郁������
			&& (hostBufferMemoryReqs.memoryTypeBits >> i) & 0x1 //i�Ԗڂ̃������ɒu����Ƃ�
			&& pMemoryProps.memoryTypes[i].heapIndex == hostHeapIndex) //�X�e�[�W���O�o�b�t�@�Ƃ��Ďg����Ƃ�
		{
			hostMemoryIndex = i;
			break;
		}
	}

	//�X�e�[�W���O�o�b�t�@������
	VkMemoryAllocateInfo hostMemoryAI{};
	hostMemoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	hostMemoryAI.pNext = nullptr;
	hostMemoryAI.allocationSize = hostBufferMemoryReqs.size;
	hostMemoryAI.memoryTypeIndex = hostMemoryIndex;
	VkDeviceMemory hostMemory;
	if (vkAllocateMemory(device, &hostMemoryAI, nullptr, &hostMemory) != VK_SUCCESS)
	{
		errors.push_back("vkAllocateMemory for hostMemory is failled");
		errorLog();
	}

	//���������X�e�[�W���O�o�b�t�@�p�̃������Ɍ��т���
	if (vkBindBufferMemory(device, hostBuffer, hostMemory, 0u) != VK_SUCCESS)
	{
		errors.push_back("vkBindingMemory for hostMemory and hostBuffer is failled");
		errorLog();
	}
}*/

void VulkanBase::terminate()
{
	//�X�e�[�W���O�o�b�t�@��j��
	vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAllocation);
}