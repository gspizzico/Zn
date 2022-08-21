#include <Znpch.h>
#define VMA_IMPLEMENTATION
#include <Rendering/Vulkan/VulkanDevice.h>
#include <SDL.h>
#include <SDL_vulkan.h>

#include <Core/Containers/Set.h>
#include <Core/IO/IO.h>

#include <Core/Memory/Memory.h>

#include <Rendering/Vulkan/VulkanPipeline.h>

#include <algorithm>

// ImGui

#include <imgui_impl_vulkan.h>
#include <imgui_impl_sdl.h>

#define ZN_VK_VALIDATION_LAYERS (ZN_DEBUG)

#define ZN_VK_VALIDATION_VERBOSE (0)

#define ZN_VK_CHECK(expression)\
if(expression != VK_SUCCESS)\
{\
_ASSERT(false);\
}

#define ZN_VK_CHECK_RETURN(expression)\
if(expression != VK_SUCCESS)\
{\
_ASSERT(false);\
return;\
}

DEFINE_STATIC_LOG_CATEGORY(LogVulkan, ELogVerbosity::Log);
// Verbosity set to ::Verbose, since messages are filtered through flags using ZN_VK_VALIDATION_VERBOSE
DEFINE_STATIC_LOG_CATEGORY(LogVulkanValidation, ELogVerbosity::Verbose);

using namespace Zn;

namespace
{
	ELogVerbosity VkMessageSeverityToZnVerbosity(VkDebugUtilsMessageSeverityFlagBitsEXT InSeverity)
	{
		switch (InSeverity)
		{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			return ELogVerbosity::Verbose;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			return ELogVerbosity::Log;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			return ELogVerbosity::Warning;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			default:
			return ELogVerbosity::Error;
		}
	}

	const String& VkMessageTypeToString(VkDebugUtilsMessageTypeFlagBitsEXT InType)
	{
		static const String kGeneral = "VkGeneral";
		static const String kValidation = "VkValidation";
		static const String kPerformance = "VkPerf";

		switch (InType)
		{
			case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
			return kGeneral;
			case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
			return kValidation;
			case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
			return kPerformance;
			default:
			return kGeneral;
		}
	}

	template<typename OutputType, typename VkFunction, typename ...Args>
	Vector<OutputType> VkEnumerate(VkFunction&& InFunction, Args&&... InArgs)
	{
		uint32 Count = 0;
		InFunction(std::forward<Args>(InArgs)..., &Count, nullptr);

		Vector<OutputType> Output(Count);
		InFunction(std::forward<Args>(InArgs)..., &Count, Output.data());

		return Output;
	}

	template<typename TypePtr, typename OwnerType, typename CreateInfoType, typename VkCreateFunction>
	VkResult VkCreate(OwnerType Owner, TypePtr& Object, const CreateInfoType& CreateInfo, VkCreateFunction&& InFunction)
	{
		return InFunction(Owner, &CreateInfo, nullptr, &Object);
	}

	template<typename TypePtr, typename OwnerType, typename VkDestroyFunction>
	void VkDestroy(TypePtr& Object, OwnerType Owner, VkDestroyFunction&& InFunction)
	{
		if (Object != VK_NULL_HANDLE)
		{
			InFunction(Owner, Object, nullptr);
			Object = VK_NULL_HANDLE;
		}
	}
}

const Vector<const char*> VulkanDevice::kValidationLayers = { "VK_LAYER_KHRONOS_validation" };
const Vector<const char*> VulkanDevice::kDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

VulkanDevice::VulkanDevice()
{

}

VulkanDevice::~VulkanDevice()
{
	Cleanup();
}

