#include <Znpch.h>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
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

	windowID = SDL_GetWindowID(InWindowHandle);

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
	vma::AllocatorCreateInfo allocatorCreateInfo;
	allocatorCreateInfo.instance = instance;
	allocatorCreateInfo.physicalDevice = gpu;
	allocatorCreateInfo.device = device;

	allocator = vma::createAllocator(allocatorCreateInfo);

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

	destroyQueue.Enqueue([=]()
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

	destroyQueue.Enqueue([=]()
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

	destroyQueue.Enqueue([=]()
	{
		device.destroyFence(uploadContext.fence);
	});

	////// Render Pass

	//	Color Attachment

	// the renderpass will use this color attachment.
	vk::AttachmentDescription colorAttachmentDesc = {};
	//the attachment will have the format needed by the swapchain
	colorAttachmentDesc.format = swapChainFormat.format;
	//1 sample, we won't be doing MSAA
	colorAttachmentDesc.samples = vk::SampleCountFlagBits::e1;
	// we Clear when this attachment is loaded
	colorAttachmentDesc.loadOp = vk::AttachmentLoadOp::eClear;
	// we keep the attachment stored when the renderpass ends
	colorAttachmentDesc.storeOp = vk::AttachmentStoreOp::eStore;
	//we don't care about stencil
	colorAttachmentDesc.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachmentDesc.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	//we don't know or care about the starting layout of the attachment
	colorAttachmentDesc.initialLayout = vk::ImageLayout::eUndefined;
	//after the renderpass ends, the image has to be on a layout ready for display
	colorAttachmentDesc.finalLayout = vk::ImageLayout::ePresentSrcKHR;

	vk::AttachmentReference colorAttachmentRef{};
	//attachment number will index into the pAttachments array in the parent renderpass itself
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	//	Depth Attachment

	vk::AttachmentDescription depthAttachmentDesc{};

	//	Depth attachment
	//	Both the depth attachment and its reference are copypaste of the color one, as it works the same, but with a small change:
	//	.format = m_DepthFormat; is set to the depth format that we created the depth image at.
	//	.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; 
	depthAttachmentDesc.flags = vk::AttachmentDescriptionFlags(0);
	// TODO
	depthAttachmentDesc.format = depthImageFormat;
	depthAttachmentDesc.samples = vk::SampleCountFlagBits::e1;
	depthAttachmentDesc.loadOp = vk::AttachmentLoadOp::eClear;
	depthAttachmentDesc.storeOp = vk::AttachmentStoreOp::eStore;
	depthAttachmentDesc.stencilLoadOp = vk::AttachmentLoadOp::eClear;
	depthAttachmentDesc.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	depthAttachmentDesc.initialLayout = vk::ImageLayout::eUndefined;
	depthAttachmentDesc.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::AttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	// Main Subpass

	//we are going to create 1 subpass, which is the minimum you can do
	vk::SubpassDescription subpassDesc = {};
	subpassDesc.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpassDesc.pColorAttachments = &colorAttachmentRef;
	subpassDesc.colorAttachmentCount = 1;
	subpassDesc.pDepthStencilAttachment = &depthAttachmentRef;

	vk::RenderPassCreateInfo renderPassCreateInfo{};

	vk::AttachmentDescription attachments[2] = { colorAttachmentDesc, depthAttachmentDesc };
	//connect the color attachment to the info
	renderPassCreateInfo.setAttachments(attachments);
	//connect the subpass to the info
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDesc;

	/*
	* https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation | Subpass dependencies
	*/
	vk::SubpassDependency colorDependency{};
	colorDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	colorDependency.dstSubpass = 0;

	colorDependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	colorDependency.srcAccessMask = vk::AccessFlags(0);

	colorDependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	colorDependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

	//	Add a new dependency that synchronizes access to depth attachments.
	//	Without this multiple frames can be rendered simultaneously by the GPU.

	vk::SubpassDependency depthDependency{};
	depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	depthDependency.dstSubpass = 0;
	
	depthDependency.srcStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
	depthDependency.srcAccessMask = vk::AccessFlags(0);

	depthDependency.dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
	depthDependency.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;

	vk::SubpassDependency dependencies[2] = { colorDependency, depthDependency };

	renderPassCreateInfo.setDependencies(dependencies);

	renderPass = device.createRenderPass(renderPassCreateInfo);

	destroyQueue.Enqueue([=]()
	{
		device.destroyRenderPass(renderPass);
	});

	/////// Frame Buffers

	CreateFramebuffers();

	////// Sync Structures

	vk::FenceCreateInfo FenceCreateInfo{};
	//we want to create the fence with the Create Signaled flag, so we can wait on it before using it on a GPU command (for the first frame)
	FenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;

	for (size_t Index = 0; Index < kMaxFramesInFlight; ++Index)
	{
		renderFences[Index] = device.createFence(FenceCreateInfo);
		destroyQueue.Enqueue([=]()
		{
			device.destroyFence(renderFences[Index]);
		});
	}

	//for the semaphores we don't need any flags
	vk::SemaphoreCreateInfo semaphoreCreateInfo{};

	for (size_t Index = 0; Index < kMaxFramesInFlight; ++Index)
	{
		presentSemaphores[Index] = device.createSemaphore(semaphoreCreateInfo);
		renderSemaphores[Index] = device.createSemaphore(semaphoreCreateInfo);

		destroyQueue.Enqueue([=]()
		{
			device.destroySemaphore(presentSemaphores[Index]);
			device.destroySemaphore(renderSemaphores[Index]);
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

		imguiDescriptorPool = device.createDescriptorPool(imGuiPoolCreateInfo);

		destroyQueue.Enqueue([=]()
		{
			device.destroyDescriptorPool(imguiDescriptorPool);
		});

		//this initializes imgui for Vulkan
		ImGui_ImplVulkan_InitInfo ImGuiInitInfo{};
		ImGuiInitInfo.Instance = instance;
		ImGuiInitInfo.PhysicalDevice = gpu;
		ImGuiInitInfo.Device = device;
		ImGuiInitInfo.Queue = graphicsQueue;
		ImGuiInitInfo.DescriptorPool = imguiDescriptorPool;
		ImGuiInitInfo.MinImageCount = 3;
		ImGuiInitInfo.ImageCount = 3;
		ImGuiInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&ImGuiInitInfo, renderPass);

		// Upload Fonts

		vk::CommandBuffer commandBuffer = commandBuffers[0];
		commandBuffer.reset();


		vk::CommandBufferBeginInfo commandBufferBeginInfo{};
		commandBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

		commandBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

		commandBuffer.end();

		vk::CommandBuffer commandBuffers[1] = { commandBuffer };
		graphicsQueue.submit(vk::SubmitInfo({}, {}, commandBuffers, {}));

		device.waitIdle();

		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	LoadMeshes();

	{
		textures["texture"] = CreateTexture(IO::GetAbsolutePath("assets/texture.jpg"));
		textures["viking_room"] = CreateTexture(IO::GetAbsolutePath("assets/VulkanTutorial/viking_room.png"));
	}

	CreateMeshPipeline();
	 
	CreateScene();

	isInitialized = true;
}

void VulkanDevice::Cleanup()
{
	if (isInitialized == false)
	{
		return;
	}

	ZN_VK_CHECK(vkDeviceWaitIdle(device));

	CleanupSwapChain();

	for (auto& textureKvp : textures)
	{
		vkDestroyImageView(device, textureKvp.second.imageView, nullptr);
		allocator.destroyImage(textureKvp.second.image, textureKvp.second.allocation);
	}

	textures.clear();

	meshes.clear();

	destroyQueue.Flush();	

	ImGui_ImplVulkan_Shutdown();

	for (size_t Index = 0; Index < kMaxFramesInFlight; ++Index)
	{
		commandBuffers[Index] = VK_NULL_HANDLE;
	}

	uploadContext.cmdBuffer = VK_NULL_HANDLE;

	frameBuffers.clear();
	swapChainImageViews.clear();

	allocator.destroy();

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

	isInitialized = false;
}

void Zn::VulkanDevice::BeginFrame()
{
	//wait until the GPU has finished rendering the last frame. Timeout of 1 second
	
	device.waitForFences({ renderFences[currentFrame] }, true, kWaitTimeOneSecond);

	if (isMinimized)
	{
		return;
	}

	static constexpr auto kSwapChainWaitTime = 1000000000;
	//request image from the swapchain, one second timeout
	//m_VkPresentSemaphore is set to make sure that we can sync other operations with the swapchain having an image ready to render.
	vk::Result acquireImageResult = device.acquireNextImageKHR(swapChain, kSwapChainWaitTime, presentSemaphores[currentFrame], nullptr/*fence*/, &swapChainImageIndex);

	if (acquireImageResult == vk::Result::eErrorOutOfDateKHR)
	{
		RecreateSwapChain();
		return;
	}
	else if (acquireImageResult != vk::Result::eSuccess && acquireImageResult != vk::Result::eSuboptimalKHR)
	{
		_ASSERT(false);
		return;
	}

	device.resetFences({ renderFences[currentFrame] });

	//now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
	commandBuffers[currentFrame].reset();
}

void VulkanDevice::Draw()
{
	if (isMinimized)
	{
		return;
	}

	// Build ImGui render commands
	ImGui::Render();

	//naming it commandBuffer for shorter writing
	vk::CommandBuffer commandBuffer = commandBuffers[currentFrame];

	//begin the command buffer recording. We will use this command buffer exactly once, so we want to let Vulkan know that
	VkCommandBufferBeginInfo CmdBufferBeginInfo{};
	CmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	CmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	commandBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

	vk::ClearValue clearColor(vk::ClearColorValue(1.0f, 1.0f, 1.0f, 1.0f));

	vk::ClearValue depthClear;
	
	//	make a clear-color from frame number. This will flash with a 120 frame period.
	depthClear.color = vk::ClearColorValue{ 0.0f, 0.0f, abs(sin(frameNumber / 120.f)), 1.0f };
	depthClear.depthStencil.depth = 1.f;

	//start the main renderpass.
	//We will use the clear color from above, and the framebuffer of the index the swapchain gave us
	vk::RenderPassBeginInfo renderPassBeginInfo{};

	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent = swapChainExtent;
	renderPassBeginInfo.framebuffer = frameBuffers[swapChainImageIndex];

	//	Connect clear values
	
	vk::ClearValue clearValues[2] = { clearColor, depthClear };
	renderPassBeginInfo.setClearValues(clearValues);

	commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

	// *** Insert Commands here ***	
	{
		// Model View Matrix

		// Camera View	
		const glm::mat4 view = glm::lookAt(cameraPosition, cameraPosition + cameraDirection, upVector);

		// Camera Projection
		glm::mat4 projection = glm::perspective(glm::radians(60.f), 16.f / 9.f, 0.1f, 200.f);
		projection[1][1] *= -1;

		Vk::GPUCameraData camera{};
		camera.projection = projection;
		camera.view = view;
		camera.view_projection = projection * view;

		CopyToGPU(cameraBuffer[currentFrame].Allocation, &camera, sizeof(Vk::GPUCameraData));

		Vk::LightingUniforms lighting{};
		lighting.directional_lights[0].direction = glm::vec4(glm::normalize(glm::mat3(camera.view_projection) * glm::vec3(0.f, -1.f, 0.0f)), 0.0f);
		lighting.directional_lights[0].color = glm::vec4(1.0f, 0.8f, 0.8f, 0.f);
		lighting.directional_lights[0].intensity = 0.25f;
		lighting.ambient_light.color = glm::vec4(0.f, 0.2f, 1.f, 0.f);
		lighting.ambient_light.intensity = 0.15f;
		lighting.num_directional_lights = 1;
		lighting.num_point_lights = 0;

		CopyToGPU(lightingBuffer[currentFrame].Allocation, &lighting, sizeof(Vk::LightingUniforms));

		DrawObjects(commandBuffer, renderables.data(), renderables.size());

		// Enqueue ImGui commands to CmdBuffer
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	}

	//finalize the render pass
	commandBuffer.endRenderPass();
	//finalize the command buffer (we can no longer add commands, but it can now be executed)
	commandBuffer.end();
}

void Zn::VulkanDevice::EndFrame()
{
	if (isMinimized)
	{
		return;
	}

	auto& CmdBuffer = commandBuffers[currentFrame];
	////// Submit

	//prepare the submission to the queue.
	//we want to wait on the m_VkPresentSemaphore, as that semaphore is signaled when the swapchain is ready
	//we will signal the m_VkRenderSemaphore, to signal that rendering has finished

	vk::Semaphore waitSemaphores[] = { presentSemaphores[currentFrame] };
	vk::Semaphore signalSemaphores[] = { renderSemaphores[currentFrame] };

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
	graphicsQueue.submit(submitInfo, renderFences[currentFrame]);

	////// Present

	// this will put the image we just rendered into the visible window.
	// we want to wait on the _renderSemaphore for that,
	// as it's necessary that drawing commands have finished before the image is displayed to the user
	vk::PresentInfoKHR presentInfo = {};

	presentInfo.pSwapchains = &swapChain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &swapChainImageIndex;
	
	vk::Result presentResult = presentQueue.presentKHR(presentInfo);

	if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR)
	{
		RecreateSwapChain();
	}
	else
	{
		_ASSERT(presentResult == vk::Result::eSuccess);
	}

	currentFrame = (currentFrame + 1) % kMaxFramesInFlight;
	frameNumber++;
}

void Zn::VulkanDevice::ResizeWindow()
{
	RecreateSwapChain();
}

void Zn::VulkanDevice::OnWindowMinimized()
{
	isMinimized = true;
}

void Zn::VulkanDevice::OnWindowRestored()
{
	isMinimized = false;
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

vk::ShaderModule Zn::VulkanDevice::CreateShaderModule(const Vector<uint8>& bytes)
{
	vk::ShaderModuleCreateInfo shaderCreateInfo{};
	shaderCreateInfo.codeSize = bytes.size();
	shaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(bytes.data());

	return device.createShaderModule(shaderCreateInfo);
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

	descriptorPool = device.createDescriptorPool(poolCreateInfo);
	
	destroyQueue.Enqueue([=]()
	{
		device.destroyDescriptorPool(descriptorPool);
	});

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

	globalDescriptorSetLayout = device.createDescriptorSetLayout(globalSetCreateInfo);

	destroyQueue.Enqueue([=]()
	{
		device.destroyDescriptorSetLayout(globalDescriptorSetLayout);
	});

	// Single Texture Set

	vk::DescriptorSetLayoutBinding singleTextureSetBinding{};
	singleTextureSetBinding.binding = 0;
	singleTextureSetBinding.descriptorCount = 1;
	singleTextureSetBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	singleTextureSetBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

	vk::DescriptorSetLayoutCreateInfo singleTextureSetCreateInfo({}, singleTextureSetBinding);

	singleTextureSetLayout = device.createDescriptorSetLayout(singleTextureSetCreateInfo);

	destroyQueue.Enqueue([=]()
	{
		device.destroyDescriptorSetLayout(singleTextureSetLayout);
	});

	globalDescriptorSets.resize(kMaxFramesInFlight);

	for (size_t Index = 0; Index < kMaxFramesInFlight; ++Index)
	{
		cameraBuffer[Index] = CreateBuffer(sizeof(Vk::GPUCameraData), vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu);

		destroyQueue.Enqueue([=]()
		{
			DestroyBuffer(cameraBuffer[Index]);
		});		

		// Lighting

		lightingBuffer[Index] = CreateBuffer(sizeof(Vk::LightingUniforms), vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu);

		destroyQueue.Enqueue([=]()
		{
			DestroyBuffer(lightingBuffer[Index]);
		});

		vk::DescriptorSetAllocateInfo descriptorSetAllocInfo{};
		descriptorSetAllocInfo.descriptorPool = descriptorPool;
		descriptorSetAllocInfo.descriptorSetCount = 1;
		descriptorSetAllocInfo.pSetLayouts = &globalDescriptorSetLayout;

		globalDescriptorSets[Index] = device.allocateDescriptorSets(descriptorSetAllocInfo)[0];

		Vector<vk::DescriptorBufferInfo> bufferInfo =
		{
			vk::DescriptorBufferInfo
			{
				cameraBuffer[Index].Buffer,
				0,
				sizeof(Vk::GPUCameraData)
			},
			vk::DescriptorBufferInfo
			{
				lightingBuffer[Index].Buffer,
				0,
				sizeof(Vk::LightingUniforms)
			}
		};

		Vector<vk::WriteDescriptorSet> descriptorSetWrites =
		{
			vk::WriteDescriptorSet
			{
				globalDescriptorSets[Index],
				0,
				0,
				1,
				vk::DescriptorType::eUniformBuffer,
				nullptr,
				&bufferInfo[0],
				nullptr,
				nullptr
			},
			vk::WriteDescriptorSet
			{
				globalDescriptorSets[Index],
				1,
				0,
				1,
				vk::DescriptorType::eUniformBuffer,
				nullptr,
				&bufferInfo[1],
				nullptr,
				nullptr
			}
		};

		device.updateDescriptorSets(descriptorSetWrites, {});
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

	SDL_Window* WindowHandle = SDL_GetWindowFromID(windowID);
	int32 Width, Height = 0;
	SDL_Vulkan_GetDrawableSize(WindowHandle, &Width, &Height);

	Width = std::clamp(Width, static_cast<int32>(SwapChainDetails.Capabilities.minImageExtent.width), static_cast<int32>(SwapChainDetails.Capabilities.maxImageExtent.width));
	Height = std::clamp(Height, static_cast<int32>(SwapChainDetails.Capabilities.minImageExtent.height), static_cast<int32>(SwapChainDetails.Capabilities.maxImageExtent.height));

	uint32 ImageCount = SwapChainDetails.Capabilities.minImageCount + 1;
	if (SwapChainDetails.Capabilities.maxImageCount > 0)
	{
		ImageCount = std::min(ImageCount, SwapChainDetails.Capabilities.maxImageCount);
	}

	swapChainFormat = surfaceFormat;

	swapChainExtent = vk::Extent2D(Width, Height);

	vk::SwapchainCreateInfoKHR swapChainCreateInfo{};
	swapChainCreateInfo.surface = surface;
	swapChainCreateInfo.minImageCount = ImageCount;
	swapChainCreateInfo.imageFormat = swapChainFormat.format;
	swapChainCreateInfo.imageColorSpace = swapChainFormat.colorSpace;
	swapChainCreateInfo.imageExtent = swapChainExtent;
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
	swapChainImages = device.getSwapchainImagesKHR(swapChain);

	swapChainImageViews.resize(swapChainImages.size());

	for (size_t Index = 0; Index < swapChainImages.size(); ++Index)
	{
		vk::ImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.image = swapChainImages[Index];
		imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
		imageViewCreateInfo.format = swapChainFormat.format;

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

		swapChainImageViews[Index] = device.createImageView(imageViewCreateInfo);
	}

	// Initialize Depth Buffer

	vk::Extent3D depthImageExtent
	{ 
		swapChainExtent.width, 
		swapChainExtent.height, 
		1 
	};

	//	Hardcoding to 32 bit float.
	//	Most GPUs support this depth format, so it’s fine to use it. You might want to choose other formats for other uses, or if you use Stencil buffer.
	depthImageFormat = vk::Format::eD32Sfloat;

	//	VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT lets the vulkan driver know that this will be a depth image used for z-testing.
	vk::ImageCreateInfo depthImageCreateInfo = Vk::AllocatedImage::GetImageCreateInfo(depthImageFormat, vk::ImageUsageFlagBits::eDepthStencilAttachment, depthImageExtent);

	//	Allocate from GPU memory.

	vma::AllocationCreateInfo depthImageAllocInfo{};
	//	VMA_MEMORY_USAGE_GPU_ONLY to make sure that the image is allocated on fast VRAM.
	depthImageAllocInfo.usage = vma::MemoryUsage::eGpuOnly;
	//	To make absolutely sure that VMA really allocates the image into VRAM, we give it VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT on required flags. 
	//	This forces VMA library to allocate the image on VRAM no matter what. (The Memory Usage part is more like a hint)
	depthImageAllocInfo.requiredFlags = vk::MemoryPropertyFlags(vk::MemoryPropertyFlagBits::eDeviceLocal);

	// TODO: VK_CHECK
	allocator.createImage(&depthImageCreateInfo, &depthImageAllocInfo, &depthImage.image, &depthImage.allocation, nullptr);

	//	VK_IMAGE_ASPECT_DEPTH_BIT lets the vulkan driver know that this will be a depth image used for z-testing.
	vk::ImageViewCreateInfo depthImageViewCreateInfo = Vk::AllocatedImage::GetImageViewCreateInfo(depthImageFormat, depthImage.image, vk::ImageAspectFlagBits::eDepth);

	depthImageView = device.createImageView(depthImageViewCreateInfo);

	destroyQueue.Enqueue([=]()
	{
		device.destroyImageView(depthImageView);
		allocator.destroyImage(depthImage.image, depthImage.allocation);
	});
}

void Zn::VulkanDevice::CreateFramebuffers()
{
	//create the framebuffers for the swapchain images. This will connect the render-pass to the images for rendering
	vk::FramebufferCreateInfo framebufferCreateInfo{};
	framebufferCreateInfo.renderPass = renderPass;	
	framebufferCreateInfo.attachmentCount = 1;
	framebufferCreateInfo.width = swapChainExtent.width;
	framebufferCreateInfo.height = swapChainExtent.height;
	framebufferCreateInfo.layers = 1;

	//grab how many images we have in the swapchain
	const size_t NumImages = swapChainImages.size();
	frameBuffers = Vector<vk::Framebuffer>(NumImages);

	//create framebuffers for each of the swapchain image views
	for (size_t index = 0; index < NumImages; index++)
	{
		vk::ImageView attachments[2] = { swapChainImageViews[index], depthImageView };

		framebufferCreateInfo.setAttachments(attachments);
		
		frameBuffers[index] = device.createFramebuffer(framebufferCreateInfo);
	}
}

void Zn::VulkanDevice::CleanupSwapChain()
{
	for (size_t index = 0; index < frameBuffers.size(); ++index)
	{
		device.destroyFramebuffer(frameBuffers[index]);
		device.destroyImageView(swapChainImageViews[index]);
	}

	device.destroySwapchainKHR(swapChain);
}

void Zn::VulkanDevice::RecreateSwapChain()
{	
	device.waitIdle();

	CleanupSwapChain();

	CreateSwapChain();

	CreateImageViews();

	CreateFramebuffers();
}

Vk::AllocatedBuffer Zn::VulkanDevice::CreateBuffer(size_t size, vk::BufferUsageFlags usage, vma::MemoryUsage memoryUsage)
{
	vk::BufferCreateInfo createInfo{};
	createInfo.size = size;
	createInfo.usage = usage;

	vma::AllocationCreateInfo allocationInfo{};
	allocationInfo.usage = memoryUsage;
	allocationInfo.requiredFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

	Vk::AllocatedBuffer OutBuffer{};

	auto bufferAllocationPair = allocator.createBuffer(createInfo, allocationInfo);

	OutBuffer.Buffer = bufferAllocationPair.first;
	OutBuffer.Allocation = bufferAllocationPair.second;

	return OutBuffer;
}

void Zn::VulkanDevice::DestroyBuffer(Vk::AllocatedBuffer buffer)
{
	allocator.destroyBuffer(buffer.Buffer, buffer.Allocation);
}

void Zn::VulkanDevice::CopyToGPU(vma::Allocation allocation, void* src, size_t size)
{
	void* dst = allocator.mapMemory(allocation);

	memcpy(dst, src, size);

	allocator.unmapMemory(allocation);
}

Vk::Mesh* Zn::VulkanDevice::GetMesh(const String& InName)
{
	if (auto it = meshes.find(InName); it != meshes.end())
	{
		return &((*it).second);
	}

	return nullptr;
}

void Zn::VulkanDevice::DrawObjects(vk::CommandBuffer commandBuffer, Vk::RenderObject* first, int32 count)
{
	Vk::Mesh* lastMesh = nullptr;
	Vk::Material* lastMaterial = nullptr;

	for (int32 index = 0; index < count; ++index)
	{
		Vk::RenderObject& object = first[index];

		static const auto identity = glm::mat4{ 1.f };
		
		glm::mat4 translation = glm::translate(identity, object.location);
		glm::mat4 rotation = glm::mat4_cast(object.rotation);
		glm::mat4 scale = glm::scale(identity, object.scale);

		glm::mat4 transform = translation * rotation * scale;

		// only bind the pipeline if it doesn't match what's already bound
		if (object.material != lastMaterial)
		{
			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, object.material->pipeline);

			lastMaterial = object.material;
			
			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, object.material->layout, 0, globalDescriptorSets[currentFrame], {});

			if (object.material->textureSet)
			{
				commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, object.material->layout, 1, object.material->textureSet, {});
			}
		}

		Vk::MeshPushConstants constants
		{
			.RenderMatrix = transform
		};

		commandBuffer.pushConstants(object.material->layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Vk::MeshPushConstants), &constants);

		// only bind the mesh if it's different from what's already bound

		if (object.mesh != lastMesh)
		{
			// bind the mesh v-buffer with offset 0
			vk::DeviceSize offset = 0;
			commandBuffer.bindVertexBuffers(0, 1, &object.mesh->Buffer.Buffer, &offset);
			lastMesh = object.mesh;
		}

		// Draw

		commandBuffer.draw(object.mesh->Vertices.size(), 1, 0, 0);
	}
}

