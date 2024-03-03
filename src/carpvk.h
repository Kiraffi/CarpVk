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
struct VmaAllocation_T;

struct VkShaderModule_T;
struct VkDescriptorSetLayout_T;

struct VkPipeline_T;
struct VkPipelineLayout_T;
struct VkDescriptorPool_T;



struct Image
{
    VkImage_T* image = nullptr;
    VkImageView_T* view = nullptr;
    VmaAllocation_T* allocation = nullptr;
    int64_t format = 0;
    int64_t layout = 0;
    int64_t accessMask = 0;
    int32_t width = 0;
    int32_t height = 0;
};


struct alignas(sizeof(char*)) VulkanInstanceBuilder
{
    char size[1024];
};

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

struct CarpVk;
using FnDestroyBuffers = void (*)(CarpVk& carpVk);


struct CarpVk
{
    static const int FramesInFlight = 4;
    static const int QueryCount = 128;

    FnDestroyBuffers destroyBuffers = nullptr;
    void* destroyBuffersData = nullptr;

    /*
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



    int64_t frameIndex = -1;

    int queueIndex = -1;
    int swapchainCount = 0;
    int swapchainWidth = 0;
    int swapchainHeight = 0;
    uint32_t imageIndex = 0;
    */
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


bool instanceBuilderFinish(VulkanInstanceBuilder &builder);


bool createPhysicalDevice(bool useIntegratedGpu);
bool createDeviceWithQueues(VulkanInstanceBuilder& builder);
bool createSwapchain(VSyncType vsyncMode, int width, int height);

bool finalizeInit(CarpVk& carpVk);
VkImageView_T* createImageView(VkImage_T* image, int64_t format);
bool createImage(uint32_t width, uint32_t height,
    int64_t imageFormat, int64_t usage,
    Image& outImage);

void destroyImage(Image& image);


VkImageMemoryBarrier imageBarrier(Image& image,
    VkAccessFlags dstAccessMask, VkImageLayout newLayout);

VkImageMemoryBarrier imageBarrier(Image& image,
    VkAccessFlags srcAccessMask, VkImageLayout oldLayout,
    VkAccessFlags dstAccessMask, VkImageLayout newLayout);

VkImageMemoryBarrier imageBarrier(VkImage_T* image,
    VkAccessFlags srcAccessMask, VkImageLayout oldLayout,
    VkAccessFlags dstAccessMask, VkImageLayout newLayout,
    int64_t aspectMask);


struct GPBuilder
{
    const VkPipelineShaderStageCreateInfo* stageInfos = {};
    const VkFormat* colorFormats = {};
    const VkPipelineColorBlendAttachmentState* blendChannels = {};
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;
    VkPipelineLayout_T* pipelineLayout = {};
    
    int stageInfoCount = 0;
    uint32_t colorFormatCount = 0;
    uint32_t blendChannelCount = 0;

    bool depthTest = false;
    bool writeDepth = false;

};


bool createShader(const char* code, int codeSize, VkShaderModule_T*& outModule);


struct DescriptorSetLayout
{
    uint32_t bindingIndex;
    int32_t descriptorType;
    uint32_t stage;
};


VkDescriptorSetLayout_T* createSetLayout(const DescriptorSetLayout* descriptors, int32_t count);

void destroyShaderModule(VkShaderModule_T** shaderModules, int32_t shaderModuleCount);
void destroyPipeline(VkPipeline_T** pipelines, int32_t pipelineCount);
void destroyPipelineLayouts(VkPipelineLayout_T** pipelineLayouts, int32_t pipelineLayoutCount);
void destroyDescriptorPools(VkDescriptorPool_T** pools, int32_t poolCount);

void setVkSurface(VkSurfaceKHR_T* surface);
VkInstance_T* getVkInstance();
VkDevice_T* getVkDevice();
VkCommandBuffer_T* getVkCommandBuffer();
const CarpSwapChainFormats& getSwapChainFormats();


VkPipeline_T* createGraphicsPipeline(const GPBuilder& builder);


bool beginFrame();
bool presentImage(Image& presentImage);





