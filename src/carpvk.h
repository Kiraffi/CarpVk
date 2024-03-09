#pragma once

#include <stdint.h>

#if (defined __x86_64__ && !defined __ILP32__)
#define VK_HANDLE(handleName) typedef struct handleName##_T* handleName
#else
#define VK_HANDLE(handleName) uint64_t
#endif
#define VK_PTR_HANDLE(handleName) typedef struct handleName##_T* handleName


VK_PTR_HANDLE(VkInstance);
VK_PTR_HANDLE(VkPhysicalDevice);
VK_PTR_HANDLE(VkDevice);
VK_PTR_HANDLE(VkQueue);
VK_PTR_HANDLE(VkCommandBuffer);

VK_HANDLE(VkDebugUtilsMessengerEXT);
VK_HANDLE(VkSurfaceKHR);
VK_HANDLE(VkSwapchainKHR);
VK_HANDLE(VkImage);
VK_HANDLE(VkImageView);
VK_HANDLE(VkQueryPool);
VK_HANDLE(VkSemaphore);
VK_HANDLE(VkFence);
VK_HANDLE(VkCommandPool);

VK_DEFINE_HANDLE(VmaAllocator);
VK_DEFINE_HANDLE(VmaAllocation);

VK_HANDLE(VkShaderModule);
VK_HANDLE(VkDescriptorSetLayout);

VK_HANDLE(VkPipeline);
VK_HANDLE(VkPipelineLayout);
VK_HANDLE(VkDescriptorPool);

enum VkFormat;
enum VkImageLayout;
enum VkColorSpaceKHR;

struct Image
{
    VkImage image = {};
    VkImageView view = {};
    VmaAllocation allocation = {};
    const char* imageName = {};
    uint64_t stageMask = 0;
    uint64_t accessMask = 0;
    VkImageLayout layout = {};
    VkFormat format = {};
    int32_t width = 0;
    int32_t height = 0;
};

struct CarpVk;

using FnGetExtraInstanceExtensions = const char *const *(*)(uint32_t* outCount);
using FnCreateSurface = VkSurfaceKHR(*)(VkInstance_T *instance, void *data);
using FnDestroyBuffers = void (*)(void* data);

enum class VSyncType : unsigned char
{
    FIFO_VSYNC,
    IMMEDIATE_NO_VSYNC,
    MAILBOX_VSYNC,
};

struct VulkanInstanceParams
{
    FnGetExtraInstanceExtensions getExtraExtensionsFn = nullptr;
    FnCreateSurface createSurfaceFn = nullptr;
    FnDestroyBuffers destroyBuffersFn = nullptr;
    void *pData = nullptr;
    const char **extensions = nullptr;
    int extensionCount = 0;
    int width = 0;
    int height = 0;
    VSyncType vsyncMode = VSyncType::MAILBOX_VSYNC;
    bool useValidation = false;
    bool useIntegratedGpu = false;
};

struct CarpSwapChainFormats
{
    VkFormat defaultColorFormat;
    VkFormat presentColorFormat;
    VkFormat depthFormat;
    VkColorSpaceKHR colorSpace;
};


struct CarpVk
{
    static const int FramesInFlight = 4;
    static const int QueryCount = 128;
};


bool initVulkan(const VulkanInstanceParams &params);
void deinitVulkan();

void printExtensions();
void printLayers();


VkImageView createImageView(VkImage image, int64_t format);
bool createImage(uint32_t width, uint32_t height,
    int64_t imageFormat, int64_t usage, const char* imageName,
    Image& outImage);

void destroyImage(Image& image);


VkImageMemoryBarrier2 imageBarrier(Image &image,
    uint64_t dstStageMask, uint64_t dstAccessMask, VkImageLayout newLayout);

VkImageMemoryBarrier2 imageBarrier(Image &image,
    uint64_t srcStageMask, uint64_t srcAccessMask, VkImageLayout oldLayout,
    uint64_t dstStageMask, uint64_t dstAccessMask, VkImageLayout newLayout);

VkImageMemoryBarrier2 imageBarrier(VkImage image,
    uint64_t srcStageMask, uint64_t srcAccessMask, VkImageLayout oldLayout,
    uint64_t dstStageMask, uint64_t dstAccessMask, VkImageLayout newLayout,
    uint32_t aspectMask);


struct GPBuilder
{
    const VkPipelineShaderStageCreateInfo* stageInfos = {};
    const VkFormat* colorFormats = {};
    const VkPipelineColorBlendAttachmentState* blendChannels = {};
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;
    VkPipelineLayout pipelineLayout = {};
    
    int stageInfoCount = 0;
    uint32_t colorFormatCount = 0;
    uint32_t blendChannelCount = 0;

    bool depthTest = false;
    bool writeDepth = false;

};


bool createShader(const char* code, int codeSize, VkShaderModule& outModule);


struct DescriptorSetLayout
{
    uint32_t bindingIndex;
    int32_t descriptorType;
    uint32_t stage;
};


VkDescriptorSetLayout createSetLayout(const DescriptorSetLayout* descriptors, int32_t count);

void destroyShaderModule(VkShaderModule* shaderModules, int32_t shaderModuleCount);
void destroyPipeline(VkPipeline* pipelines, int32_t pipelineCount);
void destroyPipelineLayouts(VkPipelineLayout* pipelineLayouts, int32_t pipelineLayoutCount);
void destroyDescriptorPools(VkDescriptorPool* pools, int32_t poolCount);

VkInstance getVkInstance();
VkDevice getVkDevice();
VkCommandBuffer getVkCommandBuffer();
VkDescriptorPool getVkDescriptorPool();
const CarpSwapChainFormats& getSwapChainFormats();


VkPipeline createGraphicsPipeline(const GPBuilder& builder, const char *pipelineName);


bool beginFrame();
bool presentImage(Image& presentImage);