void Zn::VulkanDevice::CreateScene()
{
	Vk::RenderObject viking_room
	{
		.mesh = GetMesh("viking_room"),
		.material = VulkanMaterialManager::Get().GetMaterial("default"),
		.location = glm::vec3(0.f),
		.rotation = glm::quat(),
		.scale = glm::vec3(1.f)
	};

	// Sampler

	vk::DescriptorSetAllocateInfo singleTextureAllocateInfo{};
	singleTextureAllocateInfo.descriptorPool = descriptorPool;
	singleTextureAllocateInfo.descriptorSetCount = 1;	
	singleTextureAllocateInfo.pSetLayouts = &singleTextureSetLayout;

	viking_room.material->textureSet = device.allocateDescriptorSets(singleTextureAllocateInfo)[0];

	const vk::Filter samplerFilters = vk::Filter::eNearest;
	const vk::SamplerAddressMode samplerAddressMode = vk::SamplerAddressMode::eRepeat;

	vk::SamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.magFilter = samplerFilters;
	samplerCreateInfo.minFilter = samplerFilters;
	samplerCreateInfo.addressModeU = samplerAddressMode;
	samplerCreateInfo.addressModeV = samplerAddressMode;
	samplerCreateInfo.addressModeW = samplerAddressMode;

	vk::Sampler sampler = device.createSampler(samplerCreateInfo);
	destroyQueue.Enqueue([=]()
	{
		device.destroySampler(sampler);
	});

	viking_room.material->textureSamplers["default"] = sampler;

	//write to the descriptor set so that it points to our empire_diffuse texture
	vk::DescriptorImageInfo imageBufferInfo;
	imageBufferInfo.sampler = sampler;
	imageBufferInfo.imageView = textures["viking_room"].imageView;
	imageBufferInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

	device.updateDescriptorSets(
		{
			vk::WriteDescriptorSet
			{
				viking_room.material->textureSet,
				0,
				0,
				1,
				vk::DescriptorType::eCombinedImageSampler,
				&imageBufferInfo
			} 
		}, {});

	renderables.push_back(viking_room);

	Vk::RenderObject monkey
	{
		.mesh = GetMesh("monkey"),
		.material = VulkanMaterialManager::Get().GetMaterial("default"),
		.location = glm::vec3(0.f, 0.f, 10.f),
		.rotation = glm::quat(),
		.scale = glm::vec3(1.f)
	};

	renderables.push_back(monkey);

	for (int32 x = -20; x <= 20; ++x)
	{
		for (int32 y = -20; y <= 20; ++y)
		{
			Vk::RenderObject triangle
			{
				.mesh = GetMesh("triangle"),
				.material = VulkanMaterialManager::Get().GetMaterial("default"),
				.location = glm::vec3(x, 0, y),
				.rotation = glm::quat(glm::vec3(glm::radians(25.f), 0.f, 0.f)),
				.scale = glm::vec3(0.2f)
			};

			renderables.push_back(triangle);
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

	meshes["triangle"] = triangle;

	Vk::Mesh monkey{};
	
	Vk::Obj::LoadMesh(IO::GetAbsolutePath("assets/VulkanGuide/monkey_smooth.obj"), monkey);
	
	UploadMesh(monkey);
	
	meshes["monkey"] = monkey;

	Vk::Mesh vikingRoom{};

	Vk::Obj::LoadMesh(IO::GetAbsolutePath("assets/VulkanTutorial/viking_room.obj"), vikingRoom);
	 
	UploadMesh(vikingRoom);
	 
	meshes["viking_room"] = vikingRoom;
}

void Zn::VulkanDevice::UploadMesh(Vk::Mesh & OutMesh)
{
	//this is the total size, in bytes, of the buffer we are allocating
	const u64 allocationSize = OutMesh.Vertices.size() * sizeof(Vk::Vertex);

	const vk::BufferUsageFlags stagingUsage = vk::BufferUsageFlagBits::eTransferSrc;
	const vma::MemoryUsage stagingMemoryUsage = vma::MemoryUsage::eCpuOnly;

	Vk::AllocatedBuffer stagingBuffer = CreateBuffer(allocationSize, stagingUsage, stagingMemoryUsage);

	CopyToGPU(stagingBuffer.Allocation, OutMesh.Vertices.data(), allocationSize);
	
	const vk::BufferUsageFlags meshUsage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
	const vma::MemoryUsage meshMemoryUsage = vma::MemoryUsage::eGpuOnly;

	OutMesh.Buffer = CreateBuffer(allocationSize, meshUsage, meshMemoryUsage);

	ImmediateSubmit([=](vk::CommandBuffer cmd)
	{
		cmd.copyBuffer(stagingBuffer.Buffer, OutMesh.Buffer.Buffer, { vk::BufferCopy(0, 0, allocationSize) });
	});

	DestroyBuffer(stagingBuffer);

	destroyQueue.Enqueue([=]()
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
		Vk::Material* material = VulkanMaterialManager::Get().CreateMaterial("default");

		material->vertexShader = CreateShaderModule(vertex_shader_data);
		material->fragmentShader = CreateShaderModule(fragment_shader_data); 
		
		if (!material->vertexShader)
		{
			ZN_LOG(LogVulkan, ELogVerbosity::Error, "Failed to create vertex shader.");
		}

		if (!material->fragmentShader)
		{
			ZN_LOG(LogVulkan, ELogVerbosity::Error, "Failed to create fragment shader.");
		}

		Vk::VertexInputDescription vertex_description = Vk::Vertex::GetVertexInputDescription();

		// Mesh pipeline with push constants 

		vk::DescriptorSetLayout layouts[2] =
		{
			globalDescriptorSetLayout,
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

		destroyQueue.Enqueue([=]()
		{
			device.destroyPipelineLayout(material->layout);
		});

		material->pipeline = VulkanPipeline::NewVkPipeline(device, renderPass, material->vertexShader, material->fragmentShader, swapChainExtent, material->layout, vertex_description);

		destroyQueue.Enqueue([=]()
		{
			device.destroyShaderModule(material->vertexShader);
			device.destroyShaderModule(material->fragmentShader);
			device.destroyPipeline(material->pipeline);
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

		stagingBuffer = CreateBuffer(textureSize, vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eCpuOnly);

		CopyToGPU(stagingBuffer.Allocation, rawTexture.data, rawTexture.size);

		Vk::RawTexture::Unload(rawTexture);
	}

	Vk::AllocatedImage outResult = CreateTextureImage(textureWidth, textureHeight, stagingBuffer);

	ImmediateSubmit([=](vk::CommandBuffer cmd)
	{
		TransitionImageLayout(cmd, outResult.image, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
		CopyBufferToImage(cmd, stagingBuffer.Buffer, outResult.image, textureWidth, textureHeight);
		TransitionImageLayout(cmd, outResult.image, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
	});

	DestroyBuffer(stagingBuffer);

	vk::ImageViewCreateInfo imageViewInfo{};

	imageViewInfo.viewType = vk::ImageViewType::e2D;
	imageViewInfo.image = outResult.image;
	imageViewInfo.format = vk::Format::eR8G8B8A8Srgb;
	imageViewInfo.subresourceRange.baseMipLevel = 0;
	imageViewInfo.subresourceRange.levelCount = 1;
	imageViewInfo.subresourceRange.baseArrayLayer = 0;
	imageViewInfo.subresourceRange.layerCount = 1;
	imageViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;

	outResult.imageView = device.createImageView(imageViewInfo);

	return outResult;
}

Vk::AllocatedImage Zn::VulkanDevice::CreateTextureImage(u32 width, u32 height, const Vk::AllocatedBuffer& inStagingTexture)
{
	Vk::AllocatedImage outImage{};

	vk::ImageCreateInfo createInfo = Vk::AllocatedImage::GetImageCreateInfo(
		vk::Format::eR8G8B8A8Srgb,		
		vk::ImageUsageFlagBits::eTransferDst| vk::ImageUsageFlagBits::eSampled,
		vk::Extent3D
		{ 
			width, 
			height,
			1 
		});

	createInfo.initialLayout = vk::ImageLayout::eUndefined;
	createInfo.sharingMode = vk::SharingMode::eExclusive;

	//	Allocate from GPU memory.

	vma::AllocationCreateInfo allocationInfo{};
	//	VMA_MEMORY_USAGE_GPU_ONLY to make sure that the image is allocated on fast VRAM.
	allocationInfo.usage = vma::MemoryUsage::eGpuOnly;
	//	To make absolutely sure that VMA really allocates the image into VRAM, we give it VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT on required flags. 
	//	This forces VMA library to allocate the image on VRAM no matter what. (The Memory Usage part is more like a hint)
	allocationInfo.requiredFlags = vk::MemoryPropertyFlags(vk::MemoryPropertyFlagBits::eDeviceLocal);
	
	allocator.createImage(&createInfo, &allocationInfo, &outImage.image, &outImage.allocation, nullptr);

	return outImage;
}

void Zn::VulkanDevice::TransitionImageLayout(vk::CommandBuffer cmd, vk::Image img, vk::Format fmt, vk::ImageLayout prevLayout, vk::ImageLayout newLayout)
{
	vk::ImageMemoryBarrier barrier{};

	barrier.oldLayout = prevLayout;
	barrier.newLayout = newLayout;

	// If you are using the barrier to transfer queue family ownership, then these two fields should be the indices of the queue families.
	// They must be set to VK_QUEUE_FAMILY_IGNORED if you don't want to do this (not the default value!).
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = img;
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	vk::PipelineStageFlags srcStage{};
	vk::PipelineStageFlags dstStage{};

	if (prevLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlags(0);
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
		dstStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (prevLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		srcStage = vk::PipelineStageFlagBits::eTransfer;
		dstStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else
	{
		_ASSERT(false); // Unsupported Transition
	}

	cmd.pipelineBarrier(srcStage, dstStage,
						vk::DependencyFlags(0),
						{}, {}, { barrier });
}

void Zn::VulkanDevice::ImmediateSubmit(std::function<void(vk::CommandBuffer)>&& function)
{
	if (function == nullptr)
	{
		ZN_LOG(LogVulkan, ELogVerbosity::Warning, "Trying to ImmediateSubmit with invalid function");
		return;
	}

	vk::CommandBuffer cmd = uploadContext.cmdBuffer;

	cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

	function(cmd);

	cmd.end();

	vk::SubmitInfo submitInfo{};
	submitInfo.pCommandBuffers = &cmd;
	submitInfo.commandBufferCount = 1;

	// .fence will now block until the graphic commands finish execution
	graphicsQueue.submit(submitInfo, uploadContext.fence);
	
	device.waitForFences({ uploadContext.fence }, true, kWaitTimeOneSecond);
	device.resetFences({ uploadContext.fence });
	
	// reset the command buffers inside the command pool
	device.resetCommandPool(uploadContext.commandPool, vk::CommandPoolResetFlags(0));
}

void Zn::VulkanDevice::CopyBufferToImage(vk::CommandBuffer cmd, vk::Buffer buffer, vk::Image img, u32 width, u32 height)
{
	vk::BufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = vk::Offset3D{ 0, 0, 0 };
	region.imageExtent = vk::Extent3D{
		width,
		height,
		1
	};

	cmd.copyBufferToImage(buffer, img, vk::ImageLayout::eTransferDstOptimal, { region });
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

	destroyQueue.Enqueue([this, OutObject, Owner, Destructor = std::move(Destroy)]() mutable
	{
		if (Owner != VK_NULL_HANDLE && OutObject != VK_NULL_HANDLE)
		{
			VkDestroy(OutObject, Owner, Destructor);
		}
	});
}