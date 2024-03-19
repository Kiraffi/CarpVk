#pragma once

#include <stdint.h>

static constexpr size_t SizeOfVoidPtr = sizeof(size_t);


#if INTPTR_MAX == INT32_MAX
#define ENV_32_BITS
#elif INTPTR_MAX == INT64_MAX
#define ENV_64_BITS
#else
#error "Environment not 32 or 64-bit."
#endif


#if (defined __x86_64__ && !defined __ILP32__ || (_WIN64))
#define VK_HANDLE(handleName) typedef struct handleName##_T* handleName
#else
#define VK_HANDLE(handleName) typedef uint64_t handleName
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
    void* data = {};
    const char* bufferName = {};
    uint64_t stageMask = 0;
    uint64_t accessMask = 0;
    VmaAllocation_T* allocation = {};
    size_t size = 0ull;
    uint32_t usage = 0;
};

