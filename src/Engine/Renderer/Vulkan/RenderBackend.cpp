

#include "Engine/Renderer/Vulkan/RenderBackend.h"



#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"



#ifdef NDEBUG
static const bool validation_layers_enabled = false;
#else
static const bool validation_layers_enabled = true;
#endif


static const std::vector<const char*> g_validation_layers = { "VK_LAYER_KHRONOS_validation" };

static const std::vector<const char*> g_device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };



/*
=======================================
Validation layer utility functions
=======================================
*/

static bool checkValidationLayerSupport()
{
	//get list of instance layer properties
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

	std::vector<VkLayerProperties> availible_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, availible_layers.data());

	//Check that validation layers required are supported by instance
	for (const char* layer_name : g_validation_layers) {
		bool layer_found = false;

		for (const auto& layer_properties : availible_layers) {
			if (std::strcmp(layer_name, layer_properties.layerName) == 0)
			{
				layer_found = true;
				break;
			}
		}

		if (!layer_found) {
			return false;
		}
	}

	return true;
}


std::vector<const char*> getRequiredExtensions() 
{
	uint32_t glfw_extension_count = 0;
	const char** glfw_extensions;
	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

	if (validation_layers_enabled) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallBack(
	VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	VkDebugUtilsMessageTypeFlagsEXT message_type,
	const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
	void* p_userdata) {

	if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) { 
		std::cerr << "Validation layer: " << p_callback_data->pMessage << std::endl;
	}
	
	return VK_FALSE;

}

static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info) 
{
	create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = debugCallBack;
	create_info.pUserData = nullptr; //could pass a pointer to something here. leaving it empty for now
}

//Create and destroy function for vulkan debug messenger (validation layers)
static VkResult createDebugUtilsMessengerExt(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
	const VkAllocationCallbacks* p_allocator, VkDebugUtilsMessengerEXT* p_debug_messenger) {

	auto funct = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (funct != nullptr) {
		return funct(instance, p_create_info, p_allocator, p_debug_messenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

static void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks* p_allocator) {
		auto funct = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (funct != nullptr) {
			funct(instance, debug_messenger, p_allocator);
		}
}


/*
=======================================
Physical device utility functions
=======================================
*/
static bool checkPhysDeviceExtensionSupport(GPUinfo_t& gpu, std::vector<const char*>& device_extensions){

	//populate set with device extensions
	std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

	//remove avaiilible extensions from set
	for (const auto& extension : gpu.extension_properties) {
		required_extensions.erase(extension.extensionName);
	}

	return required_extensions.empty();
}




/*
=======================================
Swapchain creation utility functions
=======================================
*/

/*
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {

	for (const auto& format : formats) {
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return format;
		}
	}

	//if best not found return the first one in list
	return formats[0];

}



VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& present_modes) {

	for (const auto& present_mode : present_modes) {
		if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return present_mode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}
*/


VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow *window) {

	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	} else {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

		extent.width = std::min<uint32_t>(extent.width, capabilities.maxImageExtent.width);
		extent.width = std::max<uint32_t>(extent.width, capabilities.minImageExtent.width);

		extent.height = std::min<uint32_t>(extent.height, capabilities.maxImageExtent.height);
		extent.height = std::max<uint32_t>(extent.height, capabilities.minImageExtent.height);

		return extent;
	}

}

/*
================================
RenderBackend vulkan boilerplate implementation
================================
*/



void RenderBackend::init(GLFWwindow* window)
{

	m_glfw_window = window;
	//Initialize vulkan context (instance devices queues)
	initContext_VK();
	
}