void VulkanDevice::Initialize(SDL_Window* InWindowHandle)
{
	/////// Create VkIntance

	VkApplicationInfo ApplicationInfo{};
	ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	ApplicationInfo.pApplicationName = "Zn";
	ApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	ApplicationInfo.pEngineName = "Zn";
	ApplicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	ApplicationInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	CreateInfo.pApplicationInfo = &ApplicationInfo;

	const Vector<const char*> RequiredExtensions = GetRequiredVkExtensions(InWindowHandle);

	CreateInfo.enabledExtensionCount = static_cast<uint32>(RequiredExtensions.size());
	CreateInfo.ppEnabledExtensionNames = RequiredExtensions.data();

#if ZN_VK_VALIDATION_LAYERS
	// Enable Validation Layers
	_ASSERT(SupportsValidationLayers());

	CreateInfo.enabledLayerCount = static_cast<uint32>(kValidationLayers.size());
	CreateInfo.ppEnabledLayerNames = kValidationLayers.data();

	VkDebugUtilsMessengerCreateInfoEXT DebugCreateInfo = GetDebugMessengerCreateInfo();

	CreateInfo.pNext = &DebugCreateInfo;
#else
	CreateInfo.enabledLayerCount = 0;
#endif


	ZN_VK_CHECK(vkCreateInstance(&CreateInfo, nullptr, &m_VkInstance));

#if ZN_VK_VALIDATION_LAYERS
	InitializeDebugMessenger();
#endif

	/////// Create Surface
	if (SDL_Vulkan_CreateSurface(InWindowHandle, m_VkInstance, &m_VkSurface) != SDL_TRUE)
	{
		_ASSERT(false);
		return;
	}

	/////// Initialize GPU

	Vector<VkPhysicalDevice> Devices = VkEnumerate<VkPhysicalDevice>(vkEnumeratePhysicalDevices, m_VkInstance);

	m_VkGPU = SelectPhysicalDevice(Devices);

	_ASSERT(m_VkGPU != VK_NULL_HANDLE);

	/////// Initialize Logical Device

	Vk::QueueFamilyIndices Indices = GetQueueFamilyIndices(m_VkGPU);

	Vk::SwapChainDetails SwapChainDetails = GetSwapChainDetails(m_VkGPU);

	const bool IsSupported = SwapChainDetails.Formats.size() > 0 && SwapChainDetails.PresentModes.size() > 0;

	if (!IsSupported)
	{
		_ASSERT(false);
		return;
	}

	Vector<VkDeviceQueueCreateInfo> QueueFamilies = BuildQueueCreateInfo(Indices);

	VkPhysicalDeviceFeatures DeviceFeatures{}; // TEMP

	VkDeviceCreateInfo LogicalDeviceCreateInfo{};
	LogicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	LogicalDeviceCreateInfo.pQueueCreateInfos = QueueFamilies.data();
	LogicalDeviceCreateInfo.queueCreateInfoCount = static_cast<uint32>(QueueFamilies.size());
	LogicalDeviceCreateInfo.pEnabledFeatures = &DeviceFeatures;

	LogicalDeviceCreateInfo.ppEnabledExtensionNames = kDeviceExtensions.data();
	LogicalDeviceCreateInfo.enabledExtensionCount = static_cast<uint32>(kDeviceExtensions.size());

	ZN_VK_CHECK(vkCreateDevice(m_VkGPU, &LogicalDeviceCreateInfo, nullptr, &m_VkDevice));

	vkGetDeviceQueue(m_VkDevice, Indices.Graphics.value(), 0, &m_VkGraphicsQueue);
	vkGetDeviceQueue(m_VkDevice, Indices.Present.value(), 0, &m_VkPresentQueue);

	//initialize the memory allocator
	VmaAllocatorCreateInfo AllocatorCreateInfo{};
	AllocatorCreateInfo.instance = m_VkInstance;
	AllocatorCreateInfo.physicalDevice = m_VkGPU;
	AllocatorCreateInfo.device = m_VkDevice;
	vmaCreateAllocator(&AllocatorCreateInfo, &m_VkAllocator);

	/////// Create Swap Chain

	VkSurfaceFormatKHR Format{ VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	for (const VkSurfaceFormatKHR& AvailableFormat : SwapChainDetails.Formats)
	{
		if (AvailableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && AvailableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			Format = AvailableFormat;
			break;
		}
	}

	if (Format.format == VK_FORMAT_UNDEFINED)
	{
		Format = SwapChainDetails.Formats[0];
	}

	// Always guaranteed to be available.
	VkPresentModeKHR PresentMode = VK_PRESENT_MODE_FIFO_KHR;

	const bool UseMailboxPresentMode = std::any_of(SwapChainDetails.PresentModes.begin(), SwapChainDetails.PresentModes.end(),
												   [](VkPresentModeKHR InPresentMode)
												   {
													   // 'Triple Buffering'
													   return InPresentMode == VK_PRESENT_MODE_MAILBOX_KHR;
												   });

	if (UseMailboxPresentMode)
	{
		PresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
	}

	int32 Width, Height = 0;
	SDL_Vulkan_GetDrawableSize(InWindowHandle, &Width, &Height);

	Width = std::clamp(Width, static_cast<int32>(SwapChainDetails.Capabilities.minImageExtent.width), static_cast<int32>(SwapChainDetails.Capabilities.maxImageExtent.width));
	Height = std::clamp(Height, static_cast<int32>(SwapChainDetails.Capabilities.minImageExtent.height), static_cast<int32>(SwapChainDetails.Capabilities.maxImageExtent.height));

	uint32 ImageCount = SwapChainDetails.Capabilities.minImageCount + 1;
	if (SwapChainDetails.Capabilities.maxImageCount > 0)
	{
		ImageCount = std::min(ImageCount, SwapChainDetails.Capabilities.maxImageCount);
	}

	m_VkSwapChainFormat = Format;

	m_VkSwapChainExtent = VkExtent2D(Width, Height);

	VkSwapchainCreateInfoKHR SwapChainCreateInfo{};
	SwapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	SwapChainCreateInfo.surface = m_VkSurface;
	SwapChainCreateInfo.minImageCount = ImageCount;
	SwapChainCreateInfo.imageFormat = m_VkSwapChainFormat.format;
	SwapChainCreateInfo.imageColorSpace = m_VkSwapChainFormat.colorSpace;
	SwapChainCreateInfo.imageExtent = m_VkSwapChainExtent;
	SwapChainCreateInfo.imageArrayLayers = 1;
	SwapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32 QueueFamilyIndicesArray[] = { Indices.Graphics.value(), Indices.Present.value() };

	if (Indices.Graphics.value() != Indices.Present.value())
	{
		SwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		SwapChainCreateInfo.queueFamilyIndexCount = 2;
		SwapChainCreateInfo.pQueueFamilyIndices = QueueFamilyIndicesArray;
	}
	else
	{
		SwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		SwapChainCreateInfo.queueFamilyIndexCount = 0;
		SwapChainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	SwapChainCreateInfo.preTransform = SwapChainDetails.Capabilities.currentTransform;
	SwapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	SwapChainCreateInfo.presentMode = PresentMode;
	SwapChainCreateInfo.clipped = true;

	CreateVkObject(m_VkDevice, m_VkSwapChain, SwapChainCreateInfo, vkCreateSwapchainKHR, vkDestroySwapchainKHR);

	m_VkSwapChainImages = VkEnumerate<VkImage>(vkGetSwapchainImagesKHR, m_VkDevice, m_VkSwapChain);

	m_VkImageViews.resize(m_VkSwapChainImages.size());

	for (size_t Index = 0; Index < m_VkSwapChainImages.size(); ++Index)
	{
		VkImageViewCreateInfo ImageViewCreateInfo{};
		ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ImageViewCreateInfo.image = m_VkSwapChainImages[Index];
		ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ImageViewCreateInfo.format = m_VkSwapChainFormat.format;

		// RGBA
		ImageViewCreateInfo.components = { 
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY 
		};

		ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ImageViewCreateInfo.subresourceRange.levelCount = 1;
		ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewCreateInfo.subresourceRange.layerCount = 1;

		ZN_VK_CHECK(vkCreateImageView(m_VkDevice, &ImageViewCreateInfo, nullptr/*allocator*/, &m_VkImageViews[Index]));
	}

	////// Command Pool

	VkCommandPoolCreateInfo GfxCmdPoolCreateInfo{};
	GfxCmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	//the command pool will be one that can submit graphics commands
	GfxCmdPoolCreateInfo.queueFamilyIndex = Indices.Graphics.value();
	//we also want the pool to allow for resetting of individual command buffers
	GfxCmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	CreateVkObject(m_VkDevice, m_VkCommandPool, GfxCmdPoolCreateInfo, vkCreateCommandPool, vkDestroyCommandPool);

	////// Command Buffer

	VkCommandBufferAllocateInfo GfxCmdBufferCreateInfo{};
	GfxCmdBufferCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	
	//commands will be made from our _commandPool
	GfxCmdBufferCreateInfo.commandPool = m_VkCommandPool;
	//we will allocate 1 command buffer
	GfxCmdBufferCreateInfo.commandBufferCount = 1;
	// command level is Primary
	GfxCmdBufferCreateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	for (size_t Index = 0; Index < kMaxFramesInFlight; ++Index)
	{
		ZN_VK_CHECK(vkAllocateCommandBuffers(m_VkDevice, &GfxCmdBufferCreateInfo, &m_VkCommandBuffers[Index]));
	}

	////// Render Pass

	// the renderpass will use this color attachment.
	VkAttachmentDescription ColorAttachmentDesc = {};
	//the attachment will have the format needed by the swapchain
	ColorAttachmentDesc.format = m_VkSwapChainFormat.format;
	//1 sample, we won't be doing MSAA
	ColorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	// we Clear when this attachment is loaded
	ColorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// we keep the attachment stored when the renderpass ends
	ColorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	//we don't care about stencil
	ColorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	ColorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	//we don't know or care about the starting layout of the attachment
	ColorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	//after the renderpass ends, the image has to be on a layout ready for display
	ColorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// Main Subpass

	VkAttachmentReference ColorAttachmentRef{};
	//attachment number will index into the pAttachments array in the parent renderpass itself
	ColorAttachmentRef.attachment = 0;
	ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//we are going to create 1 subpass, which is the minimum you can do
	VkSubpassDescription SubpassDesc = {};
	SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	SubpassDesc.colorAttachmentCount = 1;
	SubpassDesc.pColorAttachments = &ColorAttachmentRef;

	VkRenderPassCreateInfo RenderPassCreateInfo{};
	RenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	//connect the color attachment to the info
	RenderPassCreateInfo.attachmentCount = 1;
	RenderPassCreateInfo.pAttachments = &ColorAttachmentDesc;
	//connect the subpass to the info
	RenderPassCreateInfo.subpassCount = 1;
	RenderPassCreateInfo.pSubpasses = &SubpassDesc;

	/*
	* https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation | Subpass dependencies
	*/
	VkSubpassDependency SubpassDependency{};
	SubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	SubpassDependency.dstSubpass = 0;

	SubpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	SubpassDependency.srcAccessMask = 0;

	SubpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	SubpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	RenderPassCreateInfo.dependencyCount = 1;
	RenderPassCreateInfo.pDependencies = &SubpassDependency;

	CreateVkObject(m_VkDevice, m_VkRenderPass, RenderPassCreateInfo, vkCreateRenderPass, vkDestroyRenderPass);

	/////// Frame Buffers

	//create the framebuffers for the swapchain images. This will connect the render-pass to the images for rendering
	VkFramebufferCreateInfo FramebufferCreateInfo{};
	FramebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;

	FramebufferCreateInfo.renderPass = m_VkRenderPass;
	FramebufferCreateInfo.attachmentCount = 1;
	FramebufferCreateInfo.width = m_VkSwapChainExtent.width;
	FramebufferCreateInfo.height = m_VkSwapChainExtent.height;
	FramebufferCreateInfo.layers = 1;

	//grab how many images we have in the swapchain
	const size_t NumImages = m_VkSwapChainImages.size();
	m_VkFramebuffers = Vector<VkFramebuffer>(NumImages);

	//create framebuffers for each of the swapchain image views
	for (size_t Index = 0; Index < NumImages; Index++)
	{
		FramebufferCreateInfo.pAttachments = &m_VkImageViews[Index];
		CreateVkObject(m_VkDevice, m_VkFramebuffers[Index], FramebufferCreateInfo, vkCreateFramebuffer,
					   [&ImageView = m_VkImageViews[Index]](VkDevice InDevice, VkFramebuffer Framebuffer, const VkAllocationCallbacks* Allocator) mutable
					   {
						   vkDestroyFramebuffer(InDevice, Framebuffer, Allocator);
						   vkDestroyImageView(InDevice, ImageView, Allocator);
						   ImageView = VK_NULL_HANDLE;
					   });
	}

	////// Sync Structures

	VkFenceCreateInfo FenceCreateInfo{};
	FenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	//we want to create the fence with the Create Signaled flag, so we can wait on it before using it on a GPU command (for the first frame)
	FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t Index = 0; Index < kMaxFramesInFlight; ++Index)
	{
		CreateVkObject(m_VkDevice, m_VkRenderFences[Index], FenceCreateInfo, vkCreateFence, vkDestroyFence);
	}

	//for the semaphores we don't need any flags
	VkSemaphoreCreateInfo SemaphoreCreateInfo{};
	SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	SemaphoreCreateInfo.flags = 0;

	for (size_t Index = 0; Index < kMaxFramesInFlight; ++Index)
	{
		CreateVkObject(m_VkDevice, m_VkPresentSemaphores[Index], SemaphoreCreateInfo, vkCreateSemaphore, vkDestroySemaphore);
		CreateVkObject(m_VkDevice, m_VkRenderSemaphores[Index], SemaphoreCreateInfo, vkCreateSemaphore, vkDestroySemaphore);
	}

	////// ImGui
	{
		ImGui_ImplSDL2_InitForVulkan(InWindowHandle);

		// 1: create descriptor pool for IMGUI
			// the size of the pool is very oversize, but it's copied from imgui demo itself.
		VkDescriptorPoolSize ImGuiPoolSizes[] =
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

		VkDescriptorPoolCreateInfo ImGuiPoolCreateInfo{};
		ImGuiPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		ImGuiPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		ImGuiPoolCreateInfo.maxSets = 1000;
		ImGuiPoolCreateInfo.poolSizeCount = static_cast<uint32>(std::size(ImGuiPoolSizes));
		ImGuiPoolCreateInfo.pPoolSizes = ImGuiPoolSizes;

		CreateVkObject(m_VkDevice, m_VkImGuiDescriptorPool, ImGuiPoolCreateInfo, vkCreateDescriptorPool, vkDestroyDescriptorPool);

		//this initializes imgui for Vulkan
		ImGui_ImplVulkan_InitInfo ImGuiInitInfo{};
		ImGuiInitInfo.Instance = m_VkInstance;
		ImGuiInitInfo.PhysicalDevice = m_VkGPU;
		ImGuiInitInfo.Device = m_VkDevice;
		ImGuiInitInfo.Queue = m_VkGraphicsQueue;
		ImGuiInitInfo.DescriptorPool = m_VkImGuiDescriptorPool;
		ImGuiInitInfo.MinImageCount = 3;
		ImGuiInitInfo.ImageCount = 3;
		ImGuiInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&ImGuiInitInfo, m_VkRenderPass);

		// Upload Fonts

		VkCommandBuffer CmdBuffer = m_VkCommandBuffers[0];

		ZN_VK_CHECK(vkResetCommandBuffer(CmdBuffer, 0));

		VkCommandBufferBeginInfo CmdBufferBeginInfo{};
		CmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		CmdBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		ZN_VK_CHECK(vkBeginCommandBuffer(CmdBuffer, &CmdBufferBeginInfo));

		ImGui_ImplVulkan_CreateFontsTexture(CmdBuffer);

		ZN_VK_CHECK(vkEndCommandBuffer(CmdBuffer));

		VkSubmitInfo CmdBufferEndInfo{};
		CmdBufferEndInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		CmdBufferEndInfo.commandBufferCount = 1;
		CmdBufferEndInfo.pCommandBuffers = &CmdBuffer;

		ZN_VK_CHECK(vkQueueSubmit(m_VkGraphicsQueue, 1, &CmdBufferEndInfo, VK_NULL_HANDLE));

		ZN_VK_CHECK(vkDeviceWaitIdle(m_VkDevice));

		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	LoadShaders();

	////// Init Pipelines

	VkPipelineLayoutCreateInfo PipelineLayout{};
	PipelineLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	PipelineLayout.setLayoutCount = 0; // Optional
	PipelineLayout.pSetLayouts = nullptr; // Optional
	PipelineLayout.pushConstantRangeCount = 0; // Optional
	PipelineLayout.pPushConstantRanges = nullptr; // Optional
	
	CreateVkObject(m_VkDevice, m_VkPipelineLayout, PipelineLayout, vkCreatePipelineLayout, vkDestroyPipelineLayout);

	m_VkPipeline = VulkanPipeline::NewVkPipeline(m_VkDevice, m_VkRenderPass, m_VkVert, m_VkFrag, m_VkSwapChainExtent, m_VkPipelineLayout);

	m_DestroyQueue.Enqueue([this]()
	{
		VkDestroy(m_VkPipeline, m_VkDevice, vkDestroyPipeline);
	});

	LoadMeshes();

	CreateMeshPipeline();

	m_IsInitialized = true;
}

void VulkanDevice::Cleanup()
{
	if (m_IsInitialized == false)
	{
		return;
	}

	ZN_VK_CHECK(vkDeviceWaitIdle(m_VkDevice));

#if ZN_VK_VALIDATION_LAYERS
	DeinitializeDebugMessenger();
#endif

	m_DestroyQueue.Flush();	

	ImGui_ImplVulkan_Shutdown();

	for (size_t Index = 0; Index < kMaxFramesInFlight; ++Index)
	{
		m_VkCommandBuffers[Index] = VK_NULL_HANDLE;
	}

	m_VkFramebuffers.clear();
	m_VkImageViews.clear();

	if (m_VkSurface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(m_VkInstance, m_VkSurface, nullptr/*allocator*/);
		m_VkSurface = VK_NULL_HANDLE;
	}

	if (m_VkDevice != VK_NULL_HANDLE)
	{
		vkDestroyDevice(m_VkDevice, nullptr/*allocator*/);
		m_VkDevice = VK_NULL_HANDLE;
	}

	if (m_VkInstance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(m_VkInstance, nullptr/*allocator*/);
		m_VkInstance = VK_NULL_HANDLE;
	}

	m_IsInitialized = false;
}

void VulkanDevice::Draw()
{
	//wait until the GPU has finished rendering the last frame. Timeout of 1 second
	ZN_VK_CHECK(vkWaitForFences(m_VkDevice, 1, &m_VkRenderFences[m_CurrentFrame], VK_TRUE, 1000000000));
	ZN_VK_CHECK(vkResetFences(m_VkDevice, 1, &m_VkRenderFences[m_CurrentFrame]));

	//request image from the swapchain, one second timeout
	uint32 SwapChainImageIndex;
	//m_VkPresentSemaphore is set to make sure that we can sync other operations with the swapchain having an image ready to render.
	ZN_VK_CHECK(vkAcquireNextImageKHR(m_VkDevice, m_VkSwapChain, 1000000000, m_VkPresentSemaphores[m_CurrentFrame], nullptr/*fence*/, &SwapChainImageIndex));

	//now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
	ZN_VK_CHECK(vkResetCommandBuffer(m_VkCommandBuffers[m_CurrentFrame], 0));

	// Build ImGui render commands
	ImGui::Render();

	//naming it CmdBuffer for shorter writing
	VkCommandBuffer CmdBuffer = m_VkCommandBuffers[m_CurrentFrame];

	//begin the command buffer recording. We will use this command buffer exactly once, so we want to let Vulkan know that
	VkCommandBufferBeginInfo CmdBufferBeginInfo{};
	CmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	CmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	ZN_VK_CHECK(vkBeginCommandBuffer(CmdBuffer, &CmdBufferBeginInfo));

	VkClearValue ClearColor;
	ClearColor.color = { { 1.0f, 1.0f, 1.0f, 1.0f } };

	//start the main renderpass.
	//We will use the clear color from above, and the framebuffer of the index the swapchain gave us
	VkRenderPassBeginInfo RenderPassBeginInfo{};
	RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

	RenderPassBeginInfo.renderPass = m_VkRenderPass;
	RenderPassBeginInfo.renderArea.offset.x = 0;
	RenderPassBeginInfo.renderArea.offset.y = 0;
	RenderPassBeginInfo.renderArea.extent = m_VkSwapChainExtent;
	RenderPassBeginInfo.framebuffer = m_VkFramebuffers[SwapChainImageIndex];

	//connect clear values
	RenderPassBeginInfo.clearValueCount = 1;
	RenderPassBeginInfo.pClearValues = &ClearColor;

	vkCmdBeginRenderPass(CmdBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	// *** Insert Commands here ***

	vkCmdBindPipeline(CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VkPipeline);
	vkCmdDraw(CmdBuffer, 3, 1, 0, 0);

	// Enqueue ImGui commands to CmdBuffer
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), CmdBuffer);

	//finalize the render pass
	vkCmdEndRenderPass(CmdBuffer);
	//finalize the command buffer (we can no longer add commands, but it can now be executed)
	ZN_VK_CHECK(vkEndCommandBuffer(CmdBuffer));

	////// Submit

	//prepare the submission to the queue.
	//we want to wait on the m_VkPresentSemaphore, as that semaphore is signaled when the swapchain is ready
	//we will signal the m_VkRenderSemaphore, to signal that rendering has finished

	VkSemaphore WaitSemaphores[] = { m_VkPresentSemaphores[m_CurrentFrame] };
	VkSemaphore SignalSemaphores[] = { m_VkRenderSemaphores[m_CurrentFrame] };

	VkSubmitInfo SubmitInfo{};
	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags WaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	SubmitInfo.pWaitDstStageMask = &WaitStage;

	SubmitInfo.waitSemaphoreCount = 1;
	SubmitInfo.pWaitSemaphores = WaitSemaphores;

	SubmitInfo.signalSemaphoreCount = 1;
	SubmitInfo.pSignalSemaphores = SignalSemaphores;

	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &CmdBuffer;

	//submit command buffer to the queue and execute it.
	// _renderFence will now block until the graphic commands finish execution
	ZN_VK_CHECK(vkQueueSubmit(m_VkGraphicsQueue, 1, &SubmitInfo, m_VkRenderFences[m_CurrentFrame]));


	////// Present

	// this will put the image we just rendered into the visible window.
	// we want to wait on the _renderSemaphore for that,
	// as it's necessary that drawing commands have finished before the image is displayed to the user
	VkPresentInfoKHR PresentInfo = {};
	PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	PresentInfo.pSwapchains = &m_VkSwapChain;
	PresentInfo.swapchainCount = 1;

	PresentInfo.pWaitSemaphores = SignalSemaphores;
	PresentInfo.waitSemaphoreCount = 1;

	PresentInfo.pImageIndices = &SwapChainImageIndex;

	ZN_VK_CHECK(vkQueuePresentKHR(m_VkPresentQueue, &PresentInfo));

	m_CurrentFrame = (m_CurrentFrame + 1) % kMaxFramesInFlight;
}

