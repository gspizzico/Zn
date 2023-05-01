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

#include <glm/gtx/matrix_decompose.hpp>

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

void VulkanDevice::Initialize(SDL_Window* InWindowHandle, vk::Instance inInstance, vk::SurfaceKHR inSurface)
{
	instance = inInstance;
	surface = inSurface;

	m_WindowID = SDL_GetWindowID(InWindowHandle);

	/////// Initialize GPU

	Vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

	gpu = SelectPhysicalDevice(devices);

	_ASSERT(gpu);

	/////// Initialize Logical Device

	Vk::QueueFamilyIndices Indices = GetQueueFamilyIndices(gpu);

	Vk::SwapChainDetails SwapChainDetails = GetSwapChainDetails(gpu);

	const bool IsSupported = SwapChainDetails.Formats.size() > 0 && SwapChainDetails.PresentModes.size() > 0;

	if (!IsSupported)
	{
		_ASSERT(false);
		return;
	}

	Vector<vk::DeviceQueueCreateInfo> queueFamilies = BuildQueueCreateInfo(Indices);

	vk::PhysicalDeviceFeatures deviceFeatures{};
	
	vk::DeviceCreateInfo deviceCreateInfo{};

	deviceCreateInfo.pQueueCreateInfos = queueFamilies.data();
	deviceCreateInfo.queueCreateInfoCount = static_cast<u32>(queueFamilies.size());
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	deviceCreateInfo.ppEnabledExtensionNames = kDeviceExtensions.data();
	deviceCreateInfo.enabledExtensionCount = static_cast<u32>(kDeviceExtensions.size());

	device = gpu.createDevice(deviceCreateInfo, nullptr);
	
	graphicsQueue = device.getQueue(Indices.Graphics.value(), 0);
	presentQueue = device.getQueue(Indices.Present.value(), 0);

	//initialize the memory allocator
	VmaAllocatorCreateInfo AllocatorCreateInfo{};
	AllocatorCreateInfo.instance = instance;
	AllocatorCreateInfo.physicalDevice = gpu;
	AllocatorCreateInfo.device = device;
	vmaCreateAllocator(&AllocatorCreateInfo, &m_VkAllocator);

	CreateDescriptors();

	/////// Create Swap Chain

	CreateSwapChain();

	CreateImageViews();

	////// Command Pool

	vk::CommandPoolCreateInfo graphicsPoolCreateInfo{};
	//the command pool will be one that can submit graphics commands
	graphicsPoolCreateInfo.queueFamilyIndex = Indices.Graphics.value();
	//we also want the pool to allow for resetting of individual command buffers
	graphicsPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

	commandPool = device.createCommandPool(graphicsPoolCreateInfo);

	m_DestroyQueue.Enqueue([=]()
	{
		device.destroyCommandPool(commandPool);
	});

	////// Command Buffer

	vk::CommandBufferAllocateInfo graphicsCmdCreateInfo(
		commandPool,
		vk::CommandBufferLevel::ePrimary, 
		kMaxFramesInFlight
	);

	commandBuffers = device.allocateCommandBuffers(graphicsCmdCreateInfo);

	// Upload Context

	vk::CommandPoolCreateInfo uploadCmdPoolCreateInfo{};
	uploadCmdPoolCreateInfo.queueFamilyIndex = Indices.Graphics.value();
	uploadCmdPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

	uploadContext.commandPool = device.createCommandPool(graphicsPoolCreateInfo);

	m_DestroyQueue.Enqueue([=]()
	{
		device.destroyCommandPool(uploadContext.commandPool);
	});

	vk::CommandBufferAllocateInfo uploadCmdBufferCreateInfo(
		uploadContext.commandPool,
		vk::CommandBufferLevel::ePrimary,
		1
	);
	
	uploadContext.cmdBuffer = device.allocateCommandBuffers(uploadCmdBufferCreateInfo)[0];

	vk::FenceCreateInfo uploadFenceCreateInfo{};
	uploadFenceCreateInfo.flags = (vk::FenceCreateFlagBits) 0;
	uploadContext.fence = device.createFence(uploadFenceCreateInfo);

	m_DestroyQueue.Enqueue([=]()
	{
		device.destroyFence(uploadContext.fence);
	});

	////// Render Pass

	//	Color Attachment

	VULKAN_HPP_NAMESPACE::AttachmentDescriptionFlags flags = {};
	VULKAN_HPP_NAMESPACE::Format                     format = VULKAN_HPP_NAMESPACE::Format::eUndefined;
	VULKAN_HPP_NAMESPACE::SampleCountFlagBits        samples = VULKAN_HPP_NAMESPACE::SampleCountFlagBits::e1;
	VULKAN_HPP_NAMESPACE::AttachmentLoadOp           loadOp = VULKAN_HPP_NAMESPACE::AttachmentLoadOp::eLoad;
	VULKAN_HPP_NAMESPACE::AttachmentStoreOp          storeOp = VULKAN_HPP_NAMESPACE::AttachmentStoreOp::eStore;
	VULKAN_HPP_NAMESPACE::AttachmentLoadOp           stencilLoadOp = VULKAN_HPP_NAMESPACE::AttachmentLoadOp::eLoad;
	VULKAN_HPP_NAMESPACE::AttachmentStoreOp          stencilStoreOp = VULKAN_HPP_NAMESPACE::AttachmentStoreOp::eStore;
	VULKAN_HPP_NAMESPACE::ImageLayout                initialLayout = VULKAN_HPP_NAMESPACE::ImageLayout::eUndefined;
	VULKAN_HPP_NAMESPACE::ImageLayout                finalLayout = VULKAN_HPP_NAMESPACE::ImageLayout::eUndefined;

	// the renderpass will use this color attachment.
	vk::AttachmentDescription ColorAttachmentDesc = {};
	//the attachment will have the format needed by the swapchain
	ColorAttachmentDesc.format = m_VkSwapChainFormat.format;
	//1 sample, we won't be doing MSAA
	ColorAttachmentDesc.samples = vk::SampleCountFlagBits::e1;
	// we Clear when this attachment is loaded
	ColorAttachmentDesc.loadOp = vk::AttachmentLoadOp::eClear;
	// we keep the attachment stored when the renderpass ends
	ColorAttachmentDesc.storeOp = vk::AttachmentStoreOp::eStore;
	//we don't care about stencil
	ColorAttachmentDesc.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	ColorAttachmentDesc.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	//we don't know or care about the starting layout of the attachment
	ColorAttachmentDesc.initialLayout = vk::ImageLayout::eUndefined;
	//after the renderpass ends, the image has to be on a layout ready for display
	ColorAttachmentDesc.finalLayout = vk::ImageLayout::ePresentSrcKHR;

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

	m_VkRenderPass = device.createRenderPass(RenderPassCreateInfo);

	m_DestroyQueue.Enqueue([=]()
	{
		device.destroyRenderPass(m_VkRenderPass);
	});

	/////// Frame Buffers

	CreateFramebuffers();

	////// Sync Structures

	vk::FenceCreateInfo FenceCreateInfo{};
	//we want to create the fence with the Create Signaled flag, so we can wait on it before using it on a GPU command (for the first frame)
	FenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;

	for (size_t Index = 0; Index < kMaxFramesInFlight; ++Index)
	{
		m_VkRenderFences[Index] = device.createFence(FenceCreateInfo);
		m_DestroyQueue.Enqueue([=]()
		{
			device.destroyFence(m_VkRenderFences[Index]);
		});
	}

	//for the semaphores we don't need any flags
	vk::SemaphoreCreateInfo semaphoreCreateInfo{};

	for (size_t Index = 0; Index < kMaxFramesInFlight; ++Index)
	{
		m_VkPresentSemaphores[Index] = device.createSemaphore(semaphoreCreateInfo);
		m_VkRenderSemaphores[Index] = device.createSemaphore(semaphoreCreateInfo);

		m_DestroyQueue.Enqueue([=]()
		{
			device.destroySemaphore(m_VkPresentSemaphores[Index]);
			device.destroySemaphore(m_VkRenderSemaphores[Index]);
		});
	}

	////// ImGui
	{
		ImGui_ImplSDL2_InitForVulkan(InWindowHandle);

		// 1: create descriptor pool for IMGUI
			// the size of the pool is very oversize, but it's copied from imgui demo itself.

		Vector<vk::DescriptorPoolSize> ImGuiPoolSizes =
		{
			{ vk::DescriptorType::eSampler, 1000 },
			{ vk::DescriptorType::eCombinedImageSampler, 1000 },
			{ vk::DescriptorType::eSampledImage, 1000 },
			{ vk::DescriptorType::eStorageImage, 1000 },
			{ vk::DescriptorType::eUniformTexelBuffer, 1000 },
			{ vk::DescriptorType::eStorageTexelBuffer, 1000 },
			{ vk::DescriptorType::eUniformBuffer, 1000 },
			{ vk::DescriptorType::eStorageBuffer, 1000 },
			{ vk::DescriptorType::eUniformBufferDynamic, 1000 },
			{ vk::DescriptorType::eStorageBufferDynamic, 1000 },
			{ vk::DescriptorType::eInputAttachment, 1000 }
		};
		
		vk::DescriptorPoolCreateInfo imGuiPoolCreateInfo(
			vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
			1000, 
			ImGuiPoolSizes);

		m_VkImGuiDescriptorPool = device.createDescriptorPool(imGuiPoolCreateInfo);

		m_DestroyQueue.Enqueue([=]()
		{
			device.destroyDescriptorPool(m_VkImGuiDescriptorPool);
		});

		//this initializes imgui for Vulkan
		ImGui_ImplVulkan_InitInfo ImGuiInitInfo{};
		ImGuiInitInfo.Instance = instance;
		ImGuiInitInfo.PhysicalDevice = gpu;
		ImGuiInitInfo.Device = device;
		ImGuiInitInfo.Queue = graphicsQueue;
		ImGuiInitInfo.DescriptorPool = m_VkImGuiDescriptorPool;
		ImGuiInitInfo.MinImageCount = 3;
		ImGuiInitInfo.ImageCount = 3;
		ImGuiInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&ImGuiInitInfo, m_VkRenderPass);

		// Upload Fonts

		VkCommandBuffer CmdBuffer = commandBuffers[0];

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

		ZN_VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &CmdBufferEndInfo, VK_NULL_HANDLE));

		ZN_VK_CHECK(vkDeviceWaitIdle(device));

		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	LoadMeshes();

	{
		textures["texture"] = CreateTexture(IO::GetAbsolutePath("assets/texture.jpg"));
		textures["viking_room"] = CreateTexture(IO::GetAbsolutePath("assets/VulkanTutorial/viking_room.png"));
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

	ZN_VK_CHECK(vkDeviceWaitIdle(device));

	CleanupSwapChain();

	for (auto& textureKvp : textures)
	{
		vkDestroyImageView(device, textureKvp.second.imageView, nullptr);
		vmaDestroyImage(m_VkAllocator, textureKvp.second.Image, textureKvp.second.Allocation);
	}

	textures.clear();

	m_Meshes.clear();

	m_DestroyQueue.Flush();	

	ImGui_ImplVulkan_Shutdown();

	for (size_t Index = 0; Index < kMaxFramesInFlight; ++Index)
	{
		commandBuffers[Index] = VK_NULL_HANDLE;
	}

	uploadContext.cmdBuffer = VK_NULL_HANDLE;

	m_VkFramebuffers.clear();
	m_VkImageViews.clear();

	vmaDestroyAllocator(m_VkAllocator);

	//if (surface != VK_NULL_HANDLE)
	//{
	//	vkDestroySurfaceKHR(instance, surface, nullptr/*allocator*/);
	//	surface = VK_NULL_HANDLE;
	//}

	if (device)
	{
		device.destroy();
	}

	//if (instance != VK_NULL_HANDLE)
	//{
	//	vkDestroyInstance(instance, nullptr/*allocator*/);
	//	instance = VK_NULL_HANDLE;
	//}

	m_IsInitialized = false;
}

