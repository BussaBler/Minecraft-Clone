#include "VkContext.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
float fov = 45.0f;

bool mouseLeft = false;
bool mouseRight = false;
bool mouseMiddle = false;

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif
/// 
/// STRUCTS
/// 

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    int width;
    int height;
    int padding[2];
};

/// 
/// METHODS
/// 

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_BLEND_OPERATION_ADVANCED_EXTENSION_NAME
};

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<VkContext*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

static void mouseCallBack(GLFWwindow* pWindow, double xPosIn, double yPosIn) {
    Game* game = reinterpret_cast<Game*>(glfwGetWindowUserPointer(pWindow));

    float xpos = static_cast<float>(xPosIn);
    float ypos = static_cast<float>(yPosIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xOffset = xpos - lastX;
    float yOffset = ypos - lastY;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xOffset *= sensitivity;
    yOffset *= sensitivity;

    yaw += xOffset;
    pitch += yOffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front(0, 0, 0);
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.z = -sin(glm::radians(pitch));
    front.y = -sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    game->worldCamera.forwards = glm::normalize(front);
}

void VkContext::processInput(GLFWwindow* pWindow, Game* pGame) {


    if (glfwGetKey(pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(pWindow, true);
    float cameraSpeed = 7.0f * pGame->deltaTime;
    if (glfwGetKey(pWindow, GLFW_KEY_W) == GLFW_PRESS)
        pGame->worldCamera.pos += cameraSpeed * pGame->worldCamera.forwards;
    if (glfwGetKey(pWindow, GLFW_KEY_S) == GLFW_PRESS)
        pGame->worldCamera.pos -= cameraSpeed * pGame->worldCamera.forwards;
    if (glfwGetKey(pWindow, GLFW_KEY_A) == GLFW_PRESS)
        pGame->worldCamera.pos -= glm::normalize(glm::cross(pGame->worldCamera.forwards, pGame->worldCamera.up)) * cameraSpeed;
    if (glfwGetKey(pWindow, GLFW_KEY_D) == GLFW_PRESS)
        pGame->worldCamera.pos += glm::normalize(glm::cross(pGame->worldCamera.forwards, pGame->worldCamera.up)) * cameraSpeed;

    bool isMouseLeftPressed = glfwGetMouseButton(pWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    if (isMouseLeftPressed && !mouseLeft) {
        
        HitResult result = pGame->raycastBlock();
        if (result.face >= 0) {

            glm::vec3 rayHit = result.pos;

            glm::vec3 currentChunkPos = floor(rayHit / (float)CHUNK_SIZE);
            std::tuple<int, int, int> currentChunk = { currentChunkPos.x, currentChunkPos.y, currentChunkPos.z };

            Chunk& chunk = pGame->worldChunks.find(currentChunk)->second;

            glm::vec3 chunkWorldPos = currentChunkPos * static_cast<float>(CHUNK_SIZE);
            glm::vec3 localBlockPos = rayHit - chunkWorldPos;

            localBlockPos.x = static_cast<int>(localBlockPos.x) % CHUNK_SIZE;
            localBlockPos.y = static_cast<int>(localBlockPos.y) % CHUNK_SIZE;
            localBlockPos.z = static_cast<int>(localBlockPos.z) % CHUNK_SIZE;

            chunk.removeBlock(localBlockPos, pGame->worldChunks);
        }
    }

    mouseLeft = isMouseLeftPressed;

    bool isMouseRightPressed = glfwGetMouseButton(pWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

    if (isMouseRightPressed && !mouseRight) {

        HitResult result = pGame->raycastBlock();
        if (result.face >= 0) {

            glm::vec3 rayHit = result.pos;

            switch (result.face) {
            case 0: // Left face
                rayHit.x -= 1.0f;
                break;
            case 1: // Right face
                rayHit.x += 1.0f;
                break;
            case 2: // Back face
                rayHit.y -= 1.0f;
                break;
            case 3: // Front face
                rayHit.y += 1.0f;
                break;
            case 4: // Bottom face
                rayHit.z -= 1.0f;
                break;
            case 5: // Top face
                rayHit.z += 1.0f;
                break;
            }

            glm::vec3 currentChunkPos = floor(rayHit / (float)CHUNK_SIZE);
            std::tuple<int, int, int> currentChunk = { currentChunkPos.x, currentChunkPos.y, currentChunkPos.z };

            Chunk& chunk = pGame->worldChunks.find(currentChunk)->second;

            glm::vec3 chunkWorldPos = currentChunkPos * static_cast<float>(CHUNK_SIZE);
            glm::vec3 localBlockPos = rayHit - chunkWorldPos;

            localBlockPos.x = static_cast<int>(localBlockPos.x) % CHUNK_SIZE;
            localBlockPos.y = static_cast<int>(localBlockPos.y) % CHUNK_SIZE;
            localBlockPos.z = static_cast<int>(localBlockPos.z) % CHUNK_SIZE;

            chunk.addBlock(localBlockPos, pGame->selectedBlock, pGame->worldChunks);
        }
    }

    mouseRight = isMouseRightPressed;

    bool isMouseMiddlePressed = glfwGetMouseButton(pWindow, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;

    if (isMouseMiddlePressed && !mouseMiddle) {
        HitResult result = pGame->raycastBlock();
        if (result.face >= 0) {
            pGame->selectedBlock = result.block;
        }
    }
}

void VkContext::initWindow(VkContext* pContext, Game* pGame) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    pContext->window = glfwCreateWindow(WIDTH, HEIGHT, "Mine", nullptr, nullptr);
    glfwSetInputMode(pContext->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetWindowUserPointer(pContext->window, pContext);
    glfwSetFramebufferSizeCallback(pContext->window, framebufferResizeCallback);
    glfwSetWindowUserPointer(pContext->window, pGame);
    glfwSetCursorPosCallback(pContext->window, mouseCallBack);
}

static bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

std::vector<const char*> getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

static void createInstance(VkContext* pContext) {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Mine Clone";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &pContext->instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void setupDebugMessenger(VkContext* pContext) {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(pContext->instance, &createInfo, nullptr, &pContext->debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

static void createSurface(VkContext* pContext) {
    if (glfwCreateWindowSurface(pContext->instance, pContext->window, nullptr, &pContext->surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkContext* pContext) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, pContext->surface, &presentSupport);

        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

static bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkContext* pContext) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, pContext->surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, pContext->surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, pContext->surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, pContext->surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, pContext->surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

static bool isDeviceSuitable(VkPhysicalDevice device, VkContext* pContext) {
    QueueFamilyIndices indices = findQueueFamilies(device, pContext);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, pContext);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

static void pickPhysicalDevice(VkContext* pContext) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(pContext->instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(pContext->instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (isDeviceSuitable(device, pContext)) {
            pContext->physicalDevice = device;
            break;
        }
    }

    if (pContext->physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

static void createLogicalDevice(VkContext* pContext) {
    QueueFamilyIndices indices = findQueueFamilies(pContext->physicalDevice, pContext);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.logicOp = VK_TRUE;
    deviceFeatures.fillModeNonSolid = VK_TRUE;
    deviceFeatures.wideLines = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(pContext->physicalDevice, &createInfo, nullptr, &pContext->device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(pContext->device, indices.graphicsFamily.value(), 0, &pContext->graphicsQueue);
    vkGetDeviceQueue(pContext->device, indices.presentFamily.value(), 0, &pContext->presentQueue);
}

static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, VkContext* pContext) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    else {
        int width, height;
        glfwGetFramebufferSize(pContext->window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

static void createSwapChain(VkContext* pContext) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(pContext->physicalDevice, pContext);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, pContext);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = pContext->surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(pContext->physicalDevice, pContext);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(pContext->device, &createInfo, nullptr, &pContext->swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(pContext->device, pContext->swapChain, &imageCount, nullptr);
    pContext->swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(pContext->device, pContext->swapChain, &imageCount, pContext->swapChainImages.data());

    pContext->swapChainImageFormat = surfaceFormat.format;
    pContext->swapChainExtent = extent;
}

static VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkContext* pContext, uint32_t mipLevels) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(pContext->device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }

    return imageView;
}

static void createImageViews(VkContext* pContext) {
    pContext->swapChainImageViews.resize(pContext->swapChainImages.size());

    for (uint32_t i = 0; i < pContext->swapChainImages.size(); i++) {
        pContext->swapChainImageViews[i] = createImageView(pContext->swapChainImages[i], pContext->swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, pContext, 1);
    }
}

static VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkContext* pContext) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(pContext->physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

static VkFormat findDepthFormat(VkContext* pContext) {
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
        pContext);
}

static void createRenderPass(VkContext* pContext) {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = pContext->swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat(pContext);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(pContext->device, &renderPassInfo, nullptr, &pContext->renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

static void createDescriptorSetLayout(VkContext* pContext) {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(pContext->device, &layoutInfo, nullptr, &pContext->descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

static VkShaderModule createShaderModule(const std::vector<char>& code, VkContext* pContext) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(pContext->device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R8G8B8_UINT;
    attributeDescriptions[0].offset = offsetof(Vertex, posX);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, texCoord);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R8_SINT;
    attributeDescriptions[2].offset = offsetof(Vertex, normal);

    return attributeDescriptions;
}

static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

static void createGraphicsPipeline(VkContext* pContext) {
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, pContext);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, pContext);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = getBindingDescription();
    auto attributeDescriptions = getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; 
    pushConstantRange.offset = 0;     
    pushConstantRange.size = sizeof(glm::vec4);

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &pContext->descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(pContext->device, &pipelineLayoutInfo, nullptr, &pContext->pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pContext->pipelineLayout;
    pipelineInfo.renderPass = pContext->renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VkPipelineCacheCreateInfo cacheInfo{};
    cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    vkCreatePipelineCache(pContext->device, &cacheInfo, nullptr, &pContext->pipelineCache);

    if (vkCreateGraphicsPipelines(pContext->device, pContext->pipelineCache, 1, &pipelineInfo, nullptr, &pContext->mainPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
    
    vkDestroyShaderModule(pContext->device, vertShaderModule, nullptr);

    auto liquidVertShaderCode = readFile("shaders/liquidVert.spv");

    VkShaderModule liquidVertShaderModule = createShaderModule(liquidVertShaderCode, pContext);

    VkPipelineShaderStageCreateInfo liquidVertShaderStageInfo{};
    liquidVertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    liquidVertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    liquidVertShaderStageInfo.module = liquidVertShaderModule;
    liquidVertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo liquidShaderStages[] = { liquidVertShaderStageInfo, fragShaderStageInfo };

    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo transparentColorBlending = colorBlending;
    transparentColorBlending.pAttachments = &colorBlendAttachment;

    VkGraphicsPipelineCreateInfo liquidPipelineInfo{};
    liquidPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    liquidPipelineInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    liquidPipelineInfo.stageCount = 2;
    liquidPipelineInfo.pStages = liquidShaderStages;
    liquidPipelineInfo.pVertexInputState = &vertexInputInfo;
    liquidPipelineInfo.pInputAssemblyState = &inputAssembly;
    liquidPipelineInfo.pViewportState = &viewportState;
    liquidPipelineInfo.pRasterizationState = &rasterizer;
    liquidPipelineInfo.pMultisampleState = &multisampling;
    liquidPipelineInfo.pDepthStencilState = &depthStencil;
    liquidPipelineInfo.pColorBlendState = &transparentColorBlending;
    liquidPipelineInfo.pDynamicState = &dynamicState;
    liquidPipelineInfo.layout = pContext->pipelineLayout;
    liquidPipelineInfo.renderPass = pContext->renderPass;
    liquidPipelineInfo.subpass = 0;
    liquidPipelineInfo.basePipelineHandle = pContext->mainPipeline;
    liquidPipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(pContext->device, pContext->pipelineCache, 1, &liquidPipelineInfo, nullptr, &pContext->liquidPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create liquid graphics pipeline!");
    }

    vkDestroyShaderModule(pContext->device, liquidVertShaderModule, nullptr);

    auto crosshairVertShaderCode = readFile("shaders/crosshairVert.spv");

    VkShaderModule crosshairVertShaderModule = createShaderModule(crosshairVertShaderCode, pContext);

    VkPipelineShaderStageCreateInfo crosshairVertShaderStageInfo{};
    crosshairVertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    crosshairVertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    crosshairVertShaderStageInfo.module = crosshairVertShaderModule;
    crosshairVertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo crosshairShaderStages[] = { crosshairVertShaderStageInfo, fragShaderStageInfo };

    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_INVERT_RGB_EXT;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_INVERT_RGB_EXT;

    VkPipelineColorBlendAdvancedStateCreateInfoEXT advancedBlendState = {};
    advancedBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_ADVANCED_STATE_CREATE_INFO_EXT;
    advancedBlendState.srcPremultiplied = VK_FALSE;
    advancedBlendState.dstPremultiplied = VK_FALSE;
    advancedBlendState.blendOverlap = VK_BLEND_OVERLAP_UNCORRELATED_EXT;

    VkPipelineColorBlendStateCreateInfo crosshairColorBlending = colorBlending;
    crosshairColorBlending.pAttachments = &colorBlendAttachment;
    crosshairColorBlending.pNext = &advancedBlendState;

    VkGraphicsPipelineCreateInfo crosshairPipelineInfo{};
    crosshairPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    crosshairPipelineInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    crosshairPipelineInfo.stageCount = 2;
    crosshairPipelineInfo.pStages = crosshairShaderStages;
    crosshairPipelineInfo.pVertexInputState = &vertexInputInfo;
    crosshairPipelineInfo.pInputAssemblyState = &inputAssembly;
    crosshairPipelineInfo.pViewportState = &viewportState;
    crosshairPipelineInfo.pRasterizationState = &rasterizer;
    crosshairPipelineInfo.pMultisampleState = &multisampling;
    crosshairPipelineInfo.pDepthStencilState = &depthStencil;
    crosshairPipelineInfo.pColorBlendState = &crosshairColorBlending;
    crosshairPipelineInfo.pDynamicState = &dynamicState;
    crosshairPipelineInfo.layout = pContext->pipelineLayout;
    crosshairPipelineInfo.renderPass = pContext->renderPass;
    crosshairPipelineInfo.subpass = 0;
    crosshairPipelineInfo.basePipelineHandle = pContext->mainPipeline;
    crosshairPipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(pContext->device, pContext->pipelineCache, 1, &crosshairPipelineInfo, nullptr, &pContext->crosshairPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create crosshair graphics pipeline!");
    }

    vkDestroyShaderModule(pContext->device, crosshairVertShaderModule, nullptr);

    auto wireframeVertShaderCode = readFile("shaders/wireframeVert.spv");
    auto wireframeFragShaderCode = readFile("shaders/wireframeFrag.spv");

    VkShaderModule wireframeVertShaderModule = createShaderModule(wireframeVertShaderCode, pContext);

    VkPipelineShaderStageCreateInfo wireframeVertShaderStageInfo{};
    wireframeVertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    wireframeVertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    wireframeVertShaderStageInfo.module = wireframeVertShaderModule;
    wireframeVertShaderStageInfo.pName = "main";

    VkShaderModule wireframeFragShaderModule = createShaderModule(wireframeFragShaderCode, pContext);

    VkPipelineShaderStageCreateInfo wireframeFragShaderStageInfo{};
    wireframeFragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    wireframeFragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    wireframeFragShaderStageInfo.module = wireframeFragShaderModule;
    wireframeFragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo wireframeShaderStages[] = { wireframeVertShaderStageInfo, wireframeFragShaderStageInfo };

    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
    rasterizer.lineWidth = 2.0f;

    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo wireframeColorBlending = colorBlending;
    wireframeColorBlending.pAttachments = &colorBlendAttachment;

    VkGraphicsPipelineCreateInfo wireframePipelineInfo{};
    wireframePipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    wireframePipelineInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    wireframePipelineInfo.stageCount = 2;
    wireframePipelineInfo.pStages = wireframeShaderStages;
    wireframePipelineInfo.pVertexInputState = &vertexInputInfo;
    wireframePipelineInfo.pInputAssemblyState = &inputAssembly;
    wireframePipelineInfo.pViewportState = &viewportState;
    wireframePipelineInfo.pRasterizationState = &rasterizer;
    wireframePipelineInfo.pMultisampleState = &multisampling;
    wireframePipelineInfo.pDepthStencilState = &depthStencil;
    wireframePipelineInfo.pColorBlendState = &wireframeColorBlending;
    wireframePipelineInfo.pDynamicState = &dynamicState;
    wireframePipelineInfo.layout = pContext->pipelineLayout;
    wireframePipelineInfo.renderPass = pContext->renderPass;
    wireframePipelineInfo.subpass = 0;
    wireframePipelineInfo.basePipelineHandle = pContext->mainPipeline;
    wireframePipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(pContext->device, pContext->pipelineCache, 1, &wireframePipelineInfo, nullptr, &pContext->wireframePipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create crosshair graphics pipeline!");
    }

    vkDestroyShaderModule(pContext->device, wireframeVertShaderModule, nullptr);
    vkDestroyShaderModule(pContext->device, wireframeFragShaderModule, nullptr);
    vkDestroyShaderModule(pContext->device, fragShaderModule, nullptr);
}

static void createCommandPool(VkContext* pContext) {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(pContext->physicalDevice, pContext);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(pContext->device, &poolInfo, nullptr, &pContext->commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics command pool!");
    }
}

static uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkContext* pContext) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(pContext->physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

static void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkContext* pContext, uint32_t mipLevels) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(pContext->device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(pContext->device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, pContext);

    if (vkAllocateMemory(pContext->device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(pContext->device, image, imageMemory, 0);
}

static void createDepthResources(VkContext* pContext) {
    VkFormat depthFormat = findDepthFormat(pContext);

    createImage(pContext->swapChainExtent.width, pContext->swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, pContext->depthImage, pContext->depthImageMemory, pContext, 1);
    pContext->depthImageView = createImageView(pContext->depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, pContext, 1);
}

static void createFramebuffers(VkContext* pContext) {
    pContext->swapChainFramebuffers.resize(pContext->swapChainImageViews.size());

    for (size_t i = 0; i < pContext->swapChainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            pContext->swapChainImageViews[i],
            pContext->depthImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = pContext->renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = pContext->swapChainExtent.width;
        framebufferInfo.height = pContext->swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(pContext->device, &framebufferInfo, nullptr, &pContext->swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

static void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkContext* pContext) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(pContext->device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(pContext->device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, pContext);

    if (vkAllocateMemory(pContext->device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(pContext->device, buffer, bufferMemory, 0);
}

static VkCommandBuffer beginSingleTimeCommands(VkContext* pContext) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = pContext->commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(pContext->device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

static void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkContext* pContext) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(pContext->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(pContext->graphicsQueue);

    vkFreeCommandBuffers(pContext->device, pContext->commandPool, 1, &commandBuffer);
}

static void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkContext* pContext, uint32_t mipLevels) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(pContext);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(commandBuffer, pContext);
}

static void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkContext* pContext) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(pContext);

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

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer, pContext);
}

static void createTextureImage(VkContext* pContext) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("textures/atlas.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    pContext->mipLevels = 1;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, pContext);

    void* data;
    vkMapMemory(pContext->device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(pContext->device, stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, pContext->textureImage, pContext->textureImageMemory, pContext, pContext->mipLevels);

    transitionImageLayout(pContext->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, pContext, pContext->mipLevels);
    copyBufferToImage(stagingBuffer, pContext->textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), pContext);
    transitionImageLayout(pContext->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pContext, pContext->mipLevels);

    vkDestroyBuffer(pContext->device, stagingBuffer, nullptr);
    vkFreeMemory(pContext->device, stagingBufferMemory, nullptr);
}

static void createTextureImageView(VkContext* pContext) {
    pContext->textureImageView = createImageView(pContext->textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, pContext, pContext->mipLevels);
}

static void createTextureSampler(VkContext* pContext) {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(pContext->physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(pContext->device, &samplerInfo, nullptr, &pContext->textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

static void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkContext* pContext) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(pContext);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer, pContext);
}

std::array<Vertex, 4> crosshairVertices = {
    Vertex{glm::vec3(0, 0, 0), glm::vec2(0, 16.f / 1024.f), 0},
    {glm::vec3(1, 0, 0), glm::vec2(16.f / 1024.f, 16.f / 1024.f), 0},
    {glm::vec3(1, 0, 1), glm::vec2(16.f / 1024.f, 32.f / 1024.f), 0},
    {glm::vec3(0, 0, 1), glm::vec2(0, 32.f / 1024.f), 0},
};

std::array<uint32_t, 6> crosshairIndices = {
    0, 3, 2,
    2, 1, 0
};

std::array<Vertex, 8> cubeVertices = {
    // Front Bottom Left
    Vertex(glm::vec3(-0.001f, -0.001f, -0.001f), glm::vec2(0.0f, 0.0f), 0),
    // Front Bottom Right
    Vertex(glm::vec3(1.001f, -0.001f, -0.001f), glm::vec2(0.0f, 0.0f), 0),
    // Front Top Right
    Vertex(glm::vec3(1.0f, 1.001f, -0.001f), glm::vec2(0.0f, 0.0f), 0),
    // Front Top Left
    Vertex(glm::vec3(-0.001f, 1.001f, -0.001f), glm::vec2(0.0f, 0.0f), 0),
    // Back Bottom Left
    Vertex(glm::vec3(-0.001f, -0.001f, 1.001f), glm::vec2(0.0f, 0.0f), 0),
    // Back Bottom Right
    Vertex(glm::vec3(1.0f, -0.001f, 1.001f), glm::vec2(0.0f, 0.0f), 0),
    // Back Top Right
    Vertex(glm::vec3(1.001f, 1.001f, 1.001f), glm::vec2(0.0f, 0.0f), 0),
    // Back Top Left
    Vertex(glm::vec3(-0.001f, 1.001f, 1.001f), glm::vec2(0.0f, 0.0f), 0)
};

std::array<uint32_t, 24> cubeIndices = {
    // Front face edges
    0, 1,  // Bottom edge
    1, 2,  // Right edge
    2, 3,  // Top edge
    3, 0,  // Left edge

    // Back face edges
    4, 5,  // Bottom edge
    5, 6,  // Right edge
    6, 7,  // Top edge
    7, 4,  // Left edge

    // Connecting edges
    0, 4,  // Front bottom to back bottom
    1, 5,  // Front bottom right to back bottom right
    2, 6,  // Front top right to back top right
    3, 7   // Front top left to back top left
};

static void createVertexBuffer(VkContext* pContext) {
    // CROSSHAIR
    VkDeviceSize bufferSize = sizeof(crosshairVertices[0]) * crosshairVertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, pContext);

    void* data;
    vkMapMemory(pContext->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, crosshairVertices.data(), (size_t)bufferSize);
    vkUnmapMemory(pContext->device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, pContext->crosshairVertexBuffer, pContext->crosshairVertexBufferMemory, pContext);

    copyBuffer(stagingBuffer, pContext->crosshairVertexBuffer, bufferSize, pContext);

    vkDestroyBuffer(pContext->device, stagingBuffer, nullptr);
    vkFreeMemory(pContext->device, stagingBufferMemory, nullptr);

    //WIREFRAME CUBE

    bufferSize = sizeof(cubeVertices[0]) * cubeVertices.size();
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, pContext);

    vkMapMemory(pContext->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, cubeVertices.data(), (size_t)bufferSize);
    vkUnmapMemory(pContext->device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, pContext->wireframeVertexBuffer, pContext->wireframeVertexBufferMemory, pContext);

    copyBuffer(stagingBuffer, pContext->wireframeVertexBuffer, bufferSize, pContext);

    vkDestroyBuffer(pContext->device, stagingBuffer, nullptr);
    vkFreeMemory(pContext->device, stagingBufferMemory, nullptr);
}

static void createIndexBuffer(VkContext* pContext) {
    // CROSSHAIR
    VkDeviceSize bufferSize = sizeof(crosshairIndices[0]) * crosshairIndices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, pContext);

    void* data;
    vkMapMemory(pContext->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, crosshairIndices.data(), (size_t)bufferSize);
    vkUnmapMemory(pContext->device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, pContext->crosshairIndexBuffer, pContext->crosshairIndexBufferMemory, pContext);

    copyBuffer(stagingBuffer, pContext->crosshairIndexBuffer, bufferSize, pContext);

    vkDestroyBuffer(pContext->device, stagingBuffer, nullptr);
    vkFreeMemory(pContext->device, stagingBufferMemory, nullptr);

    //WIREFRAME CUBE
    bufferSize = sizeof(cubeIndices[0]) * cubeIndices.size();

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, pContext);

    vkMapMemory(pContext->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, cubeIndices.data(), (size_t)bufferSize);
    vkUnmapMemory(pContext->device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, pContext->wireframeIndexBuffer, pContext->wireframeIndexBufferMemory, pContext);

    copyBuffer(stagingBuffer, pContext->wireframeIndexBuffer, bufferSize, pContext);

    vkDestroyBuffer(pContext->device, stagingBuffer, nullptr);
    vkFreeMemory(pContext->device, stagingBufferMemory, nullptr);
}

static void createStagingBuffersForChunk(VkContext* pContext) {
    
    size_t maxVerticesPerBlock = 24; 
    size_t totalBlocksInChunk = 32 * 32 * 32; 
    size_t totalVertices = totalBlocksInChunk * maxVerticesPerBlock; 
    VkDeviceSize vertexBufferSize = totalVertices * sizeof(Vertex);

    
    size_t maxIndicesPerBlock = 36; 
    size_t totalIndices = totalBlocksInChunk * maxIndicesPerBlock; 
    VkDeviceSize indexBufferSize = totalIndices * sizeof(uint32_t);

    createBuffer(vertexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        pContext->vertexStagingBuffer,
        pContext->vertexStagingBufferMemory,
        pContext);

    createBuffer(indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        pContext->indexStagingBuffer,
        pContext->indexStagingBufferMemory,
        pContext);
}

static void createBuffersForChunk(VkContext* pContext, Chunk& chunk) {
    size_t vertexBufferSize = chunk.mesh.vertices.size() * sizeof(Vertex);
    size_t indexBufferSize = chunk.mesh.indices.size() * sizeof(uint32_t);
    size_t transparentVertexBufferSize = chunk.transparentMesh.vertices.size() * sizeof(Vertex);
    size_t transparentIndexBufferSize = chunk.transparentMesh.indices.size() * sizeof(uint32_t);

    size_t maxVertexBufferSize = chunk.size * chunk.size * chunk.size * sizeof(Vertex) * 24;
    size_t maxIndexBufferSize = chunk.size * chunk.size * chunk.size * sizeof(uint32_t) * 36;

    if (chunk.vertexBuffer == VK_NULL_HANDLE)
    createBuffer(maxVertexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        chunk.vertexBuffer,
        chunk.vertexBufferMemory,
        pContext);

    if (chunk.indexBuffer == VK_NULL_HANDLE)
    createBuffer(maxIndexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        chunk.indexBuffer,
        chunk.indexBufferMemory,
        pContext);

    if (chunk.liquidVertexBuffer == VK_NULL_HANDLE && transparentVertexBufferSize > 0)
        createBuffer(maxVertexBufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            chunk.liquidVertexBuffer,
            chunk.liquidVertexBufferMemory,
            pContext);

    if (chunk.liquidIndexBuffer == VK_NULL_HANDLE && transparentVertexBufferSize > 0)
        createBuffer(maxIndexBufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            chunk.liquidIndexBuffer,
            chunk.liquidIndexBufferMemory,
            pContext);


    void* vertexData;
    vkMapMemory(pContext->device, pContext->vertexStagingBufferMemory, 0, vertexBufferSize, 0, &vertexData);

    void* indexData;
    vkMapMemory(pContext->device, pContext->indexStagingBufferMemory, 0, indexBufferSize, 0, &indexData);

    memcpy(vertexData, chunk.mesh.vertices.data(), vertexBufferSize);
    memcpy(indexData, chunk.mesh.indices.data(), indexBufferSize);

    vkUnmapMemory(pContext->device, pContext->vertexStagingBufferMemory);
    vkUnmapMemory(pContext->device, pContext->indexStagingBufferMemory);

    copyBuffer(pContext->vertexStagingBuffer, chunk.vertexBuffer, vertexBufferSize, pContext);
    copyBuffer(pContext->indexStagingBuffer, chunk.indexBuffer, indexBufferSize, pContext);

    if (transparentVertexBufferSize > 0) {
        void* transparentVertexData;
        vkMapMemory(pContext->device, pContext->vertexStagingBufferMemory, 0, vertexBufferSize, 0, &transparentVertexData);

        void* transparentIndexData;
        vkMapMemory(pContext->device, pContext->indexStagingBufferMemory, 0, indexBufferSize, 0, &transparentIndexData);

        memcpy(transparentVertexData, chunk.transparentMesh.vertices.data(), transparentVertexBufferSize);
        memcpy(transparentIndexData, chunk.transparentMesh.indices.data(), transparentIndexBufferSize);

        vkUnmapMemory(pContext->device, pContext->vertexStagingBufferMemory);
        vkUnmapMemory(pContext->device, pContext->indexStagingBufferMemory);

        copyBuffer(pContext->vertexStagingBuffer, chunk.liquidVertexBuffer, transparentVertexBufferSize, pContext);
        copyBuffer(pContext->indexStagingBuffer, chunk.liquidIndexBuffer, transparentIndexBufferSize, pContext);
    }

    chunk.ready = true; 
}

static void createUniformBuffers(VkContext* pContext) {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    pContext->uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    pContext->uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    pContext->uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, pContext->uniformBuffers[i], pContext->uniformBuffersMemory[i], pContext);

        vkMapMemory(pContext->device, pContext->uniformBuffersMemory[i], 0, bufferSize, 0, &pContext->uniformBuffersMapped[i]);
    }
}

static void createDescriptorPool(VkContext* pContext) {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(pContext->device, &poolInfo, nullptr, &pContext->descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

static void createDescriptorSets(VkContext* pContext) {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, pContext->descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pContext->descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    pContext->descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(pContext->device, &allocInfo, pContext->descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = pContext->uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = pContext->textureImageView;
        imageInfo.sampler = pContext->textureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = pContext->descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = pContext->descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(pContext->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

static void createCommandBuffers(VkContext* pContext) {
    pContext->commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = pContext->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)pContext->commandBuffers.size();

    if (vkAllocateCommandBuffers(pContext->device, &allocInfo, pContext->commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

static void createSyncObjects(VkContext* pContext) {
    pContext->imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    pContext->renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    pContext->inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(pContext->device, &semaphoreInfo, nullptr, &pContext->imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(pContext->device, &semaphoreInfo, nullptr, &pContext->renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(pContext->device, &fenceInfo, nullptr, &pContext->inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void VkContext::initVulkan(VkContext* pContext, Game* pGame) {
    createInstance(pContext);
    setupDebugMessenger(pContext);
    createSurface(pContext);
    pickPhysicalDevice(pContext);
    createLogicalDevice(pContext);
    createSwapChain(pContext);
    createImageViews(pContext);
    createRenderPass(pContext);
    createDescriptorSetLayout(pContext);
    createGraphicsPipeline(pContext);
    createCommandPool(pContext);
    createDepthResources(pContext);
    createFramebuffers(pContext);
    createTextureImage(pContext);
    createTextureImageView(pContext);
    createTextureSampler(pContext);
    createStagingBuffersForChunk(pContext);
    createVertexBuffer(pContext);
    createIndexBuffer(pContext);
    createUniformBuffers(pContext);
    createDescriptorPool(pContext);
    createDescriptorSets(pContext);
    createCommandBuffers(pContext);
    createSyncObjects(pContext);
}

static void cleanupSwapChain(VkContext* pContext) {
    vkDestroyImageView(pContext->device, pContext->depthImageView, nullptr);
    vkDestroyImage(pContext->device, pContext->depthImage, nullptr);
    vkFreeMemory(pContext->device, pContext->depthImageMemory, nullptr);

    for (auto framebuffer : pContext->swapChainFramebuffers) {
        vkDestroyFramebuffer(pContext->device, framebuffer, nullptr);
    }

    for (auto imageView : pContext->swapChainImageViews) {
        vkDestroyImageView(pContext->device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(pContext->device, pContext->swapChain, nullptr);
}

static void recreateSwapChain(VkContext* pContext) {
    int width = 0, height = 0;
    glfwGetFramebufferSize(pContext->window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(pContext->window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(pContext->device);

    cleanupSwapChain(pContext);

    createSwapChain(pContext);
    createImageViews(pContext);
    createDepthResources(pContext);
    createFramebuffers(pContext);
}

static void updateUniformBuffer(uint32_t currentImage, VkContext* pContext, Game* pGame) {

    UniformBufferObject ubo{};
    ubo.model = glm::mat4(1.0f);
    ubo.view = glm::lookAt(pGame->worldCamera.pos, pGame->worldCamera.pos + pGame->worldCamera.forwards, pGame->worldCamera.up);
    ubo.proj = glm::perspective(glm::radians(45.0f), (pContext->swapChainExtent.width / (float)pContext->swapChainExtent.height), 0.1f, 1000.0f);
    ubo.proj[1][1] *= -1;
    pGame->worldCamera.view = ubo.view;
    pGame->worldCamera.proj = ubo.proj;
    ubo.width = pContext->swapChainExtent.width;
    ubo.height = pContext->swapChainExtent.height;

    memcpy(pContext->uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

static void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, VkContext* pContext, Game* pGame) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = pContext->renderPass;
    renderPassInfo.framebuffer = pContext->swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = pContext->swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.5f, 0.87f, 1.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pContext->mainPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)pContext->swapChainExtent.width;
    viewport.height = (float)pContext->swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = pContext->swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[1]{};
    VkDeviceSize offsets[] = { 0 };

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pContext->pipelineLayout, 0, 1, &pContext->descriptorSets[pContext->currentFrame], 0, nullptr);

    for (auto& chunk : pGame->worldChunks) {
        if (chunk.second.dirty || !chunk.second.visible) {
            continue;
        }
        if (!chunk.second.ready && chunk.second.baked && chunk.second.mesh.vertices.size() > 0) {
            createBuffersForChunk(pContext, chunk.second);
        }
        if (chunk.second.baked && chunk.second.mesh.vertices.size() > 0) {
            vertexBuffers[0] = { chunk.second.vertexBuffer };
            glm::vec4 push = glm::vec4(chunk.second.pos, 1);
            vkCmdPushConstants(commandBuffer, pContext->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::vec4), &push);
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, chunk.second.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, chunk.second.mesh.indices.size(), 1, 0, 0, 0);
        }
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pContext->liquidPipeline);

    for (auto& chunk : pGame->worldChunks) {
        if (chunk.second.dirty || !chunk.second.visible) {
            continue;
        }
        if (!chunk.second.ready && chunk.second.baked && chunk.second.transparentMesh.vertices.size() > 0) {
            createBuffersForChunk(pContext, chunk.second);
        }
        if (chunk.second.baked && chunk.second.transparentMesh.vertices.size() > 0) {
            vertexBuffers[0] = { chunk.second.liquidVertexBuffer };
            glm::vec4 push = glm::vec4(chunk.second.pos, glfwGetTime());
            vkCmdPushConstants(commandBuffer, pContext->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::vec4), &push);
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, chunk.second.liquidIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, chunk.second.transparentMesh.indices.size(), 1, 0, 0, 0);
        }
    }

    HitResult result = pGame->raycastBlock();
    if (result.face >= 0) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pContext->wireframePipeline);
        vertexBuffers[0] = { pContext->wireframeVertexBuffer };
        glm::vec4 push = glm::vec4(result.pos, 0);
        vkCmdPushConstants(commandBuffer, pContext->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::vec4), &push);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, pContext->wireframeIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, cubeIndices.size(), 1, 0, 0, 0);
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pContext->crosshairPipeline);

    vertexBuffers[0] = { pContext->crosshairVertexBuffer };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, pContext->crosshairIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, crosshairIndices.size(), 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void VkContext::drawFrame(VkContext* pContext, Game* pGame) {
    vkWaitForFences(pContext->device, 1, &pContext->inFlightFences[pContext->currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(pContext->device, pContext->swapChain, UINT64_MAX, pContext->imageAvailableSemaphores[pContext->currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain(pContext);
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    updateUniformBuffer(pContext->currentFrame, pContext, pGame);

    vkResetFences(pContext->device, 1, &pContext->inFlightFences[pContext->currentFrame]);

    vkResetCommandBuffer(pContext->commandBuffers[pContext->currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
    recordCommandBuffer(pContext->commandBuffers[pContext->currentFrame], imageIndex, pContext, pGame);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { pContext->imageAvailableSemaphores[pContext->currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &pContext->commandBuffers[pContext->currentFrame];

    VkSemaphore signalSemaphores[] = { pContext->renderFinishedSemaphores[pContext->currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(pContext->graphicsQueue, 1, &submitInfo, pContext->inFlightFences[pContext->currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { pContext->swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(pContext->presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || pContext->framebufferResized) {
        pContext->framebufferResized = false;
        recreateSwapChain(pContext);
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    pContext->currentFrame = (pContext->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

}

static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void VkContext::cleanup(VkContext* pContext, Game* pGame) {
    cleanupSwapChain(pContext);

    vkDestroyPipeline(pContext->device, pContext->mainPipeline, nullptr);
    vkDestroyPipeline(pContext->device, pContext->liquidPipeline, nullptr);
    vkDestroyPipeline(pContext->device, pContext->crosshairPipeline, nullptr);
    vkDestroyPipeline(pContext->device, pContext->wireframePipeline, nullptr);
    vkDestroyPipelineCache(pContext->device, pContext->pipelineCache, nullptr);
    vkDestroyPipelineLayout(pContext->device, pContext->pipelineLayout, nullptr);
    vkDestroyRenderPass(pContext->device, pContext->renderPass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(pContext->device, pContext->uniformBuffers[i], nullptr);
        vkFreeMemory(pContext->device, pContext->uniformBuffersMemory[i], nullptr);
    }

    for (auto& chunk : pGame->worldChunks) {
        vkDestroyBuffer(pContext->device, chunk.second.vertexBuffer, nullptr);
        vkFreeMemory(pContext->device, chunk.second.vertexBufferMemory, nullptr);

        vkDestroyBuffer(pContext->device, chunk.second.indexBuffer, nullptr);
        vkFreeMemory(pContext->device, chunk.second.indexBufferMemory, nullptr);
    }

    vkDestroyDescriptorPool(pContext->device, pContext->descriptorPool, nullptr);

    vkDestroySampler(pContext->device, pContext->textureSampler, nullptr);
    vkDestroyImageView(pContext->device, pContext->textureImageView, nullptr);

    vkDestroyImage(pContext->device, pContext->textureImage, nullptr);
    vkFreeMemory(pContext->device, pContext->textureImageMemory, nullptr);

    vkDestroyDescriptorSetLayout(pContext->device, pContext->descriptorSetLayout, nullptr);

    vkDestroyBuffer(pContext->device, pContext->crosshairIndexBuffer, nullptr);
    vkFreeMemory(pContext->device, pContext->crosshairIndexBufferMemory, nullptr);

    vkDestroyBuffer(pContext->device, pContext->crosshairVertexBuffer, nullptr);
    vkFreeMemory(pContext->device, pContext->crosshairVertexBufferMemory, nullptr);

    vkDestroyBuffer(pContext->device, pContext->wireframeVertexBuffer, nullptr);
    vkFreeMemory(pContext->device, pContext->wireframeVertexBufferMemory, nullptr);

    vkDestroyBuffer(pContext->device, pContext->wireframeIndexBuffer, nullptr);
    vkFreeMemory(pContext->device, pContext->wireframeIndexBufferMemory, nullptr);

    vkDestroyBuffer(pContext->device, pContext->vertexStagingBuffer, nullptr);
    vkFreeMemory(pContext->device, pContext->vertexStagingBufferMemory, nullptr);

    vkDestroyBuffer(pContext->device, pContext->indexStagingBuffer, nullptr);
    vkFreeMemory(pContext->device, pContext->indexStagingBufferMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(pContext->device, pContext->renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(pContext->device, pContext->imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(pContext->device, pContext->inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(pContext->device, pContext->commandPool, nullptr);

    vkDestroyDevice(pContext->device, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(pContext->instance, pContext->debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(pContext->instance, pContext->surface, nullptr);
    vkDestroyInstance(pContext->instance, nullptr);

    glfwDestroyWindow(pContext->window);

    glfwTerminate();
}