void RenderBackend::initContext_VK()
{

	//Create vulkan intance
	createInstance();

	//Create validaiton layer debug messenger
	setupDebugMessenger();

	//Create surface
	createSurface();

	//Pick best physical device (and queue families)
	choosePhysicalDevice();

	//Create logical device and queues
	createDeviceAndQueues();

	//Create vma allocator
	createVmaAllocator();

	//Initialize swapchain
	createSwapchainAndImages();

	//Create command pool and command buffers
	createCommandPoolAndBuffers();

	//Create the default renderpass
	createDefaultRenderPass();

	//Create the framebufffers
	createFramebuffers();

	//Create synchronisation structures
	createUploadSemaphoresAndFences();

	//Create descriptor set pool allocator
	createDescriptorAllocator();

	m_isInitialized = true;
}





void RenderBackend::createInstance() 
{
	if (validation_layers_enabled && !checkValidationLayerSupport()) {
		CRITICAL_ERROR_LOG("Validation layers requested but not availible");
	}
	
	//Application info
	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "VKProject";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "No Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;



	//Instance create info
	VkInstanceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;



	for (int i = 0; i < g_device_extensions.size(); ++i) {
		m_device_extensions.push_back(g_device_extensions[i]);
	}

	for (int i = 0; i < g_validation_layers.size(); ++i) {
		m_validation_layers.push_back(g_validation_layers[i]);
	}

	//validation layers
	VkDebugUtilsMessengerCreateInfoEXT debugmsg_create_info = {}; //extra debug messenger used to debug vkCreateInstance and vkDestroyInstance
	if (validation_layers_enabled) {
		create_info.enabledLayerCount = static_cast<uint32_t>(m_validation_layers.size());
		create_info.ppEnabledLayerNames = m_validation_layers.data();

		populateDebugMessengerCreateInfo(debugmsg_create_info);
		create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugmsg_create_info;

	} else {
		create_info.enabledLayerCount = 0;
	}

	auto extensions = getRequiredExtensions();
	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	create_info.ppEnabledExtensionNames = extensions.data();

	//Create instance
	VK_CHECK(vkCreateInstance(&create_info, nullptr, &m_instance));
	CONSOLE_LOG("Vulkan instance created");

}





void RenderBackend::setupDebugMessenger()
{
	if(!validation_layers_enabled) return;

	VkDebugUtilsMessengerCreateInfoEXT create_info;
	populateDebugMessengerCreateInfo(create_info);

	VK_CHECK(createDebugUtilsMessengerExt(m_instance, &create_info, nullptr, &m_debug_messenger));
}



void RenderBackend::createSurface() 
{
	VkWin32SurfaceCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	create_info.hwnd = glfwGetWin32Window(m_glfw_window);
	create_info.hinstance = GetModuleHandle(nullptr);

	VK_CHECK(vkCreateWin32SurfaceKHR(m_instance, &create_info, nullptr, &m_surface));

}



