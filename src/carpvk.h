#pragma once

#include <stdint.h>


struct VkInstance_T;
struct VkDevice_T;
struct VkDebugUtilsMessengerEXT_T;
struct VkSurfaceKHR_T;
struct VkPhysicalDevice_T;
struct VkQueue_T;
struct VkSwapchainKHR_T;
struct VkImage_T;
struct VkImageView_T;
struct VkQueryPool_T;
struct VkSemaphore_T;
struct VkFence_T;
struct VkCommandPool_T;
struct VkCommandBuffer_T;
struct VmaAllocator_T;



struct alignas(sizeof(char*)) VulkanInstanceBuilder
{
    char size[1024];
};

struct alignas(sizeof(char*)) ShaderBuilder
{

}
enum class VSyncType : unsigned char
{
    FIFO_VSYNC,
    IMMEDIATE_NO_VSYNC,
    MAILBOX_VSYNC,
};


struct CarpSwapChainFormats
{
    int64_t defaultColorFormat;
    int64_t presentColorFormat;
    int64_t depthFormat;
    int64_t colorSpace;
};


struct CarpVk
{
    static const int FramesInFlight = 4;
    static const int QueryCount = 128;

    VkInstance_T* instance = nullptr;
    VkPhysicalDevice_T* physicalDevice = nullptr;
    VkDevice_T* device = nullptr;
    VkDebugUtilsMessengerEXT_T* debugMessenger = nullptr;
    VkSurfaceKHR_T* surface = nullptr;
    VkQueue_T* queue = nullptr;
    VkSwapchainKHR_T* swapchain = nullptr;
    VkImage_T* swapchainImages[16] = {};
    VkImageView_T* swapchainImagesViews[16] = {};

    VkQueryPool_T* queryPools[FramesInFlight] = {};
    int queryPoolIndexCounts[FramesInFlight] = {};

    VkSemaphore_T* acquireSemaphores[FramesInFlight] = {};
    VkSemaphore_T* releaseSemaphores[FramesInFlight] = {};

    VkFence_T* fences[FramesInFlight] = {};
    VkCommandPool_T* commandPool = {};

    VkCommandBuffer_T* commandBuffers[FramesInFlight] = {};

    VmaAllocator_T* allocator = {};



    CarpSwapChainFormats swapchainFormats = {};

    using FnDestroyBuffers = void (*)(CarpVk& carpVk);

    FnDestroyBuffers destroyBuffers = nullptr;
    void* destroyBuffersData = nullptr;

    int64_t frameIndex = 0;

    int queueIndex = -1;
    int swapchainCount = 0;
    int swapchainWidth = 0;
    int swapchainHeight = 0;
};


void deinitCarpVk(CarpVk& carpvk);

void printExtensions();
void printLayers();

VulkanInstanceBuilder instaceBuilder();

VulkanInstanceBuilder& instanceBuilderSetApplicationVersion(
    VulkanInstanceBuilder &builder, int major, int minor, int patch);

VulkanInstanceBuilder& instanceBuilderSetExtensions(
    VulkanInstanceBuilder &builder, const char** extensions, int extensionCount);

VulkanInstanceBuilder& instanceBuilderUseDefaultValidationLayers(VulkanInstanceBuilder &builder);
VulkanInstanceBuilder& instanceBuilderSetDefaultMessageSeverity(VulkanInstanceBuilder &builder);


bool instanceBuilderFinish(VulkanInstanceBuilder &builder, CarpVk& carpVk);


bool createPhysicalDevice(CarpVk& carpVk, bool useIntegratedGpu);
bool createDeviceWithQueues(CarpVk& carpVk, VulkanInstanceBuilder& builder);
bool createSwapchain(CarpVk& carpVk, VSyncType vsyncMode, int width, int height);

bool finalizeInit(CarpVk& carpVk);
