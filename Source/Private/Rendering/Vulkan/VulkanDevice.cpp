#include <Znpch.h>
#define VMA_IMPLEMENTATION
#include <Rendering/Vulkan/VulkanDevice.h>
#include <SDL.h>
#include <SDL_vulkan.h>

#include <Core/Containers/Set.h>
#include <Core/IO/IO.h>

#include <Core/Memory/Memory.h>

#include <Rendering/Vulkan/VulkanPipeline.h>
#include <Rendering/Vulkan/VulkanMaterialManager.h>

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

	template<typename TypePtr, typename OwnerType, typename VkDestroyFunction>
	void VkDestroy(const TypePtr& Object, OwnerType Owner, VkDestroyFunction&& InFunction)
	{
		if (Object != VK_NULL_HANDLE)
		{
			InFunction(Owner, Object, nullptr);
		}
	}
}

const Vector<const char*> VulkanDevice::kDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

VulkanDevice::VulkanDevice()
{	
}

VulkanDevice::~VulkanDevice()
{
	Cleanup();
}

void VulkanDevice::Initialize(SDL_Window* InWindowHandle, VkInstance InVkInstanceHandle, VkSurfaceKHR InVkSurface)
{
	m_VkInstance = InVkInstanceHandle;
	m_VkSurface = InVkSurface;

	m_WindowID = SDL_GetWindowID(InWindowHandle);

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

	CreateDescriptors();

	/////// Create Swap Chain

	CreateSwapChain();

	CreateImageViews();

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

	//	Color Attachment

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

	VkAttachmentReference ColorAttachmentRef{};
	//attachment number will index into the pAttachments array in the parent renderpass itself
	ColorAttachmentRef.attachment = 0;
	ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//	Depth Attachment

	VkAttachmentDescription DepthAttachmentDesc{};

	//	Depth attachment
	//	Both the depth attachment and its reference are copypaste of the color one, as it works the same, but with a small change:
	//	.format = m_DepthFormat; is set to the depth format that we created the depth image at.
	//	.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; 
	DepthAttachmentDesc.flags = 0;
	DepthAttachmentDesc.format = m_DepthFormat;
	DepthAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	DepthAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	DepthAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	DepthAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	DepthAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	DepthAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	DepthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference DepthAttachmentRef{};
	DepthAttachmentRef.attachment = 1;
	DepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Main Subpass

	//we are going to create 1 subpass, which is the minimum you can do
	VkSubpassDescription SubpassDesc = {};
	SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	SubpassDesc.colorAttachmentCount = 1;
	SubpassDesc.pColorAttachments = &ColorAttachmentRef;
	SubpassDesc.pDepthStencilAttachment = &DepthAttachmentRef;

	VkRenderPassCreateInfo RenderPassCreateInfo{};
	RenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	VkAttachmentDescription Attachments[2] = { ColorAttachmentDesc, DepthAttachmentDesc };
	//connect the color attachment to the info
	RenderPassCreateInfo.attachmentCount = 2;
	RenderPassCreateInfo.pAttachments = &Attachments[0];
	//connect the subpass to the info
	RenderPassCreateInfo.subpassCount = 1;
	RenderPassCreateInfo.pSubpasses = &SubpassDesc;

	/*
	* https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation | Subpass dependencies
	*/
	VkSubpassDependency ColorDependency{};
	ColorDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	ColorDependency.dstSubpass = 0;

	ColorDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	ColorDependency.srcAccessMask = 0;

	ColorDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	ColorDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	//	Add a new dependency that synchronizes access to depth attachments.
	//	Without this multiple frames can be rendered simultaneously by the GPU.

	VkSubpassDependency DepthDependency{};
	DepthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	DepthDependency.dstSubpass = 0;
	
	DepthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	DepthDependency.srcAccessMask = 0;

	DepthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	DepthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkSubpassDependency Dependencies[2] = { ColorDependency, DepthDependency };

	RenderPassCreateInfo.dependencyCount = 2;
	RenderPassCreateInfo.pDependencies = &Dependencies[0];

	CreateVkObject(m_VkDevice, m_VkRenderPass, RenderPassCreateInfo, vkCreateRenderPass, vkDestroyRenderPass);

	/////// Frame Buffers

	CreateFramebuffers();

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

	LoadMeshes();

	{
		auto test_texture = create_texture(IO::GetAbsolutePath("assets/texture.jpg"));
		textures.push_back(std::move(test_texture));
	}

	CreateMeshPipeline();

	CreateScene();

	m_IsInitialized = true;
}