bool VulkanDevice::SupportsValidationLayers() const
{
	Vector<VkLayerProperties> AvailableLayers = VkEnumerate<VkLayerProperties>(vkEnumerateInstanceLayerProperties);

	return std::any_of(AvailableLayers.begin(), AvailableLayers.end(), [](const VkLayerProperties& It)
	{
		for (const auto& LayerName : kValidationLayers)
		{
			if (strcmp(It.layerName, LayerName) == 0)
			{
				return true;
			}
		}

		return false;
	});
}

Vector<const char*> VulkanDevice::GetRequiredVkExtensions(SDL_Window* InWindowHandle) const
{
	uint32 ExtensionsCount = 0;
	SDL_Vulkan_GetInstanceExtensions(InWindowHandle, &ExtensionsCount, nullptr);
	Vector<const char*> ExtensionsNames(ExtensionsCount);
	SDL_Vulkan_GetInstanceExtensions(InWindowHandle, &ExtensionsCount, ExtensionsNames.data());

#if ZN_VK_VALIDATION_LAYERS
	static const Vector<const char*> kRequiredExtensions = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };

	ExtensionsCount += static_cast<uint32>(kRequiredExtensions.size());
	ExtensionsNames.insert(ExtensionsNames.end(), kRequiredExtensions.begin(), kRequiredExtensions.end());