void Zn::VulkanDevice::BeginFrame()
{
	//wait until the GPU has finished rendering the last frame. Timeout of 1 second
	
	// device.waitForFences(vk::ArrayProxy(1, m_VkRenderFences), VK_TRUE, kWaitTimeOneSecond);
	ZN_VK_CHECK(vkWaitForFences(device, 1, &m_VkRenderFences[m_CurrentFrame], VK_TRUE, kWaitTimeOneSecond));

	if (m_IsMinimized)
	{
		return;
	}

	//request image from the swapchain, one second timeout
	//m_VkPresentSemaphore is set to make sure that we can sync other operations with the swapchain having an image ready to render.
	VkResult AcquireImageResult = vkAcquireNextImageKHR(device, swapChain, 1000000000, m_VkPresentSemaphores[m_CurrentFrame], nullptr/*fence*/, &m_SwapChainImageIndex);

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

	ZN_VK_CHECK(vkResetFences(device, 1, &m_VkRenderFences[m_CurrentFrame]));

	//now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
	commandBuffers[m_CurrentFrame].reset();
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
	VkCommandBuffer CmdBuffer = commandBuffers[m_CurrentFrame];

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

	auto& CmdBuffer = commandBuffers[m_CurrentFrame];
	////// Submit

	//prepare the submission to the queue.
	//we want to wait on the m_VkPresentSemaphore, as that semaphore is signaled when the swapchain is ready
	//we will signal the m_VkRenderSemaphore, to signal that rendering has finished

	vk::Semaphore waitSemaphores[] = { m_VkPresentSemaphores[m_CurrentFrame] };
	vk::Semaphore signalSemaphores[] = { m_VkRenderSemaphores[m_CurrentFrame] };

	vk::SubmitInfo submitInfo{};

	vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	submitInfo.pWaitDstStageMask = &waitStage;

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &CmdBuffer;


	//submit command buffer to the queue and execute it.
	// _renderFence will now block until the graphic commands finish execution
	graphicsQueue.submit(submitInfo, m_VkRenderFences[m_CurrentFrame]);

	////// Present

	// this will put the image we just rendered into the visible window.
	// we want to wait on the _renderSemaphore for that,
	// as it's necessary that drawing commands have finished before the image is displayed to the user
	vk::PresentInfoKHR presentInfo = {};

	presentInfo.pSwapchains = &swapChain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &m_SwapChainImageIndex;
	
	vk::Result presentResult = presentQueue.presentKHR(presentInfo);

	if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR)
	{
		RecreateSwapChain();
	}
	else
	{
		_ASSERT(presentResult == vk::Result::eSuccess);
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

bool VulkanDevice::HasRequiredDeviceExtensions(vk::PhysicalDevice inDevice) const
{
	Vector<vk::ExtensionProperties> availableExtensions = inDevice.enumerateDeviceExtensionProperties();

	static const Set<String> kRequiredDeviceExtensions(kDeviceExtensions.begin(), kDeviceExtensions.end());
	
	u32 numFoundExtensions =
		std::count_if(availableExtensions.begin(), availableExtensions.end(), [](const vk::ExtensionProperties& extension)
		{
			return kRequiredDeviceExtensions.contains(extension.extensionName);
		});

	return kRequiredDeviceExtensions.size() == numFoundExtensions;
}

vk::PhysicalDevice VulkanDevice::SelectPhysicalDevice(const Vector<vk::PhysicalDevice>& inDevices) const
{
	u32 num = inDevices.size();

	i32 selectedIndex = std::numeric_limits<size_t>::max();
	u32 maxScore = 0;

	for (i32 idx = 0; idx < num; ++idx)
	{
		vk::PhysicalDevice device = inDevices[idx];

		u32 deviceScore = 0;

		const bool hasGraphicsQueue = GetQueueFamilyIndices(device).Graphics.has_value();

		const bool hasRequiredExtensions = HasRequiredDeviceExtensions(device);

		if (hasGraphicsQueue && hasRequiredExtensions)
		{
			vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
			vk::PhysicalDeviceFeatures deviceFeatures = device.getFeatures();

			if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			{
				deviceScore += 1000;
			}

			// Texture size influences quality
			deviceScore += deviceProperties.limits.maxImageDimension2D;

			// TODO: Add more criteria to choose GPU.
		}

		if (deviceScore > maxScore && deviceScore != 0)
		{
			maxScore = deviceScore;
			selectedIndex = idx;
		}
	}

	if (selectedIndex != std::numeric_limits<size_t>::max())
	{
		return inDevices[selectedIndex];
	}
	else
	{
		return VK_NULL_HANDLE;
	}
}

Vk::QueueFamilyIndices VulkanDevice::GetQueueFamilyIndices(vk::PhysicalDevice inDevice) const
{
	Vk::QueueFamilyIndices outIndices;

	Vector<vk::QueueFamilyProperties> queueFamilies = inDevice.getQueueFamilyProperties();

	for (u32 idx = 0; idx < queueFamilies.size(); ++idx)
	{
		const vk::QueueFamilyProperties& queueFamily = queueFamilies[idx];

		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			outIndices.Graphics = idx;
		}

		// tbd: We could enforce that Graphics and Present are in the same queue but is not mandatory.
		if (inDevice.getSurfaceSupportKHR(idx, surface) == VK_TRUE)
		{
			outIndices.Present = idx;
		}
	}

	return outIndices;
}