void RenderBackend::choosePhysicalDevice()
{

	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);

	if (device_count == 0) {
		CRITICAL_ERROR_LOG("No GPUs found with Vulkan support");
	}

	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());


	std::vector<GPUinfo_t> gpus_list(device_count);

	//Fill gpu infos
	for (uint32_t i = 0; i < device_count; ++i) {
		GPUinfo_t &gpu = gpus_list[i];
		gpu.device = devices[i];

		//Queue family properties
		{
			uint32_t queue_count = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(gpu.device, &queue_count, nullptr);

			gpu.queue_family_properties.resize(queue_count);
			vkGetPhysicalDeviceQueueFamilyProperties(gpu.device, &queue_count, gpu.queue_family_properties.data());
		}

		//Extension properties
		{
			uint32_t extension_count = 0;
			vkEnumerateDeviceExtensionProperties(gpu.device, nullptr, &extension_count, nullptr);

			gpu.extension_properties.resize(extension_count);
			vkEnumerateDeviceExtensionProperties(gpu.device, nullptr, &extension_count, gpu.extension_properties.data());
		}

		//Surface capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu.device, m_surface, &gpu.surface_capabilities);

		//Surface formats
		{
			uint32_t format_count = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR(gpu.device, m_surface, &format_count, nullptr);

			gpu.surface_formats.resize(format_count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(gpu.device, m_surface, &format_count, gpu.surface_formats.data());
		}

		//Present modes
		{
			uint32_t present_mode_count = 0;
			vkGetPhysicalDeviceSurfacePresentModesKHR(gpu.device, m_surface, &present_mode_count, nullptr);

			gpu.present_modes.resize(present_mode_count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(gpu.device, m_surface, &present_mode_count, gpu.present_modes.data());
		}
		
		//Memory properties
		vkGetPhysicalDeviceMemoryProperties(gpu.device, &gpu.memory_properties);

		//Device properties
		vkGetPhysicalDeviceProperties(gpu.device, &gpu.properties);

		//Device features
		vkGetPhysicalDeviceFeatures(gpu.device, &gpu.features);
	}

	//Choose best gpu
	for (uint32_t i = 0; i < device_count; ++i) {
		
		GPUinfo_t &gpu = gpus_list[i];

		int graphics_idx = -1;
		int present_idx = -1;


		if (!checkPhysDeviceExtensionSupport(gpu, m_device_extensions)) {
			continue;
		}

		if (!gpu.features.geometryShader) {
			continue;
		}

		if (gpu.properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && device_count > 1) {
			continue;
		}

		if (gpu.surface_formats.size() == 0) {
			continue;
		}

		if (gpu.present_modes.size() == 0) {
			continue;
		}




		//Find graphics queue family
		for (int j = 0; j < gpu.queue_family_properties.size(); ++j) {
			VkQueueFamilyProperties &properties = gpu.queue_family_properties[j];

			if (properties.queueCount == 0) {
				continue;
			}

			if (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				graphics_idx = j;
				break;
			}

		}

		//Find present queue family
		for (int j = 0; j < gpu.queue_family_properties.size(); ++j) {
			VkQueueFamilyProperties& properties = gpu.queue_family_properties[j];

			if (properties.queueCount == 0) {
				continue;
			}

			VkBool32 supports_present = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(gpu.device, j, m_surface, &supports_present);
			if (supports_present) {
				present_idx = j;
				break;
			}

		}


		//Is gpu good for both graphics and present?
		if (graphics_idx >= 0 && present_idx >= 0) {
			m_graphics_family_idx = graphics_idx;
			m_present_family_idx = present_idx;
			m_gpu_info = gpu;

			return;
		}

	}

	CRITICAL_ERROR_LOG("No suitable GPU was found");
		
}



void RenderBackend::createDeviceAndQueues()
{

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<uint32_t> unique_queue_families;

	unique_queue_families.insert(m_graphics_family_idx);
	unique_queue_families.insert(m_present_family_idx);

	float queue_priority = 1.0f;
	for (uint32_t queue_family : unique_queue_families) {
		VkDeviceQueueCreateInfo queue_create_info = {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queue_family;
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;

		queue_create_infos.push_back(queue_create_info);
	}

	//Could select more VkPhysicalDeviceFeatures here, for now all false
	VkPhysicalDeviceFeatures device_features = {};


	VkDeviceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.pEnabledFeatures = &device_features;

	create_info.enabledExtensionCount = static_cast<uint32_t>(m_device_extensions.size());
	create_info.ppEnabledExtensionNames = m_device_extensions.data();

	if (validation_layers_enabled) {
		create_info.enabledLayerCount = static_cast<uint32_t>(m_validation_layers.size());
		create_info.ppEnabledLayerNames = m_validation_layers.data();
	}
	else {
		create_info.enabledLayerCount = 0;
	}

	
	VK_CHECK(vkCreateDevice(m_gpu_info.device, &create_info, nullptr, &m_device));

	vkGetDeviceQueue(m_device, m_graphics_family_idx, 0, &m_graphics_queue);
	vkGetDeviceQueue(m_device, m_present_family_idx, 0, &m_present_queue);
}




void RenderBackend::createVmaAllocator()
{
	VmaAllocatorCreateInfo allocator_info = {};
	allocator_info.physicalDevice = m_gpu_info.device;
	allocator_info.device = m_device;
	allocator_info.instance = m_instance;
	vmaCreateAllocator(&allocator_info, &m_allocator);
}




void RenderBackend::createSwapchainAndImages()
{

	VkExtent2D extent = chooseSwapExtent(m_gpu_info.surface_capabilities, m_glfw_window);

	m_swapchain = std::make_unique<Swapchain>(extent);

	ImageProperties depth_img_properties{ {extent.width, extent.height}, {VK_FORMAT_D32_SFLOAT} };
	m_swapchain_depth_image = {depth_img_properties};

}


void RenderBackend::createCommandPoolAndBuffers() {


	
	for (int i = 0; i < FRAME_OVERLAP_COUNT; i++) {

		/*
		VkCommandBufferAllocateInfo buffer_create_info = {};
		buffer_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		buffer_create_info.pNext = nullptr;
		buffer_create_info.commandPool = command_pool;
		buffer_create_info.commandBufferCount = 1;
		buffer_create_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		VK_CHECK(vkAllocateCommandBuffers(m_device, &buffer_create_info, &m_frame_data[i].m_command_buffer));

		//m_deletion_queue.pushFunction([=]() { vkDestroyCommandPool(m_device, m_frame_data[i].m_command_pool, nullptr); });*/
		
		CommandPool* command_pool = m_command_pools.emplace_back(std::make_shared<CommandPool>()).get();
		m_command_buffers[i] = command_pool->allocateCommandBuffer();
	}
	

	//Create command pool for upload context (staging buffers etc)
	m_upload_context.m_command_pool = std::make_shared<CommandPool>();

}



void RenderBackend::createDefaultRenderPass()
{

	ImageProperties color_props = m_swapchain.get()->images()[0].properties();//{ ImageSize{m_swapchain_extent.width,m_swapchain_extent.height}, ImageFormat{m_swapchain_format} };
	ImageProperties depth_props = m_swapchain_depth_image.properties();//{ ImageSize{m_swapchain_extent.width,m_swapchain_extent.height}, ImageFormat{VK_FORMAT_D32_SFLOAT} };


	std::vector<RenderPass::AttachmentProperties> main_color_attachments;
	main_color_attachments.push_back({ color_props, RenderPass::LoadOp::Clear, ImageUsage::SwapchainImage | ImageUsage::ColorAttachment });

	RenderPass::AttachmentProperties main_depth_attachment{ depth_props, RenderPass::LoadOp::Clear, ImageUsage::SwapchainImage | ImageUsage::DepthAttachment };
	m_render_pass = RenderPass{ main_color_attachments, main_depth_attachment };
}



void RenderBackend::createFramebuffers() 
{

	//create a framebuffer for each swapchain image
	const uint32_t swapchain_image_count = m_swapchain.get()->imageCount();
	for (int i = 0; i < swapchain_image_count; i++) 
	{
		std::vector<ImageBase> color_images = { m_swapchain.get()->images()[i] };
		m_framebuffers.push_back({color_images, m_swapchain_depth_image, &m_render_pass});
	}

}


void RenderBackend::createUploadSemaphoresAndFences() 
{
	
	//create fence for upload context (staging buffers etc)
	VkFenceCreateInfo upload_fence_create_info = {};
	upload_fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	upload_fence_create_info.pNext = nullptr;
	upload_fence_create_info.flags = 0;

	VK_CHECK(vkCreateFence(m_device, &upload_fence_create_info, nullptr, &m_upload_context.m_fence));
	m_deletion_queue.pushFunction([=]() { vkDestroyFence(m_device, m_upload_context.m_fence, nullptr); });

}



void RenderBackend::createDescriptorAllocator()
{

	m_descriptor_allocator = std::make_unique<DescriptorSetAllocator>();

}



void RenderBackend::pushToDeletionQueue(std::function<void()> function) {

	m_deletion_queue.pushFunction(function);

}

void RenderBackend::pushToSwapchainDeletionQueue(std::function<void()> function) {

	m_swapchain_deletion_queue.pushFunction(function);

}


void RenderBackend::shutdown() {
	
	if(m_isInitialized){
		
		--m_frame_count;
		vkWaitForFences(m_device, 1, &getCurrentFrameCmdBuffer().fence(), true, 1000000000);
		++m_frame_count;

		m_descriptor_allocator->cleanup();

		m_swapchain_deletion_queue.executeDeletionQueue(); //maybe add the rest to the deletion queue

		m_deletion_queue.executeDeletionQueue(); //maybe add the rest to the deletion queue


		vmaDestroyAllocator(m_allocator);

		vkDestroyDevice(m_device, nullptr);
	
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

		if (validation_layers_enabled) {
			destroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);
		}

		vkDestroyInstance(m_instance, nullptr);
	}
}




/*
=====================================
RenderBackend ImGUI initialization
=====================================
*/
void RenderBackend::initImGUI()
{

	if (!m_isInitialized)
	{
		CRITICAL_ERROR_LOG("IMGUI init failed: vulkan not properly intialized");
	}
	
	//1: create descriptor pool for IMGUI. from imgui demo
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool imguiPool;
	VK_CHECK(vkCreateDescriptorPool(m_device, &pool_info, nullptr, &imguiPool));


	// 2: initialize imgui library

	//this initializes the core structures of imgui
	ImGui::CreateContext();

	//this initializes imgui for SDL
	ImGui_ImplGlfw_InitForVulkan(m_glfw_window, true);

	//this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = m_instance;
	init_info.PhysicalDevice = m_gpu_info.device;
	init_info.Device = m_device;
	init_info.Queue = m_graphics_queue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 2; //CHECK THIS LATER DEFAULT VALUE FROM TUTORIAL
	init_info.ImageCount = 2; //CHECK THIS LATER DEFAULT VALUE FROM TUTORIAL
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	


	ImGui_ImplVulkan_Init(&init_info, m_render_pass.vk_renderpass());

	//execute a gpu command to upload imgui font textures
	CommandBuffer cmd_buffer = createDisposableCmdBuffer();
	ImGui_ImplVulkan_CreateFontsTexture(cmd_buffer.vk_commandBuffer());
	cmd_buffer.immediateSubmit();

	//immediateSubmit([&](VkCommandBuffer cmd) {
	//	ImGui_ImplVulkan_CreateFontsTexture(cmd);
	//	});

	//clear font textures from cpu data
	ImGui_ImplVulkan_DestroyFontUploadObjects();

	//add the destroy the imgui created structures
	m_deletion_queue.pushFunction([=]() {
			vkDestroyDescriptorPool(m_device, imguiPool, nullptr);
			ImGui_ImplVulkan_Shutdown();
		});

		
}


/*
============================================
RenderBackend create disposable cmd buffer
============================================
*/

CommandBuffer& RenderBackend::createDisposableCmdBuffer()
{
	return m_upload_context.m_command_pool.get()->allocateCommandBuffer(true);
}

/*
=====================================
RenderBackend Immediate Submit
=====================================
*/

void RenderBackend::immediateSubmit(std::function<void(VkCommandBuffer cmd)> function)
{
	//Allocate command buffer
	VkCommandBufferAllocateInfo cmd_allocate_info = {};
	cmd_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd_allocate_info.commandBufferCount = 1;
	cmd_allocate_info.commandPool = m_upload_context.m_command_pool.get()->commandPool();
	cmd_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd_allocate_info.pNext = nullptr;
		
	VkCommandBuffer cmd;
	VK_CHECK(vkAllocateCommandBuffers(m_device, &cmd_allocate_info, &cmd));

	//Begin commadn buffer
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = nullptr;
	begin_info.pInheritanceInfo = nullptr;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));
	

	//execute function
	function(cmd);


	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = nullptr;
	submit_info.waitSemaphoreCount = 0;
	submit_info.pWaitSemaphores = nullptr;
	submit_info.pWaitDstStageMask = nullptr;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd;
	submit_info.signalSemaphoreCount = 0;
	submit_info.pSignalSemaphores = nullptr;

	//submit command buffer to queue and execute it.
	//fence will block until the commands finish
	VK_CHECK(vkQueueSubmit(m_graphics_queue, 1, &submit_info, m_upload_context.m_fence));

	vkWaitForFences(m_device, 1, &m_upload_context.m_fence, true, 9999999999);

	vkResetFences(m_device, 1, &m_upload_context.m_fence);

	//clear the command pool
	m_upload_context.m_command_pool.get()->resetPool();

	CONSOLE_LOG("IMMEDIATE SUBMIT: Data sumbitted to GPU");
}



