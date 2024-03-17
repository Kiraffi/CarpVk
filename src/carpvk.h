#pragma once

#include <stdint.h>

#include "common.h"


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
VK_HANDLE(VkBuffer);
VK_HANDLE(VkImage);
VK_HANDLE(VkImageView);
VK_HANDLE(VkQueryPool);
VK_HANDLE(VkSemaphore);
VK_HANDLE(VkFence);
VK_HANDLE(VkCommandPool);

VK_PTR_HANDLE(VmaAllocator);
VK_PTR_HANDLE(VmaAllocation);

VK_HANDLE(VkShaderModule);
VK_HANDLE(VkDescriptorSetLayout);

VK_HANDLE(VkPipeline);
VK_HANDLE(VkPipelineLayout);
VK_HANDLE(VkDescriptorPool);

VK_HANDLE(VkSampler);

typedef uint32_t VkBool32;
typedef uint64_t VkDeviceAddress;
typedef uint64_t VkDeviceSize;
typedef uint32_t VkFlags;
typedef uint64_t VkFlags64;
typedef uint32_t VkSampleMask;



enum VkFormat;
enum VkImageLayout;
enum VkColorSpaceKHR;
enum VkDescriptorType;

typedef VkFlags VkImageUsageFlags;

typedef VkFlags64 VkPipelineStageFlags2;
typedef VkFlags64 VkAccessFlags2;


struct VkSamplerCreateInfo;


static constexpr VkPipelineColorBlendAttachmentState cDefaultBlendState = {
    .blendEnable = VK_FALSE,
    .colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT
};


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

// resource
struct Buffer
{
    VkBuffer buffer = {};
    // cpu mapped memory for cpu accessbuffers
    void *data = {};
    const char *bufferName = {};
    uint64_t stageMask = 0;
    uint64_t accessMask = 0;
    VmaAllocation_T *allocation = {};
    size_t size = 0ull;
    uint32_t usage = 0;
};

struct BufferCopyRegion
{
    size_t srcOffset = 0;
    size_t dstOffset = 0;
    size_t size = 0;
};


struct UniformBuffer
{
    size_t offset = 0;
    size_t size = 0;
};



struct CarpVk;

using FnGetExtraInstanceExtensions = const char *const *(*)(uint32_t* outCount);
using FnCreateSurface = VkSurfaceKHR(*)(VkInstance_T *instance, void* userData);
using FnDestroyBuffers = void (*)(void* userData);
using FnGetWindowSize = void (*)(int32_t* width, int32_t* height, void* userData);
using FnResized = void (*)(void* userData);
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
    FnGetWindowSize getWindowSizeFn = nullptr;
    FnResized resizedFn = nullptr;
    void* userData = nullptr;
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

struct DescriptorSetLayout
{
    uint32_t bindingIndex = {};
    VkDescriptorType descriptorType = {};
    uint32_t stage = {};
    VkSampler immutableSampler = {};
};

struct DescriptorInfo
{
    enum class DescriptorType
    {
        NOT_VALID,
        IMAGE,
        BUFFER,
    };


    DescriptorInfo(const VkImageView imageView, const VkImageLayout layout, const VkSampler sampler)
    {
        imageInfo.imageView = imageView;
        imageInfo.imageLayout = layout;
        imageInfo.sampler = sampler;
        type = DescriptorType::IMAGE;
    }

    //
    DescriptorInfo(const Buffer &buffer, const VkDeviceSize offset, const VkDeviceSize range)
    {
        ASSERT(range > 0u);
        bufferInfo.buffer = buffer.buffer;
        bufferInfo.offset = offset;
        bufferInfo.range = range;
        type = DescriptorType::BUFFER;
    }
    DescriptorInfo(const Buffer &buffer)
    {
        bufferInfo.buffer = buffer.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = buffer.size;
        type = DescriptorType::BUFFER;
    }

    DescriptorInfo(const UniformBuffer& buffer);

    union
    {
        VkDescriptorImageInfo imageInfo;
        VkDescriptorBufferInfo bufferInfo;
    };

    DescriptorType type = DescriptorType::NOT_VALID;
};

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

struct CPBuilder
{
    VkPipelineShaderStageCreateInfo stageInfo = {};
    VkPipelineLayout pipelineLayout = {};
};