Vk::SwapChainDetails VulkanDevice::GetSwapChainDetails(vk::PhysicalDevice inGPU) const
{
	Vk::SwapChainDetails outDetails
	{
		.Capabilities = inGPU.getSurfaceCapabilitiesKHR(surface),
		.Formats = inGPU.getSurfaceFormatsKHR(surface),
		.PresentModes = inGPU.getSurfacePresentModesKHR(surface)
	};

	return outDetails;
}

Vector<vk::DeviceQueueCreateInfo> VulkanDevice::BuildQueueCreateInfo(const Vk::QueueFamilyIndices& InIndices) const
{
	UnorderedSet<u32> queues = { InIndices.Graphics.value(), InIndices.Present.value() };

	Vector<vk::DeviceQueueCreateInfo> outCreateInfo{};
	outCreateInfo.reserve(queues.size());

	for (uint32 queueIdx : queues)
	{
		vk::DeviceQueueCreateInfo queueInfo;
		queueInfo.queueFamilyIndex = queueIdx;
		queueInfo.queueCount = 1;

		static const float kQueuePriority = 1.0f;

		queueInfo.pQueuePriorities = &kQueuePriority;

		outCreateInfo.emplace_back(std::move(queueInfo));
	}

	return outCreateInfo;
}

VkShaderModule Zn::VulkanDevice::CreateShaderModule(const Vector<uint8>& InBytes)
{
	VkShaderModuleCreateInfo ShaderCreateInfo{};
	ShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	ShaderCreateInfo.codeSize = InBytes.size();
	ShaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(InBytes.data());

	VkShaderModule OutModule{ VK_NULL_HANDLE };
	vkCreateShaderModule(device, &ShaderCreateInfo, nullptr, &OutModule);

	return OutModule;
}

