#pragma once
#include <vector>
#include <stdexcept>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <algorithm>
#include <array>
#include <fstream>
#include <chrono>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Game.h"
#include "VulkanIncludes.h"


class VkContext {
// Member variables
public:
	GLFWwindow* window;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline mainPipeline;
    VkPipelineCache pipelineCache;
    VkPipeline liquidPipeline; // used for the liquid blocks don't know what will do with glasses and etc
    VkPipeline crosshairPipeline; // a pipeline just for the crosshair ????
    VkPipeline wireframePipeline;

    VkCommandPool commandPool;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    uint32_t mipLevels;
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    VkBuffer vertexStagingBuffer;
    VkDeviceMemory vertexStagingBufferMemory;
    VkBuffer crosshairVertexBuffer;
    VkDeviceMemory crosshairVertexBufferMemory;
    VkBuffer wireframeVertexBuffer;
    VkDeviceMemory wireframeVertexBufferMemory;
    size_t currentVertexBufferSize;

    VkBuffer indexStagingBuffer;
    VkDeviceMemory indexStagingBufferMemory;
    VkBuffer crosshairIndexBuffer;
    VkDeviceMemory crosshairIndexBufferMemory;
    VkBuffer wireframeIndexBuffer;
    VkDeviceMemory wireframeIndexBufferMemory;
    size_t currentIndexBufferSize;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;

    bool framebufferResized = false;
public:
    static void initWindow(VkContext* pContext, Game* pGame);
    static void initVulkan(VkContext* pContext, Game* pGame);
    static void drawFrame(VkContext* pContext, Game* pGame);
    static void cleanup(VkContext* pContext, Game* pGame);
    static void processInput(GLFWwindow* pWindow, Game* pGame);
};