#endif

	return ExtensionsNames;

}

bool VulkanDevice::HasRequiredDeviceExtensions(VkPhysicalDevice InDevice) const
{
	Vector<VkExtensionProperties> AvailableExtensions = VkEnumerate<VkExtensionProperties>(vkEnumerateDeviceExtensionProperties, InDevice, nullptr);

	static const Set<String> kRequiredDeviceExtensions(kDeviceExtensions.begin(), kDeviceExtensions.end());

	size_t NumFoundExtensions =
		std::count_if(AvailableExtensions.begin(), AvailableExtensions.end(), [](const VkExtensionProperties& InExtension)
		{
			return kRequiredDeviceExtensions.contains(InExtension.extensionName);
		});

	return kRequiredDeviceExtensions.size() == NumFoundExtensions;
}

VkDebugUtilsMessengerCreateInfoEXT VulkanDevice::GetDebugMessengerCreateInfo() const
{
	VkDebugUtilsMessengerCreateInfoEXT DebugCreateInfo{};

	DebugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
#if ZN_VK_VALIDATION_VERBOSE
	DebugCreateInfo.messageSeverity = (VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);
#else
	DebugCreateInfo.messageSeverity = (VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);
#endif
	DebugCreateInfo.messageType = (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT);
	DebugCreateInfo.pfnUserCallback = OnDebugMessage;
	DebugCreateInfo.pUserData = nullptr;

	return DebugCreateInfo;
}