void Zn::VulkanDevice::CreateDescriptors()
{
	// Create Descriptor Pool
	Vector<vk::DescriptorPoolSize> poolSizes =
	{
		{ vk::DescriptorType::eUniformBuffer, 10 },
		{ vk::DescriptorType::eUniformBufferDynamic, 10 },
		{ vk::DescriptorType::eStorageBuffer, 10 },
		{ vk::DescriptorType::eCombinedImageSampler, 10 }
	};

	vk::DescriptorPoolCreateInfo poolCreateInfo(
		(vk::DescriptorPoolCreateFlagBits) 0,
		10,
		poolSizes);

	m_VkDescriptorPool = device.createDescriptorPool(poolCreateInfo);

	// Global Set

	vk::DescriptorSetLayoutBinding bindings[2];

	vk::DescriptorSetLayoutBinding& cameraBufferBinding = bindings[0];
	cameraBufferBinding.binding = 0;
	cameraBufferBinding.descriptorCount = 1;
	cameraBufferBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
	cameraBufferBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

	// #Lighting
	vk::DescriptorSetLayoutBinding& lightingBufferBinding = bindings[1];
	lightingBufferBinding.binding = 1;
	lightingBufferBinding.descriptorCount = 1;
	lightingBufferBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
	lightingBufferBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

	vk::DescriptorSetLayoutCreateInfo globalSetCreateInfo({}, bindings);

	m_VkGlobalSetLayout = device.createDescriptorSetLayout(globalSetCreateInfo);

	m_DestroyQueue.Enqueue([=]()
	{
		device.destroyDescriptorSetLayout(m_VkGlobalSetLayout);
	});

	// Single Texture Set

	vk::DescriptorSetLayoutBinding singleTextureSetBinding{};
	singleTextureSetBinding.binding = 0;
	singleTextureSetBinding.descriptorCount = 1;
	singleTextureSetBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	singleTextureSetBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

	vk::DescriptorSetLayoutCreateInfo singleTextureSetCreateInfo({}, singleTextureSetBinding);

	singleTextureSetLayout = device.createDescriptorSetLayout(singleTextureSetCreateInfo);

	m_DestroyQueue.Enqueue([=]()
	{
		device.destroyDescriptorSetLayout(singleTextureSetLayout);
	});

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

		vkAllocateDescriptorSets(device, &SetAllocateInfo, &m_VkGlobalDescriptorSet[Index]);

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

		vkUpdateDescriptorSets(device, 2, &descriptorWrites[0], 0, nullptr);
	}


}

