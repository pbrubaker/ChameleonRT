#include <array>
#include <iostream>
#include <vector>
#include <stdexcept>
#include "vulkan_utils.h"

PFN_vkCreateAccelerationStructureNV vkCreateAccelerationStructure = nullptr;
PFN_vkDestroyAccelerationStructureNV vkDestroyAccelerationStructure = nullptr;
PFN_vkBindAccelerationStructureMemoryNV vkBindAccelerationStructureMemory = nullptr;
PFN_vkGetAccelerationStructureHandleNV vkGetAccelerationStructureHandle = nullptr;
PFN_vkGetAccelerationStructureMemoryRequirementsNV vkGetAccelerationStructureMemoryRequirements = nullptr;
PFN_vkCmdBuildAccelerationStructureNV vkCmdBuildAccelerationStructure = nullptr;
PFN_vkCreateRayTracingPipelinesNV vkCreateRayTracingPipelines = nullptr;
PFN_vkGetRayTracingShaderGroupHandlesNV vkGetRayTracingShaderGroupHandles = nullptr;
PFN_vkCmdTraceRaysNV vkCmdTraceRays = nullptr;

namespace vk {

void load_nv_rtx(VkDevice &device) {
	vkCreateAccelerationStructure =
		reinterpret_cast<PFN_vkCreateAccelerationStructureNV>(vkGetDeviceProcAddr(device,
					"vkCreateAccelerationStructureNV"));
	vkDestroyAccelerationStructure =
		reinterpret_cast<PFN_vkDestroyAccelerationStructureNV>(vkGetDeviceProcAddr(device,
					"vkDestroyAccelerationStructureNV"));
	vkBindAccelerationStructureMemory =
		reinterpret_cast<PFN_vkBindAccelerationStructureMemoryNV>(vkGetDeviceProcAddr(device,
					"vkBindAccelerationStructureMemoryNV"));
	vkGetAccelerationStructureHandle =
		reinterpret_cast<PFN_vkGetAccelerationStructureHandleNV>(vkGetDeviceProcAddr(device,
					"vkGetAccelerationStructureHandleNV"));
	vkGetAccelerationStructureMemoryRequirements =
		reinterpret_cast<PFN_vkGetAccelerationStructureMemoryRequirementsNV>(vkGetDeviceProcAddr(device,
					"vkGetAccelerationStructureMemoryRequirementsNV"));
	vkCmdBuildAccelerationStructure =
		reinterpret_cast<PFN_vkCmdBuildAccelerationStructureNV>(vkGetDeviceProcAddr(device,
					"vkCmdBuildAccelerationStructureNV"));
	vkCreateRayTracingPipelines =
		reinterpret_cast<PFN_vkCreateRayTracingPipelinesNV>(vkGetDeviceProcAddr(device,
					"vkCreateRayTracingPipelinesNV"));
	vkGetRayTracingShaderGroupHandles =
		reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesNV>(vkGetDeviceProcAddr(device,
					"vkGetRayTracingShaderGroupHandlesNV"));
	vkCmdTraceRays =
		reinterpret_cast<PFN_vkCmdTraceRaysNV>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysNV"));
}

static const std::array<const char*, 1> validation_layers = {
	"VK_LAYER_KHRONOS_validation"
};

Device::Device() {
	make_instance();
	select_physical_device();
	make_logical_device();

	load_nv_rtx(device);

	// Query the properties we'll use frequently
	vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);

	rt_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
	VkPhysicalDeviceProperties2 props = {};
	props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	props.pNext = &rt_props;
	props.properties = {};
	vkGetPhysicalDeviceProperties2(physical_device, &props);

	std::cout << "Raytracing props:\n"
		<< "max recursion depth: " << rt_props.maxRecursionDepth
		<< "\nSBT handle size: " << rt_props.shaderGroupHandleSize
		<< "\nShader group base align: " << rt_props.shaderGroupBaseAlignment << "\n";
}

Device::~Device() {
	if (instance != VK_NULL_HANDLE) { 
		vkDestroyDevice(device, nullptr);
		vkDestroyInstance(instance, nullptr);
	}
}

Device::Device(Device &&d)
	: instance(d.instance), physical_device(d.physical_device), device(d.device), queue(d.queue),
	mem_props(d.mem_props), rt_props(d.rt_props)
{
	d.instance = VK_NULL_HANDLE;
	d.physical_device = VK_NULL_HANDLE;
	d.device = VK_NULL_HANDLE;
	d.queue = VK_NULL_HANDLE;
}