void VulkanDevice::InitializeDebugMessenger()
{
	VkDebugUtilsMessengerCreateInfoEXT DebugUtilsInfo = GetDebugMessengerCreateInfo();

	// Load function
	auto vkCreateDebugUtilsMessengerEXTPtr = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_VkInstance, "vkCreateDebugUtilsMessengerEXT");
	if (vkCreateDebugUtilsMessengerEXTPtr != nullptr)
	{
		vkCreateDebugUtilsMessengerEXTPtr(m_VkInstance, &DebugUtilsInfo, nullptr/*Allocator*/, &m_DebugMessenger);
	}
	else
	{
		ZN_LOG(LogVulkanValidation, ELogVerbosity::Error, "vkCreateDebugUtilsMessengerEXT not available.");
	}
}

void VulkanDevice::DeinitializeDebugMessenger()
{
	if (m_DebugMessenger != VK_NULL_HANDLE)
	{
		// Load function
		auto vkDestroyDebugUtilsMessengerEXTPtr = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_VkInstance, "vkDestroyDebugUtilsMessengerEXT");
		if (vkDestroyDebugUtilsMessengerEXTPtr != nullptr)
		{
			vkDestroyDebugUtilsMessengerEXTPtr(m_VkInstance, m_DebugMessenger, nullptr/*Allocator*/);

			m_DebugMessenger = VK_NULL_HANDLE;
		}
	}
}