void Zn::VulkanDevice::CreateSwapChain()
{
	Vk::SwapChainDetails SwapChainDetails = GetSwapChainDetails(gpu);

	Vk::QueueFamilyIndices Indices = GetQueueFamilyIndices(gpu);

	vk::SurfaceFormatKHR surfaceFormat = { vk::Format::eUndefined, vk::ColorSpaceKHR::eSrgbNonlinear };

	for (const vk::SurfaceFormatKHR& availableFormat : SwapChainDetails.Formats)
	{
		if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			surfaceFormat = availableFormat;
			break;
		}
	}

	if (surfaceFormat.format == vk::Format::eUndefined)
	{
		surfaceFormat = SwapChainDetails.Formats[0];
	}

	// Always guaranteed to be available.
	vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;

	const bool useMailboxPresentMode = std::any_of(SwapChainDetails.PresentModes.begin(), SwapChainDetails.PresentModes.end(),
												   [](vk::PresentModeKHR InPresentMode)
												   {
													   // 'Triple Buffering'
													   return InPresentMode == vk::PresentModeKHR::eMailbox;
												   });

	if (useMailboxPresentMode)
	{
		presentMode = vk::PresentModeKHR::eMailbox;
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

	m_VkSwapChainFormat = surfaceFormat;

	m_VkSwapChainExtent = VkExtent2D(Width, Height);

	vk::SwapchainCreateInfoKHR swapChainCreateInfo{};
	swapChainCreateInfo.surface = surface;
	swapChainCreateInfo.minImageCount = ImageCount;
	swapChainCreateInfo.imageFormat = m_VkSwapChainFormat.format;
	swapChainCreateInfo.imageColorSpace = m_VkSwapChainFormat.colorSpace;
	swapChainCreateInfo.imageExtent = m_VkSwapChainExtent;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

	uint32 QueueFamilyIndicesArray[] = { Indices.Graphics.value(), Indices.Present.value() };

	if (Indices.Graphics.value() != Indices.Present.value())
	{
		swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		swapChainCreateInfo.pQueueFamilyIndices = QueueFamilyIndicesArray;
	}
	else
	{
		swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	swapChainCreateInfo.preTransform = SwapChainDetails.Capabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

	swapChainCreateInfo.presentMode = presentMode;
	swapChainCreateInfo.clipped = true;

	swapChain = device.createSwapchainKHR(swapChainCreateInfo);
}

void Zn::VulkanDevice::CreateImageViews()
{
	m_VkSwapChainImages = VkEnumerate<VkImage>(vkGetSwapchainImagesKHR, device, swapChain);

	m_VkImageViews.resize(m_VkSwapChainImages.size());

	for (size_t Index = 0; Index < m_VkSwapChainImages.size(); ++Index)
	{
		vk::ImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.image = m_VkSwapChainImages[Index];
		imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
		imageViewCreateInfo.format = m_VkSwapChainFormat.format;

		// RGBA
		imageViewCreateInfo.components = {
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity
		};

		vk::ImageSubresourceRange subresourceRange{};
		subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = 1;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.layerCount = 1;

		imageViewCreateInfo.setSubresourceRange(subresourceRange);

		m_VkImageViews[Index] = device.createImageView(imageViewCreateInfo);
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

	ZN_VK_CHECK(vkCreateImageView(device, &DepthImageViewCreateInfo, nullptr, &m_DepthImageView));

	m_DestroyQueue.Enqueue([=]()
	{
		vkDestroyImageView(device, m_DepthImageView, nullptr);
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

		VkCreate(device, m_VkFramebuffers[Index], FramebufferCreateInfo, vkCreateFramebuffer);
	}
}

void Zn::VulkanDevice::CleanupSwapChain()
{
	for (size_t Index = 0; Index < m_VkFramebuffers.size(); ++Index)
	{
		vkDestroyFramebuffer(device, m_VkFramebuffers[Index], nullptr);
		vkDestroyImageView(device, m_VkImageViews[Index], nullptr);
	}

	vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void Zn::VulkanDevice::RecreateSwapChain()
{	
	vkDeviceWaitIdle(device);

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
	lighting.directional_lights[0].intensity = 0.25f;
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

		static const auto identity = glm::mat4{ 1.f };
		
		glm::mat4 translation = glm::translate(identity, object.location);
		glm::mat4 rotation = glm::mat4_cast(object.rotation);
		glm::mat4 scale = glm::scale(identity, object.scale);

		glm::mat4 transform = translation * rotation * scale;

		// only bind the pipeline if it doesn't match what's already bound
		if (object.material != last_material)
		{
			vkCmdBindPipeline(InCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
			last_material = object.material;

			vkCmdBindDescriptorSets(InCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->layout, 0, 1, &m_VkGlobalDescriptorSet[m_CurrentFrame], 0, nullptr);

			if (object.material->textureSet != VK_NULL_HANDLE)
			{
				vkCmdBindDescriptorSets(InCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->layout, 1, 1, &object.material->textureSet, 0, nullptr);
			}
		}

		Vk::MeshPushConstants constants
		{
			.RenderMatrix = transform
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
	Vk::RenderObject viking_room
	{
		.mesh = GetMesh("viking_room"),
		.material = VulkanMaterialManager::get().get_material("default"),
		.location = glm::vec3(0.f),
		.rotation = glm::quat(),
		.scale = glm::vec3(1.f)
	};

	// Sampler

	VkDescriptorSetAllocateInfo singleTextureAllocateInfo{};
	singleTextureAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	singleTextureAllocateInfo.descriptorPool = m_VkDescriptorPool;
	singleTextureAllocateInfo.descriptorSetCount = 1;
	singleTextureAllocateInfo.pSetLayouts = &singleTextureSetLayout;

	ZN_VK_CHECK(vkAllocateDescriptorSets(device, &singleTextureAllocateInfo, &viking_room.material->textureSet));

	const vk::Filter samplerFilters = vk::Filter::eNearest;
	const vk::SamplerAddressMode samplerAddressMode = vk::SamplerAddressMode::eRepeat;

	vk::SamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.magFilter = samplerFilters;
	samplerCreateInfo.minFilter = samplerFilters;
	samplerCreateInfo.addressModeU = samplerAddressMode;
	samplerCreateInfo.addressModeV = samplerAddressMode;
	samplerCreateInfo.addressModeW = samplerAddressMode;

	VkSampler sampler = device.createSampler(samplerCreateInfo);
	m_DestroyQueue.Enqueue([=]()
	{
		device.destroySampler(sampler);
	});

	viking_room.material->texture_samplers["default"] = sampler;

	//write to the descriptor set so that it points to our empire_diffuse texture
	VkDescriptorImageInfo imageBufferInfo;
	imageBufferInfo.sampler = sampler;
	imageBufferInfo.imageView = textures["viking_room"].imageView;
	imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet imageWrite{};
	imageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	imageWrite.pNext = nullptr;
	imageWrite.dstBinding = 0;
	imageWrite.dstSet = viking_room.material->textureSet;
	imageWrite.descriptorCount = 1;
	imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	imageWrite.pImageInfo = &imageBufferInfo;

	vkUpdateDescriptorSets(device, 1, &imageWrite, 0, nullptr);

	m_Renderables.push_back(viking_room);

	Vk::RenderObject monkey
	{
		.mesh = GetMesh("monkey"),
		.material = VulkanMaterialManager::get().get_material("default"),
		.location = glm::vec3(0.f, 0.f, 10.f),
		.rotation = glm::quat(),
		.scale = glm::vec3(1.f)
	};

	m_Renderables.push_back(monkey);

	for (int32 x = -20; x <= 20; ++x)
	{
		for (int32 y = -20; y <= 20; ++y)
		{
			Vk::RenderObject triangle
			{
				.mesh = GetMesh("triangle"),
				.material = VulkanMaterialManager::get().get_material("default"),
				.location = glm::vec3(x, 0, y),
				.rotation = glm::quat(glm::vec3(glm::radians(25.f), 0.f, 0.f)),
				.scale = glm::vec3(0.2f)
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

	triangle.Vertices[0].UV = glm::vec2(1.0f);
	triangle.Vertices[1].UV = glm::vec2(0.0, 1.f);
	triangle.Vertices[2].UV = glm::vec2(0.0f);

	//we don't care about the vertex normals

	UploadMesh(triangle);

	m_Meshes["triangle"] = triangle;

	Vk::Mesh monkey{};
	
	Vk::Obj::LoadMesh(IO::GetAbsolutePath("assets/VulkanGuide/monkey_smooth.obj"), monkey);
	
	UploadMesh(monkey);
	
	m_Meshes["monkey"] = monkey;

	Vk::Mesh vikingRoom{};

	Vk::Obj::LoadMesh(IO::GetAbsolutePath("assets/VulkanTutorial/viking_room.obj"), vikingRoom);
	 
	UploadMesh(vikingRoom);
	 
	m_Meshes["viking_room"] = vikingRoom;
}

void Zn::VulkanDevice::UploadMesh(Vk::Mesh & OutMesh)
{
	//this is the total size, in bytes, of the buffer we are allocating
	const u64 allocationSize = OutMesh.Vertices.size() * sizeof(Vk::Vertex);

	const VkBufferUsageFlags stagingUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	const VmaMemoryUsage stagingMemoryUsage = VMA_MEMORY_USAGE_CPU_ONLY;

	Vk::AllocatedBuffer stagingBuffer = CreateBuffer(allocationSize, stagingUsage, stagingMemoryUsage);

	CopyToGPU(stagingBuffer.Allocation, OutMesh.Vertices.data(), allocationSize);
	
	const VkBufferUsageFlags meshUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	const VmaMemoryUsage meshMemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;

	OutMesh.Buffer = CreateBuffer(allocationSize, meshUsage, meshMemoryUsage);

	ImmediateSubmit([=](VkCommandBuffer cmd)
	{
		CopyBuffer(cmd, stagingBuffer.Buffer, OutMesh.Buffer.Buffer, allocationSize);
	});

	DestroyBuffer(stagingBuffer);

	m_DestroyQueue.Enqueue([=]()
	{
		DestroyBuffer(OutMesh.Buffer);
	});
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

		vk::DescriptorSetLayout layouts[2] =
		{
			m_VkGlobalSetLayout,
			singleTextureSetLayout
		};

		vk::PipelineLayoutCreateInfo layoutCreateInfo({}, layouts);

		vk::PushConstantRange pushConstants{};
		pushConstants.offset = 0;
		pushConstants.size = sizeof(Vk::MeshPushConstants);
		//this push constant range is accessible only in the vertex shader
		pushConstants.stageFlags = vk::ShaderStageFlagBits::eVertex;


		layoutCreateInfo.setPushConstantRanges(pushConstants);

		material->layout = device.createPipelineLayout(layoutCreateInfo);

		m_DestroyQueue.Enqueue([=]()
		{
			device.destroyPipelineLayout(material->layout);
		});

		material->pipeline = VulkanPipeline::NewVkPipeline(device, m_VkRenderPass, material->vertex_shader, material->fragment_shader, m_VkSwapChainExtent, material->layout, vertex_description);

		m_DestroyQueue.Enqueue([=]()
		{
			VkDestroy(material->vertex_shader, device, vkDestroyShaderModule);
			VkDestroy(material->fragment_shader, device, vkDestroyShaderModule);
			VkDestroy(material->pipeline, device, vkDestroyPipeline);
		});
	}
}

Vk::AllocatedImage Zn::VulkanDevice::CreateTexture(const String& texture)
{
	u32 textureWidth = 0;
	u32 textureHeight = 0;
	u32 textureSize = 0;

	Vk::AllocatedBuffer stagingBuffer{};

	{
		Vk::RawTexture rawTexture{};

		if (!Vk::RawTexture::LoadFromFile(texture, rawTexture))
		{
			ZN_LOG(LogVulkan, ELogVerbosity::Warning, "Failed to load texture %s", texture.c_str());

			return Vk::AllocatedImage{};
		}

		textureWidth = static_cast<u32>(rawTexture.width);
		textureHeight = static_cast<u32>(rawTexture.height);
		textureSize = static_cast<u32>(rawTexture.size);

		stagingBuffer = CreateBuffer(textureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

		CopyToGPU(stagingBuffer.Allocation, rawTexture.data, rawTexture.size);

		Vk::RawTexture::Unload(rawTexture);
	}

	Vk::AllocatedImage outResult = CreateTextureImage(textureWidth, textureHeight, stagingBuffer);

	ImmediateSubmit([=](VkCommandBuffer cmd)
	{
		TransitionImageLayout(cmd, outResult.Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(cmd, stagingBuffer.Buffer, outResult.Image, textureWidth, textureHeight);
		TransitionImageLayout(cmd, outResult.Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	});

	DestroyBuffer(stagingBuffer);

	VkImageViewCreateInfo imageViewInfo{};
	imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewInfo.pNext = nullptr;

	imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewInfo.image = outResult.Image;
	imageViewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageViewInfo.subresourceRange.baseMipLevel = 0;
	imageViewInfo.subresourceRange.levelCount = 1;
	imageViewInfo.subresourceRange.baseArrayLayer = 0;
	imageViewInfo.subresourceRange.layerCount = 1;
	imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	ZN_VK_CHECK(vkCreateImageView(device, &imageViewInfo, nullptr, &outResult.imageView));

	return outResult;
}

Vk::AllocatedBuffer Zn::VulkanDevice::create_staging_texture(const Vk::RawTexture& inRawTexture)
{
	if (inRawTexture.data)
	{
		VkBufferUsageFlags texture_usage_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		Vk::AllocatedBuffer buffer = CreateBuffer(inRawTexture.size, texture_usage_flags, VMA_MEMORY_USAGE_CPU_ONLY);

		CopyToGPU(buffer.Allocation, inRawTexture.data, inRawTexture.size);

		return buffer;
	}

	return Vk::AllocatedBuffer{};
}

Vk::AllocatedImage Zn::VulkanDevice::CreateTextureImage(u32 width, u32 height, const Vk::AllocatedBuffer& inStagingTexture)
{
	Vk::AllocatedImage outImage{};

	VkImageCreateInfo createInfo = Vk::AllocatedImage::GetImageCreateInfo(
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VkExtent3D
		{ 
			width, 
			height,
			1 
		});

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

void Zn::VulkanDevice::TransitionImageLayout(VkCommandBuffer cmd, VkImage img, VkFormat fmt, VkImageLayout prevLayout, VkImageLayout newLayout)
{
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

	vkCmdPipelineBarrier(cmd,
						 srcStage, dstStage,
						 0,
						 0, nullptr,
						 0, nullptr,
						 1, &barrier);
}

VkCommandBufferBeginInfo Zn::VulkanDevice::CreateCmdBufferBeginInfo(VkCommandBufferUsageFlags flags)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = flags;
	beginInfo.pInheritanceInfo = nullptr;
	beginInfo.pNext = nullptr;

	return beginInfo;
}

VkSubmitInfo Zn::VulkanDevice::CreateCmdBufferSubmitInfo(VkCommandBuffer* cmdBuffer)
{
	VkSubmitInfo submitInfo{};
	
	memset(&submitInfo, 0, sizeof(submitInfo));

	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = cmdBuffer;

	return submitInfo;
}

void Zn::VulkanDevice::ImmediateSubmit(std::function<void(VkCommandBuffer)>&& function)
{
	if (function == nullptr)
	{
		ZN_LOG(LogVulkan, ELogVerbosity::Warning, "Trying to ImmediateSubmit with invalid function");
		return;
	}

	VkCommandBuffer cmd = uploadContext.cmdBuffer;

	VkCommandBufferBeginInfo beginInfo = CreateCmdBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	ZN_VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

	function(cmd);

	ZN_VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submitInfo = CreateCmdBufferSubmitInfo(&cmd);

	// .fence will now block until the graphic commands finish execution
	ZN_VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, uploadContext.fence));

	ZN_VK_CHECK(vkWaitForFences(device, 1, &uploadContext.fence, true, kWaitTimeOneSecond));
	ZN_VK_CHECK(vkResetFences(device, 1, &uploadContext.fence));

	// reset the command buffers inside the command pool
	ZN_VK_CHECK(vkResetCommandPool(device, uploadContext.commandPool, 0));
}

void Zn::VulkanDevice::CopyBuffer(VkCommandBuffer cmd, VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
	VkBufferCopy region{};
	region.size = size;
	vkCmdCopyBuffer(cmd, src, dst, 1, &region);
}

void Zn::VulkanDevice::CopyBufferToImage(VkCommandBuffer cmd, VkBuffer buffer, VkImage img, u32 width, u32 height)
{
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

	vkCmdCopyBufferToImage(cmd, buffer, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
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

	m_DestroyQueue.Enqueue([this, OutObject, Owner, Destructor = std::move(Destroy)]() mutable
	{
		if (Owner != VK_NULL_HANDLE && OutObject != VK_NULL_HANDLE)
		{
			VkDestroy(OutObject, Owner, Destructor);
		}
	});
}