/*
=====================================
RenderBackend Render commands
=====================================
*/



void RenderBackend::RC_beginFrame()
{

	//Wait for GPU to finish last frame
	VK_CHECK( vkWaitForFences(m_device, 1, &getCurrentFrameCmdBuffer().fence(), true, 1000000000)); //1 second timeout 1000000000ns
	VK_CHECK( vkResetFences(m_device, 1, &getCurrentFrameCmdBuffer().fence() ));

	//request next image from swapchain, 1 second timeout
	VK_CHECK(vkAcquireNextImageKHR(m_device, m_swapchain.get()->swapchainKHR(), 1000000000, m_swapchain.get()->currentSyncStructures(m_frame_count).m_present_semaphore, 
			nullptr, &m_swapchain.get()->currentImageIndex()));

	//Reset command buffer (past fence so we know commands finished executing)
	VK_CHECK(vkResetCommandBuffer(getCurrentFrameCmdBuffer().vk_commandBuffer(), 0));


	VkCommandBuffer cmd_buffer = getCurrentFrameCmdBuffer().vk_commandBuffer();

	VkCommandBufferBeginInfo cmd_buffer_begin_info = {};
	cmd_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmd_buffer_begin_info.pNext = nullptr;
	cmd_buffer_begin_info.pInheritanceInfo = nullptr;
	cmd_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; //Set it to VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT if command buffer is being reset i think

	VK_CHECK(vkBeginCommandBuffer(cmd_buffer, &cmd_buffer_begin_info));



	//begin main renderpass //TODO: Rework BeginRenderPass and the whole beginFrame function in general to be more modular

	VkRenderPassBeginInfo render_pass_begin_info = {};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.pNext = nullptr;

	render_pass_begin_info.renderPass = m_render_pass.vk_renderpass();
	render_pass_begin_info.renderArea.offset.x = 0;
	render_pass_begin_info.renderArea.offset.y = 0;
	render_pass_begin_info.renderArea.extent = m_swapchain.get()->extent();
	render_pass_begin_info.framebuffer = m_framebuffers[m_swapchain.get()->currentImageIndex()].framebuffer();

	VkClearValue color_clear;
	//color_clear.color = { {0.1f, 0.2f, 0.4f, 1.0f} };//sky blue
	color_clear.color = { {0.005f, 0.005f, 0.005f, 1.0f} };
	VkClearValue depth_clear;
	depth_clear.depthStencil.depth = 1.0f; //max depth

	VkClearValue clear_values[2] = { color_clear, depth_clear };
	render_pass_begin_info.clearValueCount = 2;
	render_pass_begin_info.pClearValues = &clear_values[0];


	vkCmdBeginRenderPass(cmd_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}


void RenderBackend::RC_endFrame() 
{

	VkCommandBuffer cmd_buffer = getCurrentFrameCmdBuffer().vk_commandBuffer();


	vkCmdEndRenderPass(cmd_buffer);
	VK_CHECK(vkEndCommandBuffer(cmd_buffer));


	//Prepare queue submission
	

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = nullptr;

	VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	submit_info.pWaitDstStageMask = &wait_stage;

	//Wait on the m_present_semaphore, that semaphore is signallled when swapchain ready
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &m_swapchain.get()->currentSyncStructures(m_frame_count).m_present_semaphore;

	//Signal the m_render_semaphore, to signal that rendering has finished
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &m_swapchain.get()->currentSyncStructures(m_frame_count).m_render_semaphore;

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd_buffer;

	//Sumbit command buffer to queue and execute. m_render_fence will block until commands finish
	VK_CHECK(vkQueueSubmit(m_graphics_queue, 1, &submit_info, getCurrentFrameCmdBuffer().fence()));


	//Put image on visible window. Must wait on m_render_semaphore to ensure that drawing commands have finished
	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext = nullptr;

	VkSwapchainKHR swapchain = m_swapchain.get()->swapchainKHR();
	present_info.pSwapchains = &swapchain;
	present_info.swapchainCount = 1;

	present_info.pWaitSemaphores = &m_swapchain.get()->currentSyncStructures(m_frame_count).m_render_semaphore;
	present_info.waitSemaphoreCount = 1;

	present_info.pImageIndices = &m_swapchain.get()->currentImageIndex();

	VK_CHECK(vkQueuePresentKHR(m_graphics_queue, &present_info));

	//Increment frame counter
	m_frame_count++;
}


void RenderBackend::RC_bindGraphicsPipeline(const std::shared_ptr<GraphicsPipeline> pipeline)
{
	VkCommandBuffer cmd_buffer = getCurrentFrameCmdBuffer().vk_commandBuffer();

	VkPipeline vulkan_pipeline = pipeline->pipeline();
	vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan_pipeline);

}