void VulkanDevice::Cleanup()
{
	if (m_IsInitialized == false)
	{
		return;
	}

	ZN_VK_CHECK(vkDeviceWaitIdle(m_VkDevice));

	CleanupSwapChain();

	for (const Vk::AllocatedImage& texture : textures)
	{
		vmaDestroyImage(m_VkAllocator, texture.Image, texture.Allocation);
	}

	textures.clear();

	m_DestroyQueue.Flush();	

	ImGui_ImplVulkan_Shutdown();

	for (size_t Index = 0; Index < kMaxFramesInFlight; ++Index)
	{
		m_VkCommandBuffers[Index] = VK_NULL_HANDLE;
	}

	m_VkFramebuffers.clear();
	m_VkImageViews.clear();

	vmaDestroyAllocator(m_VkAllocator);

	//if (m_VkSurface != VK_NULL_HANDLE)
	//{
	//	vkDestroySurfaceKHR(m_VkInstance, m_VkSurface, nullptr/*allocator*/);
	//	m_VkSurface = VK_NULL_HANDLE;
	//}

	if (m_VkDevice != VK_NULL_HANDLE)
	{
		vkDestroyDevice(m_VkDevice, nullptr/*allocator*/);
		m_VkDevice = VK_NULL_HANDLE;
	}

	//if (m_VkInstance != VK_NULL_HANDLE)
	//{
	//	vkDestroyInstance(m_VkInstance, nullptr/*allocator*/);
	//	m_VkInstance = VK_NULL_HANDLE;
	//}

	m_IsInitialized = false;
}

void Zn::VulkanDevice::BeginFrame()
{
	//wait until the GPU has finished rendering the last frame. Timeout of 1 second
	ZN_VK_CHECK(vkWaitForFences(m_VkDevice, 1, &m_VkRenderFences[m_CurrentFrame], VK_TRUE, 1000000000));

	if (m_IsMinimized)
	{
		return;
	}

	//request image from the swapchain, one second timeout
	//m_VkPresentSemaphore is set to make sure that we can sync other operations with the swapchain having an image ready to render.
	VkResult AcquireImageResult = vkAcquireNextImageKHR(m_VkDevice, m_VkSwapChain, 1000000000, m_VkPresentSemaphores[m_CurrentFrame], nullptr/*fence*/, &m_SwapChainImageIndex);

	if (AcquireImageResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapChain();
		return;
	}
	else if (AcquireImageResult != VK_SUCCESS && AcquireImageResult != VK_SUBOPTIMAL_KHR)
	{
		_ASSERT(false);
		return;
	}

	ZN_VK_CHECK(vkResetFences(m_VkDevice, 1, &m_VkRenderFences[m_CurrentFrame]));

	//now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
	ZN_VK_CHECK(vkResetCommandBuffer(m_VkCommandBuffers[m_CurrentFrame], 0));
}

void VulkanDevice::Draw()
{
	if (m_IsMinimized)
	{
		return;
	}

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

	VkClearValue DepthClear;
	
	//	make a clear-color from frame number. This will flash with a 120 frame period.
	DepthClear.color = { { 0.0f, 0.0f, abs(sin(m_FrameNumber / 120.f)), 1.0f } };
	DepthClear.depthStencil.depth = 1.f;

	//start the main renderpass.
	//We will use the clear color from above, and the framebuffer of the index the swapchain gave us
	VkRenderPassBeginInfo RenderPassBeginInfo{};
	RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

	RenderPassBeginInfo.renderPass = m_VkRenderPass;
	RenderPassBeginInfo.renderArea.offset.x = 0;
	RenderPassBeginInfo.renderArea.offset.y = 0;
	RenderPassBeginInfo.renderArea.extent = m_VkSwapChainExtent;
	RenderPassBeginInfo.framebuffer = m_VkFramebuffers[m_SwapChainImageIndex];

	//	Connect clear values
	
	VkClearValue ClearValues[2] = { ClearColor, DepthClear };
	RenderPassBeginInfo.clearValueCount = 2;
	RenderPassBeginInfo.pClearValues = &ClearValues[0];

	vkCmdBeginRenderPass(CmdBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	// *** Insert Commands here ***	
	{
		DrawObjects(CmdBuffer, m_Renderables.data(), m_Renderables.size());

		// Enqueue ImGui commands to CmdBuffer
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), CmdBuffer);
	}


	//finalize the render pass
	vkCmdEndRenderPass(CmdBuffer);
	//finalize the command buffer (we can no longer add commands, but it can now be executed)
	ZN_VK_CHECK(vkEndCommandBuffer(CmdBuffer));

	
}