struct RenderingAttachmentInfo
{
    VkClearValue clearValue = {};
    Image *image = nullptr;
    int32_t loadOp = 0;
    int32_t storeOp = 0;
};

bool initVulkan(const VulkanInstanceParams &params);
void deinitVulkan();

void printExtensions();
void printLayers();

bool createDescriptorSet(VkDescriptorSetLayout layout, VkDescriptorSet* outSet);

VkImageView createImageView(VkImage image, VkFormat format);
bool createImage(uint32_t width, uint32_t height,
    VkFormat imageFormat, VkImageUsageFlags usage, const char* imageName,
    Image& outImage);

void destroyImage(Image& image);
bool createBuffer(size_t size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags memoryFlags,
    const char* bufferName,
    Buffer &outBuffer);
void destroyBuffer(Buffer& buffer);

void uploadToGpuBuffer(Buffer &gpuBuffer, const void *data, size_t dstOffset, size_t size);
void uploadToUniformBuffer(UniformBuffer &uniformBuffer, const void *data, size_t size);

void uploadToImage(u32 width, u32 height, u32 pixelSize,
    Image& targetImage, void* data, u32 dataSize);

bool updateBindDescriptorSet(VkDescriptorSet descriptorSet,
    const DescriptorSetLayout* descriptorSetLayout,
    const DescriptorInfo* descriptorSetInfos, int descriptorSetInfoCount);




void imageBarrier(Image &image,
    VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask, VkImageLayout newLayout);

void imageBarrier(Image &image,
    VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask, VkImageLayout oldLayout,
    VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask, VkImageLayout newLayout);

void imageBarrier(VkImage image,
    VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask, VkImageLayout oldLayout,
    VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask, VkImageLayout newLayout,
    uint32_t aspectMask);

void bufferBarrier(UniformBuffer& buffer,
     VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask);

void bufferBarrier(Buffer& buffer,
     VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask);

void bufferBarrier(VkBuffer buffer,
    VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
    VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask,
    size_t size, size_t offset);

bool createShader(const char* code, int codeSize, VkShaderModule& outModule);
bool createShader(const char* filename, VkShaderModule& outModule);


VkDescriptorSetLayout createSetLayout(const DescriptorSetLayout* descriptors, int32_t count);

VkPipelineLayout createPipelineLayout(const VkDescriptorSetLayout descriptorSetLayout);
void destroyShaderModule(VkShaderModule* shaderModules, int32_t shaderModuleCount);
void destroyPipeline(VkPipeline* pipelines, int32_t pipelineCount);
void destroyPipelineLayouts(VkPipelineLayout* pipelineLayouts, int32_t pipelineLayoutCount);
void destroyDescriptorPools(VkDescriptorPool* pools, int32_t poolCount);

VkSampler createSampler(const VkSamplerCreateInfo& info);
void destroySampler(VkSampler &sampler);

VkInstance getVkInstance();
VkDevice getVkDevice();
VkCommandBuffer getVkCommandBuffer();
VkDescriptorPool getVkDescriptorPool();
const CarpSwapChainFormats& getSwapChainFormats();

VkPipeline createGraphicsPipeline(const GPBuilder& builder, const char* pipelineName);
VkPipeline createComputePipeline(const CPBuilder& builder, const char* pipelineName);

bool beginFrame();
bool presentImage(Image& presentImage);
void beginPreFrame();
void endPreFrame();

VkPipelineShaderStageCreateInfo createDefaultVertexInfo(VkShaderModule module);
VkPipelineShaderStageCreateInfo createDefaultFragmentInfo(VkShaderModule module);
VkPipelineShaderStageCreateInfo createDefaultComputeInfo(VkShaderModule module);

UniformBuffer createUniformBuffer(size_t size);

void beginRenderPipeline(RenderingAttachmentInfo *colorTargets, int32_t colorTargetCount,
    RenderingAttachmentInfo *depthTarget,
    VkPipelineLayout pipelineLayout, VkPipeline pipeline, VkDescriptorSet descriptorSet);

void endRenderPipeline();

void beginComputePipeline(VkPipelineLayout pipelineLayout, VkPipeline pipeline, VkDescriptorSet descriptorSet);
void endComputePipeline();

void flushBarriers();

Buffer& getUniformBuffer();