VkBool32 VulkanDevice::OnDebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT Severity,
									  VkDebugUtilsMessageTypeFlagsEXT Type,
									  const VkDebugUtilsMessengerCallbackDataEXT* Data,
									  void* UserData)
{
	ELogVerbosity Verbosity = VkMessageSeverityToZnVerbosity(Severity);

	const String& MessageType = VkMessageTypeToString(static_cast<VkDebugUtilsMessageTypeFlagBitsEXT>(Type));

	ZN_LOG(LogVulkanValidation, Verbosity, "[%s] %s", MessageType.c_str(), Data->pMessage);

	return VK_FALSE;
}

VkPhysicalDevice VulkanDevice::SelectPhysicalDevice(const Vector<VkPhysicalDevice>& InDevices) const
{
	size_t NumDevices = InDevices.size();

	size_t BestIndex = std::numeric_limits<size_t>::max();
	uint32 MaxScore = 0;

	for (size_t Index = 0; Index < NumDevices; ++Index)
	{
		VkPhysicalDevice Device = InDevices[Index];

		uint32 Score = 0;

		const bool HasGraphicsQueue = GetQueueFamilyIndices(Device).Graphics.has_value();

		const bool HasRequiredExtensions = HasRequiredDeviceExtensions(Device);

		if (HasGraphicsQueue && HasRequiredExtensions)
		{
			VkPhysicalDeviceProperties Properties;
			VkPhysicalDeviceFeatures Features;
			vkGetPhysicalDeviceProperties(Device, &Properties);
			vkGetPhysicalDeviceFeatures(Device, &Features);

			if (Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				Score += 1000;
			}

			// Texture size influences quality
			Score += Properties.limits.maxImageDimension2D;

			// TODO: Add more criteria to choose GPU.
		}

		if (Score > MaxScore && Score != 0)
		{
			MaxScore = Score;
			BestIndex = Index;
		}
	}

	if (BestIndex != std::numeric_limits<size_t>::max())
	{
		return InDevices[BestIndex];
	}
	else
	{
		return VK_NULL_HANDLE;
	}
}