void RenderBackend::RC_pushConstants(const std::shared_ptr<GraphicsPipeline> pipeline, const MatrixPushConstant push_constant)
{
	VkCommandBuffer cmd_buffer = getCurrentFrameCmdBuffer().vk_commandBuffer();

	VkPipelineLayout vulkan_pipeline_layout = pipeline->pipelineLayout();

	vkCmdPushConstants(cmd_buffer, vulkan_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MatrixPushConstant), &push_constant);


}


void RenderBackend::RC_bindVertexBuffer(const std::shared_ptr<VertexBuffer> vertex_buffer)
{
	VkCommandBuffer cmd_buffer = getCurrentFrameCmdBuffer().vk_commandBuffer();

	VkBuffer buffer = (vertex_buffer->getBuffer());

	VkDeviceSize offset = 0;

	vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &buffer, &offset);
}


void RenderBackend::RC_bindIndexBuffer(const std::shared_ptr<IndexBuffer> index_buffer)
{
	VkCommandBuffer cmd_buffer = getCurrentFrameCmdBuffer().vk_commandBuffer();

	VkBuffer buffer = (index_buffer->getBuffer());


	VkDeviceSize offset = 0;
	vkCmdBindIndexBuffer(cmd_buffer, buffer, 0, VK_INDEX_TYPE_UINT16);
}


void RenderBackend::RC_drawSumbit(uint32_t size)
{
	VkCommandBuffer cmd_buffer = getCurrentFrameCmdBuffer().vk_commandBuffer();
	vkCmdDraw(cmd_buffer, size, 1, 0, 0);
}


void RenderBackend::RC_drawIndexed(uint32_t size)
{
	VkCommandBuffer cmd_buffer = getCurrentFrameCmdBuffer().vk_commandBuffer();

	vkCmdDrawIndexed(cmd_buffer, size, 1, 0, 0, 0);

}



void RenderBackend::RC_bindDescriptorSet(const std::shared_ptr<GraphicsPipeline> pipeline, const DescriptorSet& descriptor_set, uint32_t offset_count, uint32_t* p_dynamic_offset)
{

	VkCommandBuffer cmd_buffer = getCurrentFrameCmdBuffer().vk_commandBuffer();

	VkPipelineLayout vulkan_pipeline_layout = pipeline->pipelineLayout();

	vkCmdBindDescriptorSets(
		cmd_buffer, 
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		vulkan_pipeline_layout, 
		descriptor_set.setNumber(), 
		1, 
		&descriptor_set.descriptorSet(), 
		offset_count, 
		p_dynamic_offset);

}