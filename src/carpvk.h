#pragma once

struct VulkanInstanceBuilder;
struct alignas(sizeof(char*)) VulkanInstanceBuilder
{
    char size[1024];
};

struct VkInstance_T;
struct VkDevice_T;
struct VkDebugUtilsMessengerEXT_T;
struct VkSurfaceKHR_T;
struct VkPhysicalDevice_T;
struct VkQueue_T;
struct CarpSwapChainFormats
{
    int defaultColorFormat;
    int presentColorFormat;
    int depthFormat;
    int colorSpace;
};

struct CarpVk
{

    VkInstance_T* instance = nullptr;
    VkPhysicalDevice_T* physicalDevice = nullptr;
    VkDevice_T* device = nullptr;
    VkDebugUtilsMessengerEXT_T* debugMessenger = nullptr;
    VkSurfaceKHR_T* surface = nullptr;
    VkQueue_T* queue = nullptr;

    CarpSwapChainFormats swapchainFormats = {};

    int queueIndex = -1;
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