Vk::QueueFamilyIndices VulkanDevice::GetQueueFamilyIndices(VkPhysicalDevice InDevice) const
{
	Vk::QueueFamilyIndices Indices;

	Vector<VkQueueFamilyProperties> QueueFamilies = VkEnumerate<VkQueueFamilyProperties>(vkGetPhysicalDeviceQueueFamilyProperties, InDevice);

	for (size_t Index = 0; Index < QueueFamilies.size(); ++Index)
	{
		const VkQueueFamilyProperties& QueueFamily = QueueFamilies[Index];

		if (QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			Indices.Graphics = static_cast<uint32>(Index);
		}

		VkBool32 PresentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(InDevice, static_cast<uint32>(Index), m_VkSurface, &PresentSupport);

		// tbd: We could enforce that Graphics and Present are in the same queue but is not mandatory.
		if (PresentSupport)
		{
			Indices.Present = static_cast<uint32>(Index);
		}
	}

	return Indices;
}

Vk::SwapChainDetails VulkanDevice::GetSwapChainDetails(VkPhysicalDevice InDevice) const
{
	Vk::SwapChainDetails Details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(InDevice, m_VkSurface, &Details.Capabilities);

	Details.Formats = VkEnumerate<VkSurfaceFormatKHR>(vkGetPhysicalDeviceSurfaceFormatsKHR, InDevice, m_VkSurface);
	Details.PresentModes = VkEnumerate<VkPresentModeKHR>(vkGetPhysicalDeviceSurfacePresentModesKHR, InDevice, m_VkSurface);

	return Details;
}

Vector<VkDeviceQueueCreateInfo> VulkanDevice::BuildQueueCreateInfo(const Vk::QueueFamilyIndices& InIndices) const
{
	Set<uint32> Queues = { InIndices.Graphics.value(), InIndices.Present.value() };

	Vector<VkDeviceQueueCreateInfo> OutCreateInfo{};
	OutCreateInfo.reserve(Queues.size());

	for (uint32 QueueIndex : Queues)
	{
		VkDeviceQueueCreateInfo DeviceQueueCreateInfo{};
		DeviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		DeviceQueueCreateInfo.queueFamilyIndex = QueueIndex;
		DeviceQueueCreateInfo.queueCount = 1;

		static const float kQueuePriority = 1.0f;

		DeviceQueueCreateInfo.pQueuePriorities = &kQueuePriority;

		OutCreateInfo.emplace_back(std::move(DeviceQueueCreateInfo));
	}

	return OutCreateInfo;
}

void Zn::VulkanDevice::LoadShaders()
{
	Vector<uint8> VertexShader,FragmentShader;
	
	const bool VertexShaderSuccess = IO::ReadBinaryFile("shaders/vertex.spv", VertexShader);
	const bool FragmentShaderSuccess = IO::ReadBinaryFile("shaders/fragment.spv", FragmentShader);

	m_VkVert = CreateShaderModule(VertexShader);
	m_VkFrag = CreateShaderModule(FragmentShader);

	m_DestroyQueue.Enqueue([this]()
	{
		VkDestroy(m_VkVert, m_VkDevice, vkDestroyShaderModule);
		VkDestroy(m_VkFrag, m_VkDevice, vkDestroyShaderModule);
	});

	if (m_VkVert == VK_NULL_HANDLE)
	{
		ZN_LOG(LogVulkan, ELogVerbosity::Error, "Failed to create vertex shader.");
	}

	if (m_VkFrag == VK_NULL_HANDLE)
	{
		ZN_LOG(LogVulkan, ELogVerbosity::Error, "Failed to create fragment shader.");
	}
}

VkShaderModule Zn::VulkanDevice::CreateShaderModule(const Vector<uint8>& InBytes)
{
	VkShaderModuleCreateInfo ShaderCreateInfo{};
	ShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	ShaderCreateInfo.codeSize = InBytes.size();
	ShaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(InBytes.data());

	VkShaderModule OutModule{ VK_NULL_HANDLE };
	vkCreateShaderModule(m_VkDevice, &ShaderCreateInfo, nullptr, &OutModule);

	return OutModule;
}

void Zn::VulkanDevice::LoadMeshes()
{
	m_Mesh = Vk::Mesh{};

	m_Mesh.Vertices.resize(3);

	//vertex positions
	m_Mesh.Vertices[0].Position[0] = 1.f;
	m_Mesh.Vertices[0].Position[1] = 1.f;
	m_Mesh.Vertices[0].Position[2] = 0.f;

	m_Mesh.Vertices[1].Position[0] = -1.f;
	m_Mesh.Vertices[1].Position[1] = 1.f;
	m_Mesh.Vertices[1].Position[2] = 0.0f;

	m_Mesh.Vertices[2].Position[0] = 0.0f;
	m_Mesh.Vertices[2].Position[1] = -1.f;
	m_Mesh.Vertices[2].Position[2] = 0.0f;

	m_Mesh.Vertices[0].Color[0] = 0.f;
	m_Mesh.Vertices[0].Color[1] = 1.f;
	m_Mesh.Vertices[0].Color[2] = 0.f;

	m_Mesh.Vertices[1].Color[0] = 0.0;
	m_Mesh.Vertices[1].Color[1] = 1.f;
	m_Mesh.Vertices[1].Color[2] = 0.0f;

	m_Mesh.Vertices[2].Color[0] = 0.0f;
	m_Mesh.Vertices[2].Color[1] = 1.f;
	m_Mesh.Vertices[2].Color[2] = 0.0f;

	Memory::Memzero(m_Mesh.Vertices[0].Normal);
	Memory::Memzero(m_Mesh.Vertices[1].Normal);
	Memory::Memzero(m_Mesh.Vertices[2].Normal);

	//we don't care about the vertex normals

	UploadMesh(m_Mesh);
}