Device& Device::operator=(Device &&d) {
	instance = d.instance;
	physical_device = d.physical_device;
	device = d.device;
	queue = d.queue;
	mem_props = d.mem_props;
	rt_props = d.rt_props;

	d.instance = VK_NULL_HANDLE;
	d.physical_device = VK_NULL_HANDLE;
	d.device = VK_NULL_HANDLE;
	d.queue = VK_NULL_HANDLE;

	return *this;
}

VkDevice Device::logical_device() {
	return device;
}

VkQueue Device::graphics_queue() {
	return queue;
}

VkCommandPool Device::make_command_pool(VkCommandPoolCreateFlagBits flags) {
	VkCommandPool pool = VK_NULL_HANDLE;
	VkCommandPoolCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.flags = flags;
	create_info.queueFamilyIndex = graphics_queue_index;
	CHECK_VULKAN(vkCreateCommandPool(device, &create_info, nullptr, &pool));
	return pool;
}

uint32_t Device::memory_type_index(uint32_t type_filter, VkMemoryPropertyFlags props) const {
	for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
		if (type_filter & (1 << i) && (mem_props.memoryTypes[i].propertyFlags & props) == props) {
			return i;
		}
	}
	throw std::runtime_error("failed to find appropriate memory");
}

const VkPhysicalDeviceMemoryProperties& Device::memory_properties() const {
	return mem_props;
}

const VkPhysicalDeviceRayTracingPropertiesNV& Device::raytracing_properties() const {
	return rt_props;
}

void Device::make_instance() {
	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "rtobj";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "None";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;
	create_info.enabledExtensionCount = 0;
	create_info.ppEnabledExtensionNames = nullptr;
	create_info.enabledLayerCount = validation_layers.size();
	create_info.ppEnabledLayerNames = validation_layers.data();

	CHECK_VULKAN(vkCreateInstance(&create_info, nullptr, &instance));
}

void Device::select_physical_device() {
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
	std::cout << "Found " << device_count << " devices\n";
	std::vector<VkPhysicalDevice> devices(device_count, VkPhysicalDevice{});
	vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

	for (const auto &d : devices) {
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceProperties(d, &properties);
		vkGetPhysicalDeviceFeatures(d, &features);
		std::cout << properties.deviceName << "\n";

		uint32_t extension_count = 0;
		vkEnumerateDeviceExtensionProperties(d, nullptr, &extension_count, nullptr);
		std::cout << "num extensions: " << extension_count << "\n";
		std::vector<VkExtensionProperties> extensions(extension_count, VkExtensionProperties{});
		vkEnumerateDeviceExtensionProperties(d, nullptr, &extension_count, extensions.data());
		std::cout << "Device available extensions:\n";
		for (const auto& e : extensions) {
			std::cout << e.extensionName << "\n";
		}

		// Check for RTX support on this device
		auto fnd = std::find_if(extensions.begin(), extensions.end(),
				[](const VkExtensionProperties &e) {
					return std::strcmp(e.extensionName, VK_NV_RAY_TRACING_EXTENSION_NAME) == 0;
				});

		if (fnd != extensions.end()) {
			physical_device = d;
			break;
		}
	}

	if (physical_device == VK_NULL_HANDLE) {
		std::cout << "Failed to find RTX capable GPU\n";
		throw std::runtime_error("Failed to find RTX capable GPU");
	}
}

void Device::make_logical_device() {
	uint32_t num_queue_families = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, nullptr);
	std::vector<VkQueueFamilyProperties> family_props(num_queue_families, VkQueueFamilyProperties{});
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, family_props.data());
	for (uint32_t i = 0; i < num_queue_families; ++i) {
		if (family_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			graphics_queue_index = i;
			break;
		}
	}

	std::cout << "Graphics queue is " << graphics_queue_index << "\n";
	const float queue_priority = 1.f;

	VkDeviceQueueCreateInfo queue_create_info = {};
	queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.queueFamilyIndex = graphics_queue_index;
	queue_create_info.queueCount = 1;
	queue_create_info.pQueuePriorities = &queue_priority;

	VkPhysicalDeviceFeatures device_features = {};

	const std::array<const char*, 2> device_extensions = {
		VK_NV_RAY_TRACING_EXTENSION_NAME,
		VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
	};

	VkDeviceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.queueCreateInfoCount = 1;
	create_info.pQueueCreateInfos = &queue_create_info;
	create_info.enabledLayerCount = validation_layers.size();
	create_info.ppEnabledLayerNames = validation_layers.data();
	create_info.enabledExtensionCount = device_extensions.size();
	create_info.ppEnabledExtensionNames = device_extensions.data();
	create_info.pEnabledFeatures = &device_features;
	CHECK_VULKAN(vkCreateDevice(physical_device, &create_info, nullptr, &device));
}

}