void Zn::VulkanDevice::EndFrame()
{
	if (m_IsMinimized)
	{
		return;
	}

	auto& CmdBuffer = m_VkCommandBuffers[m_CurrentFrame];
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

	PresentInfo.pImageIndices = &m_SwapChainImageIndex;

	VkResult QueuePresentResult = vkQueuePresentKHR(m_VkPresentQueue, &PresentInfo);

	if (QueuePresentResult == VK_ERROR_OUT_OF_DATE_KHR || QueuePresentResult == VK_SUBOPTIMAL_KHR)
	{
		RecreateSwapChain();
	}
	else
	{
		_ASSERT(QueuePresentResult == VK_SUCCESS);
	}

	m_CurrentFrame = (m_CurrentFrame + 1) % kMaxFramesInFlight;
	m_FrameNumber++;
}

void Zn::VulkanDevice::ResizeWindow()
{
	RecreateSwapChain();
}

void Zn::VulkanDevice::OnWindowMinimized()
{
	m_IsMinimized = true;
}

void Zn::VulkanDevice::OnWindowRestored()
{
	m_IsMinimized = false;
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

void Zn::VulkanDevice::CreateDescriptors()
{
	VkDescriptorSetLayoutBinding bindings[2];

	VkDescriptorSetLayoutBinding& CameraBufferBinding = bindings[0];
	CameraBufferBinding.binding = 0;
	CameraBufferBinding.descriptorCount = 1;
	CameraBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	CameraBufferBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	// #Lighting
	VkDescriptorSetLayoutBinding& lightingBufferBinding = bindings[1];
	lightingBufferBinding.binding = 1;
	lightingBufferBinding.descriptorCount = 1;
	lightingBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	lightingBufferBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo SetCreateInfo{};
	SetCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	SetCreateInfo.pNext = nullptr;
	SetCreateInfo.bindingCount = 2;
	SetCreateInfo.flags = 0;
	SetCreateInfo.pBindings = &bindings[0];

	CreateVkObject(m_VkDevice, m_VkGlobalSetLayout, SetCreateInfo, vkCreateDescriptorSetLayout, vkDestroyDescriptorSetLayout);

	VkDescriptorPoolSize PoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 };

	VkDescriptorPoolCreateInfo PoolCreateInfo{};
	PoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	PoolCreateInfo.flags = 0;
	PoolCreateInfo.maxSets = 10;
	PoolCreateInfo.poolSizeCount = 1;
	PoolCreateInfo.pPoolSizes = &PoolSize;

	CreateVkObject(m_VkDevice, m_VkDescriptorPool, PoolCreateInfo, vkCreateDescriptorPool, vkDestroyDescriptorPool);

	for (size_t Index = 0; Index < kMaxFramesInFlight; ++Index)
	{
		m_CameraBuffer[Index] = CreateBuffer(sizeof(Vk::GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		m_DestroyQueue.Enqueue([=]()
		{
			DestroyBuffer(m_CameraBuffer[Index]);
		});		

		// Lighting

		lighting_buffer[Index] = CreateBuffer(sizeof(Vk::LightingUniforms), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		m_DestroyQueue.Enqueue([=]()
		{
			DestroyBuffer(lighting_buffer[Index]);
		});

		VkDescriptorSetAllocateInfo SetAllocateInfo{};
		SetAllocateInfo.pNext = nullptr;
		SetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		SetAllocateInfo.descriptorPool = m_VkDescriptorPool;
		SetAllocateInfo.descriptorSetCount = 1;
		SetAllocateInfo.pSetLayouts = &m_VkGlobalSetLayout;

		vkAllocateDescriptorSets(m_VkDevice, &SetAllocateInfo, &m_VkGlobalDescriptorSet[Index]);

		VkDescriptorBufferInfo bufferInfo[2];

		VkDescriptorBufferInfo& cameraBufferInfo = bufferInfo[0];
		cameraBufferInfo.buffer = m_CameraBuffer[Index].Buffer;
		cameraBufferInfo.offset = 0;
		cameraBufferInfo.range = sizeof(Vk::GPUCameraData);

		VkDescriptorBufferInfo& lightingBufferInfo = bufferInfo[1];
		lightingBufferInfo.buffer = lighting_buffer[Index].Buffer;
		lightingBufferInfo.offset = 0;
		lightingBufferInfo.range = sizeof(Vk::LightingUniforms);

		VkWriteDescriptorSet descriptorWrites[2];

		VkWriteDescriptorSet& cameraWrite = descriptorWrites[0];

		cameraWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		cameraWrite.pNext = nullptr;
		// We are going to write into binding number 0
		cameraWrite.dstBinding = 0;
		cameraWrite.dstSet = m_VkGlobalDescriptorSet[Index];
		cameraWrite.descriptorCount = 1;
		cameraWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		cameraWrite.pBufferInfo = &bufferInfo[0];
		cameraWrite.dstArrayElement = 0;

		VkWriteDescriptorSet& lightingWrite = descriptorWrites[1];

		lightingWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		lightingWrite.pNext = nullptr;
		// We are going to write into binding number 1
		lightingWrite.dstBinding = 1;
		lightingWrite.dstSet = m_VkGlobalDescriptorSet[Index];
		lightingWrite.descriptorCount = 1;
		lightingWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		lightingWrite.pBufferInfo = &bufferInfo[1];
		lightingWrite.dstArrayElement = 0;

		vkUpdateDescriptorSets(m_VkDevice, 2, &descriptorWrites[0], 0, nullptr);
	}
}

void Zn::VulkanDevice::CreateSwapChain()
{
	Vk::SwapChainDetails SwapChainDetails = GetSwapChainDetails(m_VkGPU);

	Vk::QueueFamilyIndices Indices = GetQueueFamilyIndices(m_VkGPU);

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

	SDL_Window* WindowHandle = SDL_GetWindowFromID(m_WindowID);
	int32 Width, Height = 0;
	SDL_Vulkan_GetDrawableSize(WindowHandle, &Width, &Height);

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

	VkCreate(m_VkDevice, m_VkSwapChain, SwapChainCreateInfo, vkCreateSwapchainKHR);
}

void Zn::VulkanDevice::CreateImageViews()
{
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

	// Initialize Depth Buffer

	VkExtent3D DepthImageExtent
	{ 
		m_VkSwapChainExtent.width, 
		m_VkSwapChainExtent.height, 
		1 
	};

	//	Hardcoding to 32 bit float.
	//	Most GPUs support this depth format, so it’s fine to use it. You might want to choose other formats for other uses, or if you use Stencil buffer.
	m_DepthFormat = VK_FORMAT_D32_SFLOAT;

	//	VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT lets the vulkan driver know that this will be a depth image used for z-testing.
	VkImageCreateInfo DepthImageCreateInfo = Vk::AllocatedImage::GetImageCreateInfo(m_DepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, DepthImageExtent);


	//	Allocate from GPU memory.

	VmaAllocationCreateInfo DepthImageAllocInfo{};
	//	VMA_MEMORY_USAGE_GPU_ONLY to make sure that the image is allocated on fast VRAM.
	DepthImageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	//	To make absolutely sure that VMA really allocates the image into VRAM, we give it VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT on required flags. 
	//	This forces VMA library to allocate the image on VRAM no matter what. (The Memory Usage part is more like a hint)
	DepthImageAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	ZN_VK_CHECK(vmaCreateImage(m_VkAllocator, &DepthImageCreateInfo, &DepthImageAllocInfo, &m_DepthImage.Image, &m_DepthImage.Allocation, nullptr));

	//	VK_IMAGE_ASPECT_DEPTH_BIT lets the vulkan driver know that this will be a depth image used for z-testing.
	VkImageViewCreateInfo DepthImageViewCreateInfo = Vk::AllocatedImage::GetImageViewCreateInfo(m_DepthFormat, m_DepthImage.Image, VK_IMAGE_ASPECT_DEPTH_BIT);

	ZN_VK_CHECK(vkCreateImageView(m_VkDevice, &DepthImageViewCreateInfo, nullptr, &m_DepthImageView));

	m_DestroyQueue.Enqueue([=]()
	{
		vkDestroyImageView(m_VkDevice, m_DepthImageView, nullptr);
		vmaDestroyImage(m_VkAllocator, m_DepthImage.Image, m_DepthImage.Allocation);
	});
}

void Zn::VulkanDevice::CreateFramebuffers()
{
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
		VkImageView Attachments[2] = { m_VkImageViews[Index], m_DepthImageView };

		FramebufferCreateInfo.pAttachments = &Attachments[0];
		FramebufferCreateInfo.attachmentCount = 2;

		VkCreate(m_VkDevice, m_VkFramebuffers[Index], FramebufferCreateInfo, vkCreateFramebuffer);
	}
}

void Zn::VulkanDevice::CleanupSwapChain()
{
	for (size_t Index = 0; Index < m_VkFramebuffers.size(); ++Index)
	{
		vkDestroyFramebuffer(m_VkDevice, m_VkFramebuffers[Index], nullptr);
		vkDestroyImageView(m_VkDevice, m_VkImageViews[Index], nullptr);
	}

	vkDestroySwapchainKHR(m_VkDevice, m_VkSwapChain, nullptr);
}

void Zn::VulkanDevice::RecreateSwapChain()
{	
	vkDeviceWaitIdle(m_VkDevice);

	CleanupSwapChain();

	CreateSwapChain();

	CreateImageViews();

	CreateFramebuffers();
}

Vk::AllocatedBuffer Zn::VulkanDevice::CreateBuffer(size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{	
	VkBufferCreateInfo CreateInfo{};
	CreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	CreateInfo.pNext = nullptr;

	CreateInfo.size = size;
	CreateInfo.usage = usage;

	VmaAllocationCreateInfo AllocationInfo{};
	AllocationInfo.usage = memoryUsage;
	AllocationInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	Vk::AllocatedBuffer OutBuffer{};

	ZN_VK_CHECK(vmaCreateBuffer(m_VkAllocator, &CreateInfo, &AllocationInfo, &OutBuffer.Buffer, &OutBuffer.Allocation, nullptr));

	return OutBuffer;
}

void Zn::VulkanDevice::DestroyBuffer(Vk::AllocatedBuffer buffer)
{
	vmaDestroyBuffer(m_VkAllocator, buffer.Buffer, buffer.Allocation);
}

void Zn::VulkanDevice::CopyToGPU(VmaAllocation allocation, void* src, size_t size)
{
	void* dst;

	vmaMapMemory(m_VkAllocator, allocation, &dst);

	memcpy(dst, src, size);

	vmaUnmapMemory(m_VkAllocator, allocation);
}

Vk::Mesh* Zn::VulkanDevice::GetMesh(const String& InName)
{
	if (auto it = m_Meshes.find(InName); it != m_Meshes.end())
	{
		return &((*it).second);
	}

	return nullptr;
}

void Zn::VulkanDevice::DrawObjects(VkCommandBuffer InCommandBuffer, Vk::RenderObject* InFirst, int32 InCount)
{
	// Model View Matrix

	// Camera View	
	const glm::mat4 view = glm::lookAt(camera_position, camera_position + camera_direction, up_vector);

	// Camera Projection
	glm::mat4 projection = glm::perspective(glm::radians(60.f), 16.f / 9.f, 0.1f, 200.f);
	projection[1][1] *= -1;

	Vk::GPUCameraData camera{};
	camera.projection = projection;
	camera.view = view;
	camera.view_projection = projection * view;

	CopyToGPU(m_CameraBuffer[m_CurrentFrame].Allocation, &camera, sizeof(Vk::GPUCameraData));

	Vk::LightingUniforms lighting{};
	lighting.directional_lights[0].direction = glm::vec4(glm::normalize(glm::mat3(camera.view_projection) * glm::vec3(0.f, -1.f, 0.0f)), 0.0f);
	lighting.directional_lights[0].color = glm::vec4(1.0f, 0.8f, 0.8f, 0.f);
	lighting.directional_lights[0].intensity = 1.f;
	lighting.ambient_light.color = glm::vec4(0.f, 0.2f, 1.f, 0.f);
	lighting.ambient_light.intensity = 0.15f;
	lighting.num_directional_lights = 1;
	lighting.num_point_lights = 0;

	CopyToGPU(lighting_buffer[m_CurrentFrame].Allocation, &lighting, sizeof(Vk::LightingUniforms));

	Vk::Mesh* last_mesh = nullptr;
	Vk::Material* last_material = nullptr;

	for (int32 index = 0; index < InCount; ++index)
	{
		Vk::RenderObject& object = InFirst[index];

		// only bind the pipeline if it doesn't match what's already bound
		if (object.material != last_material)
		{
			vkCmdBindPipeline(InCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
			last_material = object.material;

			vkCmdBindDescriptorSets(InCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->layout, 0, 1, &m_VkGlobalDescriptorSet[m_CurrentFrame], 0, nullptr);
		}

		Vk::MeshPushConstants constants
		{
			.RenderMatrix = object.transform
		};

		vkCmdPushConstants(InCommandBuffer, object.material->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Vk::MeshPushConstants), &constants);

		// only bind the mesh if it's different from what's already bound

		if (object.mesh != last_mesh)
		{
			// bind the mesh v-buffer with offset 0
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(InCommandBuffer, 0, 1, &object.mesh->Buffer.Buffer, &offset);
			last_mesh = object.mesh;
		}

		// Draw

		vkCmdDraw(InCommandBuffer, object.mesh->Vertices.size(), 1, 0, 0);
	}
}

void Zn::VulkanDevice::CreateScene()
{
	Vk::RenderObject monkey
	{
		.mesh = GetMesh("monkey"),
		.material = VulkanMaterialManager::get().get_material("default"),
		.transform = glm::mat4(1.0f)
	};

	m_Renderables.push_back(monkey);

	for (int32 x = -20; x <= 20; ++x)
	{
		for (int32 y = -20; y <= 20; ++y)
		{
			glm::mat4 translation = glm::translate(glm::mat4{ 1.0f }, glm::vec3(x, 0, y));
			glm::mat4 rotate = glm::rotate(glm::mat4{ 1.0f }, glm::radians(25.f), glm::vec3(1.0f, 0.0f, 0.f));
			glm::mat4 scale = glm::scale(glm::mat4{ 1.0f }, glm::vec3(0.2f));

			Vk::RenderObject triangle
			{	
				.mesh = GetMesh("triangle"),
				.material = VulkanMaterialManager::get().get_material("default"),
				.transform = translation * rotate * scale
			};

			m_Renderables.push_back(triangle);
		}
	}
}

void Zn::VulkanDevice::LoadMeshes()
{
	Vk::Mesh triangle{};

	triangle.Vertices.resize(3);

	//vertex positions
	triangle.Vertices[0].Position = { 1.f, 1.f, 0.f };
	triangle.Vertices[1].Position = { -1.f, 1.f, 0.0f };
	triangle.Vertices[2].Position = { 0.f, -0.5f, 0.0f };

	triangle.Vertices[0].Color = { 1.0f, 0.f, 0.f };
	triangle.Vertices[1].Color = { 0.0f, 1.0f, 0.0f };
	triangle.Vertices[2].Color = { 0.0f, 0.0f, 1.0f };

	triangle.Vertices[0].Normal = glm::vec3(1.0f);
	triangle.Vertices[1].Normal = glm::vec3(1.0f);
	triangle.Vertices[2].Normal = glm::vec3(1.0f);

	//we don't care about the vertex normals

	UploadMesh(triangle);

	m_Meshes["triangle"] = triangle;

	Vk::Mesh monkey{};

	Vk::Obj::LoadMesh(IO::GetAbsolutePath("assets/VulkanGuide/monkey_smooth.obj"), monkey);

	UploadMesh(monkey);

	m_Meshes["monkey"] = monkey;
}

void Zn::VulkanDevice::UploadMesh(Vk::Mesh & OutMesh)
{
	//this is the total size, in bytes, of the buffer we are allocating
	const size_t allocationSize = OutMesh.Vertices.size() * sizeof(Vk::Vertex);

	//this buffer is going to be used as a Vertex Buffer
	VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	//let the VMA library know that this data should be writeable by CPU, but also readable by GPU
	VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	OutMesh.Buffer = CreateBuffer(allocationSize, usage, memoryUsage);

	//add the destruction of triangle mesh buffer to the deletion queue
	m_DestroyQueue.Enqueue([=]()
	{
		DestroyBuffer(OutMesh.Buffer);
	});

	//copy vertex data

	CopyToGPU(OutMesh.Buffer.Allocation, OutMesh.Vertices.data(), OutMesh.Vertices.size() * sizeof(Vk::Vertex));
}

void Zn::VulkanDevice::CreateMeshPipeline()
{
	Vector<uint8> vertex_shader_data;
	Vector<uint8> fragment_shader_data;

	const bool vertex_success = IO::ReadBinaryFile("shaders/tri_mesh_vertex.spv", vertex_shader_data);
	const bool fragment_success = IO::ReadBinaryFile("shaders/fragment.spv", fragment_shader_data);

	if (vertex_success && fragment_success)
	{
		Vk::Material* material = VulkanMaterialManager::get().create_material("default");

		material->vertex_shader = CreateShaderModule(vertex_shader_data);
		material->fragment_shader = CreateShaderModule(fragment_shader_data); 
		
		if (material->vertex_shader == VK_NULL_HANDLE)
		{
			ZN_LOG(LogVulkan, ELogVerbosity::Error, "Failed to create vertex shader.");
		}

		if (material->fragment_shader == VK_NULL_HANDLE)
		{
			ZN_LOG(LogVulkan, ELogVerbosity::Error, "Failed to create fragment shader.");
		}

		Vk::VertexInputDescription vertex_description = Vk::Vertex::GetVertexInputDescription();

		// Mesh pipeline with push constants 

		VkPipelineLayoutCreateInfo layout_create_info{};
		layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layout_create_info.setLayoutCount = 1;
		layout_create_info.pSetLayouts = &m_VkGlobalSetLayout;

		VkPushConstantRange push_constants{};
		push_constants.offset = 0;
		push_constants.size = sizeof(Vk::MeshPushConstants);
		//this push constant range is accessible only in the vertex shader
		push_constants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		layout_create_info.pPushConstantRanges = &push_constants;
		layout_create_info.pushConstantRangeCount = 1;

		CreateVkObject(m_VkDevice, material->layout, layout_create_info, vkCreatePipelineLayout, vkDestroyPipelineLayout);

		material->pipeline= VulkanPipeline::NewVkPipeline(m_VkDevice, m_VkRenderPass, material->vertex_shader, material->fragment_shader, m_VkSwapChainExtent, material->layout, vertex_description);

		m_DestroyQueue.Enqueue([=]()
		{
			VkDestroy(material->vertex_shader, m_VkDevice, vkDestroyShaderModule);
			VkDestroy(material->fragment_shader, m_VkDevice, vkDestroyShaderModule);
			VkDestroy(material->pipeline, m_VkDevice, vkDestroyPipeline);
		});
	}
}

Vk::AllocatedImage Zn::VulkanDevice::create_texture(const String& texture)
{
	struct Deleter
	{
		Deleter(Vk::RawTexture& inTarget)
			: target(inTarget)
		{
		}

		~Deleter()
		{
			if (target.data != nullptr)
			{
				Vk::RawTexture::destroy_image(target);
			}
		}
		Vk::RawTexture& target;
	};

	Vk::RawTexture rawTexture{};
	if (!Vk::RawTexture::create_texture_image(texture, rawTexture))
	{
		ZN_LOG(LogVulkan, ELogVerbosity::Warning, "Failed to create texture %s", texture.c_str());

		return Vk::AllocatedImage{};
	}

	auto scopedDeleter = Deleter(rawTexture);

	Vk::AllocatedBuffer stagingTexture = create_staging_texture(rawTexture);

	if (stagingTexture.Buffer == VK_NULL_HANDLE)
	{
		ZN_LOG(LogVulkan, ELogVerbosity::Error, "Failed to create stagin buffer for texture %s", texture.c_str());

		return Vk::AllocatedImage{};
	}

	auto outResult = create_texture_image(rawTexture.width, rawTexture.height, stagingTexture);

	transition_image_layout(outResult.Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	copy_buffer_to_image(stagingTexture.Buffer, outResult.Image, rawTexture.width, rawTexture.height);

	transition_image_layout(outResult.Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	DestroyBuffer(stagingTexture);

	return outResult;
}

Vk::AllocatedBuffer Zn::VulkanDevice::create_staging_texture(const Vk::RawTexture& inRawTexture)
{
	if (inRawTexture.data)
	{
		VkBufferUsageFlags texture_usage_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		Vk::AllocatedBuffer buffer = CreateBuffer(inRawTexture.size, texture_usage_flags, VMA_MEMORY_USAGE_CPU_TO_GPU);

		CopyToGPU(buffer.Allocation, inRawTexture.data, inRawTexture.size);

		return buffer;
	}

	return Vk::AllocatedBuffer{};
}

Vk::AllocatedImage Zn::VulkanDevice::create_texture_image(i32 width, i32 height, const Vk::AllocatedBuffer& inStagingTexture)
{
	Vk::AllocatedImage outImage{};

	VkImageCreateInfo createInfo = Vk::AllocatedImage::GetImageCreateInfo(
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VkExtent3D{ static_cast<u32>(width), static_cast<u32>(height), 1 });

	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	//	Allocate from GPU memory.

	VmaAllocationCreateInfo allocationInfo{};
	//	VMA_MEMORY_USAGE_GPU_ONLY to make sure that the image is allocated on fast VRAM.
	allocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	//	To make absolutely sure that VMA really allocates the image into VRAM, we give it VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT on required flags. 
	//	This forces VMA library to allocate the image on VRAM no matter what. (The Memory Usage part is more like a hint)
	allocationInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	ZN_VK_CHECK(vmaCreateImage(m_VkAllocator, &createInfo, &allocationInfo, &outImage.Image, &outImage.Allocation, nullptr));


	return outImage;
}

void Zn::VulkanDevice::transition_image_layout(VkImage img, VkFormat fmt, VkImageLayout prevLayout, VkImageLayout newLayout)
{
	VkCommandBuffer cmdBuffer = create_single_command_buffer();

	VkImageMemoryBarrier barrier{};

	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = prevLayout;
	barrier.newLayout = newLayout;

	// If you are using the barrier to transfer queue family ownership, then these two fields should be the indices of the queue families.
	// They must be set to VK_QUEUE_FAMILY_IGNORED if you don't want to do this (not the default value!).
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = img;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags srcStage{};
	VkPipelineStageFlags dstStage{};

	if (prevLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (prevLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		_ASSERT(false); // Unsupported Transition
	}
	
	vkCmdPipelineBarrier(cmdBuffer, 
						 srcStage, dstStage,
						 0, 
						 0, nullptr, 
						 0, nullptr, 
						 1, &barrier);

	end_single_command_buffer(cmdBuffer);
}

VkCommandBuffer Zn::VulkanDevice::create_single_command_buffer()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_VkCommandPool;
	allocInfo.commandBufferCount = 1;


	VkCommandBuffer cmdBuffer{};

	ZN_VK_CHECK(vkAllocateCommandBuffers(m_VkDevice, &allocInfo, &cmdBuffer));

	//	begin the command buffer recording. We will use this command buffer exactly once, so we want to let Vulkan know that
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	ZN_VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

	return cmdBuffer;
}

void Zn::VulkanDevice::end_single_command_buffer(VkCommandBuffer cmdBuffer)
{
	vkEndCommandBuffer(cmdBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	ZN_VK_CHECK(vkQueueSubmit(m_VkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
	ZN_VK_CHECK(vkQueueWaitIdle(m_VkGraphicsQueue));

	vkFreeCommandBuffers(m_VkDevice, m_VkCommandPool, 1, &cmdBuffer);
}

void Zn::VulkanDevice::copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
	VkCommandBuffer cmdBuffer = create_single_command_buffer();

	VkBufferCopy region{};
	region.size = size;
	vkCmdCopyBuffer(cmdBuffer, src, dst, 1, &region);

	end_single_command_buffer(cmdBuffer);
}

void Zn::VulkanDevice::copy_buffer_to_image(VkBuffer buffer, VkImage img, u32 width, u32 height)
{
	VkCommandBuffer cmdBuffer = create_single_command_buffer();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(cmdBuffer, buffer, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,	&region);
	
	end_single_command_buffer(cmdBuffer);
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
		if (Owner != VK_NULL_HANDLE && OutObject != VK_NULL_HANDLE)
		{
			VkDestroy(OutObject, Owner, Destructor);
		}
	});
}