void Zn::VulkanDevice::UploadMesh(Vk::Mesh & OutMesh)
{
	//allocate vertex buffer
	VkBufferCreateInfo CreateBufferInfo{};
	CreateBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	//this is the total size, in bytes, of the buffer we are allocating
	CreateBufferInfo.size = OutMesh.Vertices.size() * sizeof(Vk::Vertex);
	//this buffer is going to be used as a Vertex Buffer
	CreateBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	//let the VMA library know that this data should be writeable by CPU, but also readable by GPU
	VmaAllocationCreateInfo AllocationInfo{};
	AllocationInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	//allocate the buffer
	ZN_VK_CHECK(vmaCreateBuffer(m_VkAllocator, &CreateBufferInfo, &AllocationInfo,
			 &OutMesh.Buffer.Buffer,
			 &OutMesh.Buffer.Allocation,
			 nullptr));

	//add the destruction of triangle mesh buffer to the deletion queue
	m_DestroyQueue.Enqueue([=]()
	{
		vmaDestroyBuffer(m_VkAllocator, OutMesh.Buffer.Buffer, OutMesh.Buffer.Allocation);
	});

	//copy vertex data
	void* Data;
	vmaMapMemory(m_VkAllocator, OutMesh.Buffer.Allocation, &Data);

	memcpy(Data, OutMesh.Vertices.data(), OutMesh.Vertices.size() * sizeof(Vk::Vertex));

	vmaUnmapMemory(m_VkAllocator, OutMesh.Buffer.Allocation);
}

void Zn::VulkanDevice::CreateMeshPipeline()
{
	//build the mesh pipeline

	Vk::VertexInputDescription VertexDescription = Vk::Vertex::GetVertexInputDescription();



	//connect the pipeline builder vertex input info to the one we get from Vertex
	//pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
	//pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();

	//pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
	//pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();

	////clear the shader stages for the builder
	//pipelineBuilder._shaderStages.clear();

	////compile mesh vertex shader


	//VkShaderModule meshVertShader;
	//if (!load_shader_module("../../shaders/tri_mesh.vert.spv", &meshVertShader))
	//{
	//	std::cout << "Error when building the triangle vertex shader module" << std::endl;
	//}
	//else
	//{
	//	std::cout << "Red Triangle vertex shader successfully loaded" << std::endl;
	//}

	////add the other shaders
	//pipelineBuilder._shaderStages.push_back(
	//	vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, meshVertShader));

	////make sure that triangleFragShader is holding the compiled colored_triangle.frag
	//pipelineBuilder._shaderStages.push_back(
	//	vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));

	////build the mesh triangle pipeline
	//_meshPipeline = pipelineBuilder.build_pipeline(_device, _renderPass);

	////deleting all of the vulkan shaders
	//vkDestroyShaderModule(_device, meshVertShader, nullptr);
	//vkDestroyShaderModule(_device, redTriangleVertShader, nullptr);
	//vkDestroyShaderModule(_device, redTriangleFragShader, nullptr);
	//vkDestroyShaderModule(_device, triangleFragShader, nullptr);
	//vkDestroyShaderModule(_device, triangleVertexShader, nullptr);

	////adding the pipelines to the deletion queue
	//_mainDeletionQueue.push_function([=]()
 //{
	// vkDestroyPipeline(_device, _redTrianglePipeline, nullptr);
	// vkDestroyPipeline(_device, _trianglePipeline, nullptr);
	// vkDestroyPipeline(_device, _meshPipeline, nullptr);

	// vkDestroyPipelineLayout(_device, _trianglePipelineLayout, nullptr);
	//});
}

VulkanDevice::DestroyQueue::~DestroyQueue()
{
	Flush();
}

void Zn::VulkanDevice::DestroyQueue::Enqueue(std::function<void()> && InDestructor)
{
	m_Queue.emplace_back(std::move(InDestructor));
}

void Zn::VulkanDevice::DestroyQueue::Flush()
{
	while (!m_Queue.empty())
	{
		std::invoke(m_Queue.front());

		m_Queue.pop_front();
	}
}

template<typename TypePtr, typename OwnerType, typename CreateInfoType, typename VkCreateFunction, typename VkDestroyFunction>
void VulkanDevice::CreateVkObject(OwnerType Owner, TypePtr& OutObject, const CreateInfoType& CreateInfo, VkCreateFunction&& Create, VkDestroyFunction&& Destroy)
{
	ZN_VK_CHECK(VkCreate(Owner, OutObject, CreateInfo, std::move(Create)));

	m_DestroyQueue.Enqueue([this, &OutObject, Owner, Destructor = std::move(Destroy)]() mutable
	{
		VkDestroy(OutObject, Owner, Destructor);
	});
}