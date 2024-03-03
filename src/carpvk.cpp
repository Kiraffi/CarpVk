#include <stdio.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "carpvk.h"

#define VMA_IMPLEMENTATION

#ifdef __clang__
#pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wtautological-compare" // comparison of unsigned expression < 0 is always false
    #pragma clang diagnostic ignored "-Wunused-private-field"
    #pragma clang diagnostic ignored "-Wunused-parameter"
    #pragma clang diagnostic ignored "-Wmissing-field-initializers"
    #pragma clang diagnostic ignored "-Wnullability-completeness"
#endif

#include "vk_mem_alloc.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "common.h"

static const uint32_t sVulkanApiVersion = VK_API_VERSION_1_3;


static VkInstance sVkInstance = {};



static VkPhysicalDevice_T* sVkPhysicalDevice = {};
static VkDevice_T* sVkDevice = {};
static VkDebugUtilsMessengerEXT_T* sVkDebugMessenger = {};
static VkSurfaceKHR_T* sVkSurface = {};
static VkQueue_T* sVkQueue = {};
static VkSwapchainKHR_T* sVkSwapchain = {};
static VkImage_T* sVkSwapchainImages[16] = {};
static VkImageView_T* sVkSwapchainImagesViews[16] = {};

static VkQueryPool_T* sVkQueryPools[CarpVk::FramesInFlight] = {};
static int sVkQueryPoolIndexCounts[CarpVk::FramesInFlight] = {};

static VkSemaphore_T* sVkAcquireSemaphores[CarpVk::FramesInFlight] = {};
static VkSemaphore_T* sVkReleaseSemaphores[CarpVk::FramesInFlight] = {};

static VkFence_T* sVkFences[CarpVk::FramesInFlight] = {};
static VkCommandPool_T* sVkCommandPool = {};

static VkCommandBuffer_T* sVkCommandBuffers[CarpVk::FramesInFlight] = {};

static VmaAllocator_T* sVkAllocator = {};

static CarpSwapChainFormats sVkSwapchainFormats = {};

//using FnDestroyBuffers = void (*)(CarpVk& carpVk);

//static FnDestroyBuffers sVkDestroyBuffers = nullptr;
//static void* sVkDestroyBuffersData = nullptr;

static int64_t sVkFrameIndex = -1;

static int sVkQueueIndex = -1;
static int sVkSwapchainCount = 0;
static int sVkSwapchainWidth = 0;
static int sVkSwapchainHeight = 0;
static uint32_t sVkImageIndex = 0;


static PFN_vkDebugMarkerSetObjectTagEXT debugMarkerSetObjectTag = nullptr;
static PFN_vkDebugMarkerSetObjectNameEXT debugMarkerSetObjectName = nullptr;
static PFN_vkCmdDebugMarkerBeginEXT cmdDebugMarkerBegin = nullptr;
static PFN_vkCmdDebugMarkerEndEXT cmdDebugMarkerEnd = nullptr;
static PFN_vkCmdDebugMarkerInsertEXT cmdDebugMarkerInsert = nullptr;








struct VulkanInstanceBuilderImpl
{
    VkApplicationInfo m_appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Test app",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
        .pEngineName = "carp engine",
        .engineVersion = VK_MAKE_VERSION(0, 0, 1),
        .apiVersion = sVulkanApiVersion,
    } ;

    VkInstanceCreateInfo m_createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &m_appInfo,
    };

    VkDebugUtilsMessengerCreateInfoEXT m_debugCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    };

    VkValidationFeaturesEXT m_validationFeatures = {
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
    };
};

static_assert(sizeof(VulkanInstanceBuilder) >= sizeof(VulkanInstanceBuilderImpl));
static_assert(alignof(VulkanInstanceBuilder) == alignof(VulkanInstanceBuilderImpl));

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities = {};
    VkSurfaceFormatKHR formats[256];
    VkPresentModeKHR presentModes[256];
    uint32_t formatCount = 0;
    uint32_t presentModeCount = 0;
};


struct SwapChainFormats
{
    VkFormat color;
    VkFormat depth;
    VkColorSpaceKHR colorSpace;
};

static constexpr u32 sFormatFlagBits =
    VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
    VK_FORMAT_FEATURE_BLIT_SRC_BIT |
    VK_FORMAT_FEATURE_BLIT_DST_BIT |
    VK_FORMAT_FEATURE_TRANSFER_SRC_BIT |
    VK_FORMAT_FEATURE_TRANSFER_DST_BIT |
    VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT |
    VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;

static constexpr SwapChainFormats sDefaultPresent[] = {
    { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_D32_SFLOAT, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
    { VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_D32_SFLOAT, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
    { VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_FORMAT_D32_SFLOAT, VK_COLOR_SPACE_HDR10_ST2084_EXT },
};

static constexpr SwapChainFormats sDefaultFormats[] = {
    { VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_D32_SFLOAT, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
    { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_D32_SFLOAT, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
};



#define VK_CHECK_CALL(call) do { \
    VkResult callResult = call; \
    if(callResult != VkResult::VK_SUCCESS) \
        printf("Result: %i\n", int(callResult)); \
    ASSERT(callResult == VkResult::VK_SUCCESS); \
    } while(0)


static const VkValidationFeatureEnableEXT sEnabledValidationFeatures[] =
{
    //VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
    //VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
    VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
    //VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
    VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
};

static const char* sDeviceExtensions[] =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    // Cannot use, renderdoc stops working
    // VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
    // VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,

    //VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME,

    // VK_KHR_MAINTENANCE1_EXTENSION_NAME
    // VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME
};


static const char* sDefaultValidationLayers[] =
{
    "VK_LAYER_KHRONOS_validation",
    "VK_LAYER_LUNARG_monitor",
    "VK_LAYER_KHRONOS_synchronization2",


    //"VK_LAYER_NV_optimus",
    //"VK_LAYER_RENDERDOC_Capture",
    //"VK_LAYER_KHRONOS_profiles",
    //"VK_LAYER_LUNARG_screenshot",
    //"VK_LAYER_LUNARG_gfxreconstruct",
    //"VK_LAYER_LUNARG_api_dump"
};

static void sSetObjectName(uint64_t object, VkDebugReportObjectTypeEXT objectType, const char* name)
{
    // Check for a valid function pointer
    if (debugMarkerSetObjectName)
    {
        VkDebugMarkerObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = objectType;
        nameInfo.object = object;
        nameInfo.pObjectName = name;
        debugMarkerSetObjectName(sVkDevice, &nameInfo);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL sDebugReportCB(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    bool errorMsg = (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0
                    || (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0;
    bool warningMsg = (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0
                      || (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) != 0;

    // Remove infos
    if(!errorMsg && !warningMsg)
    {
        return VK_FALSE;
    }
    const char *type = errorMsg ? "Error" : (warningMsg  ? "Warning" : "Info");
    printf("Type:%s, message: %s\n\n", type, pCallbackData->pMessage);
    if(errorMsg)
    {
        ASSERT(!"Validation error encountered!");
    }
    return VK_FALSE;
}


static VkImageAspectFlags sGetAspectMaskFromFormat(VkFormat format)
{
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    switch(format)
    {
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_X8_D24_UNORM_PACK32:
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D16_UNORM_S8_UINT:
        {
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            break;
        }

        case VK_FORMAT_UNDEFINED:
        {
            ASSERT(!"Undefined format");
            aspectMask = VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;
        }

        default:
        {
            break;
        }
    }
    return aspectMask;
}



static SwapChainSupportDetails sQuerySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

    u32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    ASSERT(formatCount > 0 && formatCount < 256);
    details.formatCount = formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats);

    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    ASSERT(presentModeCount > 0 && presentModeCount < 256);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes);
    details.presentModeCount = presentModeCount;

    return details;
}

// Returns index that has all bits and supports present.
static int sFindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    ASSERT(queueFamilyCount < 256);
    VkQueueFamilyProperties queueFamilies[256];
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);

    int index = -1;
    for (int i = 0; i < queueFamilyCount; ++i)
    {
        u32 queueBits = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT;

        // make everything into same queue
        if ((queueFamilies[i].queueFlags & queueBits) != queueBits)
            continue;

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

        if (!presentSupport)
            continue;

        return i;
    }
    return -1;
}




static void sDestroySwapchain()
{
    for(int i = 0; i < sVkSwapchainCount; ++i)
    {
        vkDestroyImageView(sVkDevice, sVkSwapchainImagesViews[i], nullptr);
    }

    sVkSwapchainCount = 0u;
    sVkSwapchainWidth = sVkSwapchainHeight = 0u;

    if(sVkSwapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(sVkDevice, sVkSwapchain, nullptr);
    sVkSwapchain = VK_NULL_HANDLE;
}





static VkSemaphore sCreateSemaphore()
{
    VkSemaphore semaphore = {};
    VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    VK_CHECK_CALL(vkCreateSemaphore(sVkDevice, &semaphoreInfo, nullptr, &semaphore));
    return semaphore;
}

static VkCommandPool sCreateCommandPool()
{
    u32 familyIndex = sVkQueueIndex;
    VkCommandPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    //poolCreateInfo.flags = 0; //VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolCreateInfo.queueFamilyIndex = familyIndex;

    VkCommandPool commandPool = {};
    VK_CHECK_CALL(vkCreateCommandPool(sVkDevice, &poolCreateInfo, nullptr, &commandPool));

    return commandPool;
}



static VkQueryPool sCreateQueryPool(u32 queryCount)
{
    VkQueryPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };

    createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    createInfo.queryCount = queryCount;

    VkQueryPool pool = {};
    VK_CHECK_CALL(vkCreateQueryPool(sVkDevice, &createInfo, nullptr, &pool));

    ASSERT(pool);
    return pool;
}


static VkFence sCreateFence()
{
    VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkFence result = {};
    VK_CHECK_CALL(vkCreateFence(sVkDevice, &createInfo, nullptr, &result));
    ASSERT(result);
    return result;
}



static void sAcquireDeviceDebugRenderdocFunctions(VkDevice device)
{
    debugMarkerSetObjectTag = (PFN_vkDebugMarkerSetObjectTagEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectTagEXT");
    debugMarkerSetObjectName = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT");
    cmdDebugMarkerBegin = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT");
    cmdDebugMarkerEnd = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT");
    cmdDebugMarkerInsert = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT");
}







void deinitCarpVk(CarpVk& carpVk)
{
    if (sVkInstance == nullptr)
    {
        return;
    }
    if (sVkDevice)
    {
        VK_CHECK_CALL(vkDeviceWaitIdle(sVkDevice));

        if(carpVk.destroyBuffers)
        {
            carpVk.destroyBuffers(carpVk);
        }

        vkDestroyCommandPool(sVkDevice, sVkCommandPool, nullptr);
        sDestroySwapchain();



        for(u32 i = 0; i < CarpVk::FramesInFlight; ++i)
        {
            vkDestroyQueryPool(sVkDevice, sVkQueryPools[i], nullptr);
        }


        for(u32 i = 0; i < CarpVk::FramesInFlight; ++i)
        {
            vkDestroyFence(sVkDevice, sVkFences[i], nullptr);
            vkDestroySemaphore(sVkDevice, sVkAcquireSemaphores[i], nullptr);
            vkDestroySemaphore(sVkDevice, sVkReleaseSemaphores[i], nullptr);
        }

        if (sVkAllocator)
        {
            vmaDestroyAllocator(sVkAllocator);
            sVkAllocator = nullptr;
        }


        vkDestroyDevice(sVkDevice, nullptr);
        sVkDevice = nullptr;
    }


    if (sVkSurface)
    {
        vkDestroySurfaceKHR(sVkInstance, sVkSurface, nullptr);
        sVkSurface = nullptr;
    }
    if(sVkDebugMessenger)
    {
        auto dest = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(sVkInstance, "vkDestroyDebugUtilsMessengerEXT");
        dest(sVkInstance, sVkDebugMessenger, nullptr);
        sVkDebugMessenger = nullptr;
    }

    vkDestroyInstance(sVkInstance, nullptr);
}

void printExtensions()
{
    VkExtensionProperties allExtensions[256] = {};
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    ASSERT(extensionCount < 256);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, allExtensions);

    for(int i = 0; i < extensionCount; ++i)
    {
        printf("Extenstion: %s\n", allExtensions[i].extensionName);
    }
    printf("Extensions: %i\n", extensionCount);
}

void printLayers()
{
    VkLayerProperties allLayers[256] = {};
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    ASSERT(layerCount < 256);
    vkEnumerateInstanceLayerProperties(&layerCount, allLayers);
    for(int i = 0; i < layerCount; ++i)
    {
        printf("Layer: %s\n", allLayers[i].layerName);
    }
    printf("Layers: %i\n", layerCount);
}




VulkanInstanceBuilder instaceBuilder()
{
    VulkanInstanceBuilderImpl newBuilder;
    VulkanInstanceBuilder builder = {};
    memcpy(&builder, &newBuilder, sizeof(VulkanInstanceBuilderImpl));
    return builder;
}


VulkanInstanceBuilderImpl* sGetBuilderCasted(VulkanInstanceBuilder& builder)
{
    return (VulkanInstanceBuilderImpl*)(&builder);
}

VulkanInstanceBuilder& instanceBuilderSetApplicationVersion(
    VulkanInstanceBuilder &builder, int major, int minor, int patch)
{
    VulkanInstanceBuilderImpl *casted =  sGetBuilderCasted(builder);
    casted->m_appInfo.applicationVersion = VK_MAKE_VERSION(major, minor, patch);
    return builder;
}

VulkanInstanceBuilder& instanceBuilderSetExtensions(
    VulkanInstanceBuilder &builder, const char** extensions, int extensionCount)
{
    VulkanInstanceBuilderImpl *casted =  sGetBuilderCasted(builder);

    casted->m_createInfo.ppEnabledExtensionNames = extensions;
    casted->m_createInfo.enabledExtensionCount = extensionCount;

    return builder;
}

VulkanInstanceBuilder& instanceBuilderUseDefaultValidationLayers(VulkanInstanceBuilder &builder)
{
    VulkanInstanceBuilderImpl *casted =  sGetBuilderCasted(builder);

    casted->m_createInfo.ppEnabledLayerNames = sDefaultValidationLayers;
    casted->m_createInfo.enabledLayerCount = ARRAYSIZES(sDefaultValidationLayers);

    casted->m_debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                                      | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                      | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    casted->m_debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                  | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                  | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    casted->m_debugCreateInfo.pfnUserCallback = sDebugReportCB;


    casted->m_validationFeatures.enabledValidationFeatureCount = ARRAYSIZES(sEnabledValidationFeatures);
    casted->m_validationFeatures.pEnabledValidationFeatures = sEnabledValidationFeatures;
    casted->m_debugCreateInfo.pNext = &casted->m_validationFeatures;

    casted->m_createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &casted->m_debugCreateInfo;


    return builder;

}

VkInstance sCreateInstance(VulkanInstanceBuilder &builder)
{
    VkInstance result = {};
    VulkanInstanceBuilderImpl *casted =  sGetBuilderCasted(builder);
    casted->m_createInfo.pApplicationInfo = &casted->m_appInfo;
    VK_CHECK_CALL(vkCreateInstance(&casted->m_createInfo, nullptr, &result));
    return result;
}

static VkDebugUtilsMessengerEXT sRegisterDebugCB(VkInstance vkInstance)
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo =
        { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                                 | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                 | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = sDebugReportCB;

    VkDebugUtilsMessengerEXT debugMessenger = nullptr;

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
        vkInstance, "vkCreateDebugUtilsMessengerEXT");
    ASSERT(func);

    VK_CHECK_CALL(func(vkInstance, &createInfo, nullptr, &debugMessenger));
    return debugMessenger;
}


bool instanceBuilderFinish(VulkanInstanceBuilder &builder)
{
    VkInstance result = sCreateInstance(builder);
    if(result == nullptr)
    {
        return false;
    }
    sVkInstance = result;
    VulkanInstanceBuilderImpl *casted =  sGetBuilderCasted(builder);
    if(casted->m_createInfo.enabledLayerCount > 0)
    {
        VkDebugUtilsMessengerEXT debugMessenger = sRegisterDebugCB(result);
        if (debugMessenger == nullptr)
        {
            return false;
        }
        sVkDebugMessenger = debugMessenger;
    }

    return true;
}























bool createPhysicalDevice(bool useIntegratedGpu)
{
    VkPhysicalDeviceType wantedDeviceType = useIntegratedGpu
        ? VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
        : VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

    VkPhysicalDevice devices[256] = {};
    u32 count = 0;

    VK_CHECK_CALL(vkEnumeratePhysicalDevices(sVkInstance, &count, nullptr));
    ASSERT(count < 256);
    VK_CHECK_CALL(vkEnumeratePhysicalDevices(sVkInstance, &count, devices));

    VkPhysicalDevice primary = nullptr;
    VkPhysicalDevice secondary = nullptr;

    int primaryQueueIndex = -1;
    int secondaryQueueIndex = -1;


    for(u32 i = 0; i < count; ++i)
    {
        int requiredExtensionCount = ARRAYSIZES(sDeviceExtensions);

        VkPhysicalDeviceProperties prop;
        VkPhysicalDevice physicalDevice = devices[i];
        vkGetPhysicalDeviceProperties(physicalDevice, &prop);
        if(prop.apiVersion < sVulkanApiVersion)
        {
            printf("Api of device is older than required for %s\n", prop.deviceName);
            continue;
        }
        int queueIndex = sFindQueueFamilies(physicalDevice, sVkSurface);
        if(queueIndex == -1)
        {
            printf("No required queue indices found or they are not all possible to be in same queue: %s\n",
                prop.deviceName);
            continue;
        }

        if(!prop.limits.timestampComputeAndGraphics)
        {
            printf("No timestamp and queries for %s\n", prop.deviceName);
            continue;
        }
        SwapChainSupportDetails swapChainSupport = sQuerySwapChainSupport(physicalDevice, sVkSurface);
        bool swapChainAdequate = swapChainSupport.formatCount > 0 && swapChainSupport.presentModeCount > 0;
        if(!swapChainAdequate)
        {
            printf("No swapchain for: %s\n", prop.deviceName);
            continue;
        }
        u32 formatIndex = ~0u;

        for (u32 j = 0; j < ARRAYSIZES(sDefaultFormats); ++j)
        {
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, sDefaultFormats[j].color, &formatProperties);
            if(((formatProperties.optimalTilingFeatures) & sFormatFlagBits) == sFormatFlagBits)
            {
                formatIndex = j;
                break;
            }
        }

        if(formatIndex == ~0u)
        {
            printf("No render target format found: %s\n", prop.deviceName);
            continue;
        }
        ASSERT(formatIndex != ~0u);


        {
            u32 extensionCount;
            vkEnumerateDeviceExtensionProperties(
                physicalDevice,
                nullptr,
                &extensionCount,
                nullptr);

            VkExtensionProperties availableExtensions[256] = {};
            ASSERT(extensionCount < 256);

            vkEnumerateDeviceExtensionProperties(
                physicalDevice,
                nullptr,
                &extensionCount,
                availableExtensions);

            for (const char* requiredExtension : sDeviceExtensions)
            {
                for (const auto &extension: availableExtensions)
                {
                    if(strcmp(extension.extensionName, requiredExtension) == 0)
                    {
                        requiredExtensionCount--;
                        break;
                    }
                }
            }
        }
        if(requiredExtensionCount != 0)
        {
            printf("No required extension support found for: %s\n", prop.deviceName);
            continue;
        }
        if(prop.deviceType == wantedDeviceType)
        {
            primary = secondary = devices[i];
            primaryQueueIndex = queueIndex;
            break;
        }
        else if(!secondary &&
                (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ||
                 prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))
        {
            secondary = devices[i];
            secondaryQueueIndex = queueIndex;
        }
    }
    if(!primary && !secondary)
    {
        printf("Didn't find any gpus\n");
        return false;
    }
    if(primary)
    {
        sVkPhysicalDevice = primary;
        sVkQueueIndex = primaryQueueIndex;
    }
    else
    {
        sVkPhysicalDevice = secondary;
        sVkQueueIndex = secondaryQueueIndex;
    }

    VkPhysicalDeviceProperties prop;
    vkGetPhysicalDeviceProperties(sVkPhysicalDevice, &prop);

    const char *typeText = prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "discrete" : "integrated";
    printf("Picking %s device: %s\n", typeText, prop.deviceName);
    return true;
}


bool createDeviceWithQueues(VulkanInstanceBuilder& builder)
{
    SwapChainSupportDetails swapChainSupport = sQuerySwapChainSupport(sVkPhysicalDevice, sVkSurface);
    bool swapChainAdequate = swapChainSupport.formatCount > 0 && swapChainSupport.presentModeCount > 0;
    if(!swapChainAdequate)
        return false;

    sVkSwapchainFormats.presentColorFormat = VK_FORMAT_UNDEFINED;
    sVkSwapchainFormats.depthFormat = sDefaultPresent[0].depth;
    sVkSwapchainFormats.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    sVkSwapchainFormats.defaultColorFormat = VK_FORMAT_UNDEFINED;

    for(u32 i = 0; i < swapChainSupport.formatCount
    && sVkSwapchainFormats.presentColorFormat == VK_FORMAT_UNDEFINED; ++i)
    {
        for (const auto& format : sDefaultPresent)
        {
            if(swapChainSupport.formats[i].colorSpace != format.colorSpace)
                continue;
            if(swapChainSupport.formats[i].format != format.color)
                continue;
            sVkSwapchainFormats.presentColorFormat = format.color;
            sVkSwapchainFormats.depthFormat = format.depth;
            sVkSwapchainFormats.colorSpace = format.colorSpace;
        }
    }

    if(sVkSwapchainFormats.presentColorFormat == VK_FORMAT_UNDEFINED
       && swapChainSupport.formatCount == 1
       && swapChainSupport.formats[0].format == VK_FORMAT_UNDEFINED)
    {
        sVkSwapchainFormats.presentColorFormat = sDefaultPresent[0].color;
        sVkSwapchainFormats.colorSpace = sDefaultPresent[0].colorSpace;
        sVkSwapchainFormats.depthFormat = sDefaultPresent[0].depth;
    }
    ASSERT(sVkSwapchainFormats.presentColorFormat != VK_FORMAT_UNDEFINED);
    if(sVkSwapchainFormats.presentColorFormat == VK_FORMAT_UNDEFINED)
    {
        return false;
    }
    for (const auto &format : sDefaultFormats)
    {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(sVkPhysicalDevice, format.color, &formatProperties);
        if (((formatProperties.optimalTilingFeatures) & sFormatFlagBits) == sFormatFlagBits)
        {
            sVkSwapchainFormats.defaultColorFormat = format.color;
            break;
        }
    }

    ASSERT(sVkSwapchainFormats.defaultColorFormat != VK_FORMAT_UNDEFINED);
    if(sVkSwapchainFormats.defaultColorFormat == VK_FORMAT_UNDEFINED)
    {
        return false;
    }

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = sVkQueueIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    static constexpr VkPhysicalDeviceFeatures deviceFeatures = {
        //.fillModeNonSolid = VK_TRUE,
        .samplerAnisotropy = VK_FALSE,
    };
    /*
    static constexpr VkPhysicalDeviceShaderObjectFeaturesEXT shaderObjectFeature = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT,
        .shaderObject = VK_TRUE,
    };
    */
    static constexpr VkPhysicalDeviceVulkan13Features deviceFeatures13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = nullptr, //(void *) &shaderObjectFeature,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
    };

    static constexpr VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = (void *) &deviceFeatures13,
        .features = deviceFeatures,
    };

    VkDeviceCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    createInfo.pNext = &physicalDeviceFeatures2;

    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;



    createInfo.pEnabledFeatures = nullptr;
    /*
        deviceExts.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
    */
    createInfo.enabledExtensionCount = ARRAYSIZES(sDeviceExtensions);
    createInfo.ppEnabledExtensionNames = sDeviceExtensions;

    VulkanInstanceBuilderImpl *casted =  sGetBuilderCasted(builder);
    createInfo.ppEnabledLayerNames = casted->m_createInfo.ppEnabledLayerNames;
    createInfo.enabledLayerCount = casted->m_createInfo.enabledLayerCount;

    VK_CHECK_CALL(vkCreateDevice(sVkPhysicalDevice, &createInfo,
        nullptr, &sVkDevice));

    ASSERT(sVkDevice);

    if (!sVkDevice)
        return false;

    vkGetDeviceQueue(sVkDevice, 0, sVkQueueIndex, &sVkQueue);
    ASSERT(sVkQueue);


    if (!sVkDevice || !sVkQueue)
        return false;
    // Init VMA
    {
        VmaVulkanFunctions vulkanFunctions = {};
        vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.vulkanApiVersion = sVulkanApiVersion;
        allocatorCreateInfo.physicalDevice = sVkPhysicalDevice;
        allocatorCreateInfo.device = sVkDevice;
        allocatorCreateInfo.instance = sVkInstance;
        allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

        VK_CHECK_CALL(vmaCreateAllocator(&allocatorCreateInfo, &sVkAllocator));
        if (!sVkAllocator)
            return false;
    }

    //if(optionals.canUseVulkanRenderdocExtensionMarker)
    //    sAcquireDeviceDebugRenderdocFunctions(vulk->device);
    return true;
}



bool createSwapchain(VSyncType vsyncMode, int width, int height)
{
    VkPresentModeKHR findPresentMode = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
    switch (vsyncMode)
    {
        case VSyncType::FIFO_VSYNC: findPresentMode = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR; break;
        case VSyncType::IMMEDIATE_NO_VSYNC: findPresentMode = VkPresentModeKHR::VK_PRESENT_MODE_IMMEDIATE_KHR; break;
        case VSyncType::MAILBOX_VSYNC: findPresentMode = VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR; break;
    }
    {
        SwapChainSupportDetails swapChainSupport = sQuerySwapChainSupport(sVkPhysicalDevice, sVkSurface);
        ASSERT(swapChainSupport.formatCount > 0);
        VkSurfaceFormatKHR surfaceFormat = swapChainSupport.formats[0];
        bool found = false;
        for (const auto& availableFormat : swapChainSupport.formats)
        {
            if (availableFormat.format == sVkSwapchainFormats.presentColorFormat
                && availableFormat.colorSpace == sVkSwapchainFormats.colorSpace)
            {
                surfaceFormat = availableFormat;
                found = true;
                break;
            }
        }
        if(!found && swapChainSupport.formatCount == 1
            && swapChainSupport.formats[0].format == VK_FORMAT_UNDEFINED)
        {
            surfaceFormat.colorSpace = (VkColorSpaceKHR)sVkSwapchainFormats.colorSpace;
            surfaceFormat.format = (VkFormat)sVkSwapchainFormats.presentColorFormat;
            found = true;
        }
        ASSERT(found);

        VkPresentModeKHR presentMode = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
        for (const auto& availablePresentMode : swapChainSupport.presentModes)
        {
            if (availablePresentMode == findPresentMode)
            {
                presentMode = availablePresentMode;
                break;
            }
        }

        VkExtent2D extent = swapChainSupport.capabilities.currentExtent;
        if (swapChainSupport.capabilities.currentExtent.width == UINT32_MAX)
        {
            extent.width = u32(width);
            extent.height = u32(height);


            extent.width = MAX_VALUE(swapChainSupport.capabilities.minImageExtent.width,
                MIN_VALUE(swapChainSupport.capabilities.maxImageExtent.width, extent.width));
            extent.height = MAX_VALUE(swapChainSupport.capabilities.minImageExtent.height,
                MIN_VALUE(swapChainSupport.capabilities.maxImageExtent.height, extent.height));
        }

        u32 imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0
            && imageCount > swapChainSupport.capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        createInfo.surface = sVkSurface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT
            //| VK_IMAGE_USAGE_SAMPLED_BIT
            //| VK_IMAGE_USAGE_STORAGE_BIT
            | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.oldSwapchain = sVkSwapchain;

        // Since everything is using single queueindex we dont have to check for multiple.
        {
            // Setting this to 1 + giving queuefamilyindices pointer
            // caused crash with VkSwapchainCreateInfoKHR with AMD
            createInfo.queueFamilyIndexCount = 0;
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        VkSwapchainKHR swapchain = nullptr;
        //    PreCallValidateCreateSwapchainKHR()
        VkResult res = vkCreateSwapchainKHR(sVkDevice, &createInfo, nullptr, &swapchain);
        VK_CHECK_CALL(res);
        if (res != VK_SUCCESS)
        {
            printf("Failed to initialize swapchain\n");
            return false;
        }
        sVkSwapchain = swapchain;

        sVkSwapchainFormats.colorSpace = surfaceFormat.colorSpace;
        sVkSwapchainFormats.presentColorFormat = surfaceFormat.format;

        sVkSwapchainWidth = extent.width;
        sVkSwapchainHeight = extent.height;

    }





    u32 swapchainCount = 0;
    VK_CHECK_CALL(vkGetSwapchainImagesKHR(sVkDevice, sVkSwapchain, &swapchainCount, nullptr));

    VK_CHECK_CALL(vkGetSwapchainImagesKHR(sVkDevice, sVkSwapchain, &swapchainCount, sVkSwapchainImages));
    sVkSwapchainCount = swapchainCount;

    for(int i = 0; i < swapchainCount; ++i)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = sVkSwapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = (VkFormat)sVkSwapchainFormats.presentColorFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VK_CHECK_CALL(vkCreateImageView(sVkDevice, &createInfo, nullptr, &sVkSwapchainImagesViews[i]));
    }

    return true;
}

bool finalizeInit(CarpVk& carpVk)
{
    for(u32 i = 0; i < CarpVk::FramesInFlight; ++i)
    {
        sVkQueryPools[i] = sCreateQueryPool(CarpVk::QueryCount);
        ASSERT(sVkQueryPools[i]);
        if(!sVkQueryPools[i])
        {
            printf("Failed to create vulkan query pool!\n");
            return false;
        }
    }


    for(u32 i = 0; i < CarpVk::FramesInFlight; ++i)
    {
        sVkAcquireSemaphores[i] = sCreateSemaphore();
        ASSERT(sVkAcquireSemaphores[i]);
        if(!sVkAcquireSemaphores[i])
        {
            printf("Failed to create vulkan acquire semapohore!\n");
            return false;
        }

        sVkReleaseSemaphores[i] = sCreateSemaphore();
        ASSERT(sVkReleaseSemaphores[i]);
        if(!sVkReleaseSemaphores[i])
        {
            printf("Failed to create vulkan release semaphore!\n");
            return false;
        }

        sVkFences[i] = sCreateFence();
        ASSERT(sVkFences[i]);
        if(!sVkFences[i])
        {
            printf("Failed to create vulkan fence!\n");
            return false;
        }
    }
    sVkCommandPool = sCreateCommandPool();
    ASSERT(sVkCommandPool);
    if(!sVkCommandPool)
    {
        printf("Failed to create vulkan command pool!\n");
        return false;
    }


    VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocateInfo.commandPool = sVkCommandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = CarpVk::FramesInFlight;

    {
        VK_CHECK_CALL(vkAllocateCommandBuffers(sVkDevice, &allocateInfo, sVkCommandBuffers));
        for(u32 i = 0; i < CarpVk::FramesInFlight; ++i)
        {
            if(!sVkCommandBuffers[i])
            {
                printf("Failed to create vulkan command buffer!\n");
                return false;
            }

            static const char* s = "Main command buffer";
            sSetObjectName((uint64_t)sVkCommandBuffers[i], VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, s);
        }
    }


    return true;
}











VkImageView_T* createImageView(VkImage_T* image, int64_t format)
{
    VkImageAspectFlags aspectMask = sGetAspectMaskFromFormat((VkFormat)format);

    VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = (VkFormat)format;

    //createInfo.components.r = VK_COMPONENT_SWIZZLE_R; // VK_COMPONENT_SWIZZLE_IDENTITY;
    //createInfo.components.g = VK_COMPONENT_SWIZZLE_G; //VK_COMPONENT_SWIZZLE_IDENTITY;
    //createInfo.components.b = VK_COMPONENT_SWIZZLE_B; //VK_COMPONENT_SWIZZLE_IDENTITY;
    //createInfo.components.a = VK_COMPONENT_SWIZZLE_A; //VK_COMPONENT_SWIZZLE_IDENTITY;

    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    createInfo.subresourceRange.aspectMask = aspectMask;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    VkImageView view = {};
    VK_CHECK_CALL(vkCreateImageView(sVkDevice, &createInfo, nullptr, &view));

    ASSERT(view);
    return view;
}


bool createImage(uint32_t width, uint32_t height,
    int64_t imageFormat, int64_t usage, const char* imageName,
    Image& outImage)
{
    VkImageCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.format = (VkFormat)imageFormat;
    createInfo.extent = { width, height, 1 };
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = 1;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.usage = (VkImageUsageFlags)usage;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createInfo.queueFamilyIndexCount = 1;
    uint32_t indices[] = { (uint32_t)sVkQueueIndex };
    createInfo.pQueueFamilyIndices = indices;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;


    VK_CHECK_CALL(vmaCreateImage(sVkAllocator,
        &createInfo, &allocInfo, &outImage.image, &outImage.allocation, nullptr));

    ASSERT(outImage.image && outImage.allocation);

    if (!outImage.image || !outImage.allocation)
        return false;

    outImage.view = createImageView(outImage.image, (VkFormat)imageFormat);
    ASSERT(outImage.view);
    if (!outImage.view)
        return false;

    sSetObjectName((uint64_t)outImage.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, imageName);
    outImage.imageName = imageName;
    outImage.width = width;
    outImage.height = height;
    outImage.format = createInfo.format;
    outImage.layout = createInfo.initialLayout;
    return true;
}
void destroyImage(Image& image)
{
    if (image.view)
        vkDestroyImageView(sVkDevice, image.view, nullptr);
    if (image.image)
        vmaDestroyImage(sVkAllocator, image.image, image.allocation);
    image = Image{};
}




VkImageMemoryBarrier imageBarrier(Image& image,
    uint32_t dstAccessMask, VkImageLayout newLayout)
{
    return imageBarrier(image,
        image.accessMask, (VkImageLayout)image.layout,
        dstAccessMask, (VkImageLayout)newLayout);
}

VkImageMemoryBarrier imageBarrier(Image& image,
    uint32_t srcAccessMask, VkImageLayout oldLayout,
    uint32_t dstAccessMask, VkImageLayout newLayout)
{
    VkImageAspectFlags aspectMask = sGetAspectMaskFromFormat((VkFormat)image.format);
    VkImageMemoryBarrier barrier =
        imageBarrier(image.image, srcAccessMask, oldLayout, dstAccessMask, newLayout, aspectMask);
    image.accessMask = dstAccessMask;
    image.layout = newLayout;
    return barrier;
}

VkImageMemoryBarrier imageBarrier(VkImage image,
    uint32_t srcAccessMask, VkImageLayout oldLayout,
    uint32_t dstAccessMask, VkImageLayout newLayout,
    uint32_t aspectMask)
{
    if (srcAccessMask == dstAccessMask && oldLayout == newLayout)
        return VkImageMemoryBarrier{};

    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.aspectMask = aspectMask;
    // Andoird error?
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return barrier;
}


bool createShader(const char* code, int codeSize, VkShaderModule& outModule)
{
    VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    createInfo.codeSize = codeSize;
    createInfo.pCode = (uint32_t*)code;
    VK_CHECK_CALL(vkCreateShaderModule(sVkDevice, &createInfo, nullptr, &outModule));
    ASSERT(outModule);
    if (!outModule)
        return false;

    return true;
}

VkDescriptorSetLayout createSetLayout(const DescriptorSetLayout* descriptors, int32_t count)
{
    ASSERT(count > 0);
    ASSERT(count < 16);


    VkDescriptorSetLayoutBinding setBindings[16] = {};

    for (int32_t i = 0; i < count; ++i)
    {
        const DescriptorSetLayout& layout = descriptors[i];

        setBindings[i] = VkDescriptorSetLayoutBinding{
            .binding = uint32_t(layout.bindingIndex),
            .descriptorType = VkDescriptorType(layout.descriptorType),
            .descriptorCount = 1,
            .stageFlags = layout.stage, // VK_SHADER_STAGE_VERTEX_BIT;
        };
    }

    VkDescriptorSetLayoutCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = u32(count),
        .pBindings = setBindings,
    };

    VkDescriptorSetLayout setLayout = 0;
    VK_CHECK_CALL(vkCreateDescriptorSetLayout(sVkDevice, &createInfo, nullptr, &setLayout));

    ASSERT(setLayout);
    return setLayout;
}


void destroyShaderModule(VkShaderModule_T** shaderModules, int32_t shaderModuleCount)
{
    for (int32_t i = 0; i < shaderModuleCount; ++i)
    {
        vkDestroyShaderModule(sVkDevice, shaderModules[i], nullptr);
        shaderModules[i] = {};
    }
}


void destroyPipeline(VkPipeline_T** pipelines, int32_t pipelineCount)
{
    for (int32_t i = 0; i < pipelineCount; ++i)
    {
        vkDestroyPipeline(sVkDevice, pipelines[i], nullptr);
        pipelines[i] = {};
    }
}

void destroyPipelineLayouts(VkPipelineLayout_T** pipelineLayouts, int32_t pipelineLayoutCount)
{
    for (int32_t i = 0; i < pipelineLayoutCount; ++i)
    {
        vkDestroyPipelineLayout(sVkDevice, pipelineLayouts[i], nullptr);
        pipelineLayouts[i] = {};
    }
}

void destroyDescriptorPools(VkDescriptorPool_T** pools, int32_t poolCount)
{
    for (int32_t i = 0; i < poolCount; ++i)
    {
        vkDestroyDescriptorPool(sVkDevice, pools[i], nullptr);
        pools[i] = {};
    }
}

void setVkSurface(VkSurfaceKHR_T* surface)
{
    sVkSurface = surface;
}


VkInstance_T* getVkInstance()
{
    return sVkInstance;
}

VkDevice_T* getVkDevice()
{
    return sVkDevice;
}


VkCommandBuffer_T* getVkCommandBuffer()
{
    int64_t frameIndex = sVkFrameIndex % CarpVk::FramesInFlight;
    VkCommandBuffer commandBuffer = sVkCommandBuffers[frameIndex];
    return commandBuffer;
}

const CarpSwapChainFormats& getSwapChainFormats()
{
    return sVkSwapchainFormats;
}



VkPipeline_T* createGraphicsPipeline(const GPBuilder& builder)
{
    VkDevice device = getVkDevice();

    VkPipelineVertexInputStateCreateInfo vertexInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

    VkPipelineInputAssemblyStateCreateInfo assemblyInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    assemblyInfo.topology = builder.topology;
    assemblyInfo.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportInfo.scissorCount = 1;
    viewportInfo.viewportCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterInfo.lineWidth = 1.0f;
    if (builder.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
    {
        rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // VK_FRONT_FACE_CLOCKWISE;
        rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    }
    else
    {
        rasterInfo.cullMode = VK_CULL_MODE_NONE;
        // notice VkPhysicalDeviceFeatures .fillModeNonSolid = VK_TRUE required
        //rasterInfo.polygonMode = VK_POLYGON_MODE_LINE;
    }
    VkPipelineMultisampleStateCreateInfo multiSampleInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multiSampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    depthInfo.depthTestEnable = builder.depthTest ? VK_TRUE : VK_FALSE;
    depthInfo.depthWriteEnable = builder.writeDepth ? VK_TRUE : VK_FALSE;
    depthInfo.depthCompareOp = builder.depthCompareOp;

    VkPipelineColorBlendStateCreateInfo blendInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = builder.blendChannelCount,
        .pAttachments = builder.blendChannels,
    };

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamicInfo.pDynamicStates = dynamicStates;
    dynamicInfo.dynamicStateCount = ARRAYSIZES(dynamicStates);


    VkGraphicsPipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    createInfo.stageCount = builder.stageInfoCount;
    createInfo.pStages = builder.stageInfos;
    createInfo.pVertexInputState = &vertexInfo;
    createInfo.pInputAssemblyState = &assemblyInfo;
    createInfo.pViewportState = &viewportInfo;
    createInfo.pRasterizationState = &rasterInfo;
    createInfo.pMultisampleState = &multiSampleInfo;
    createInfo.pDepthStencilState = &depthInfo;
    createInfo.pColorBlendState = &blendInfo;
    createInfo.pDynamicState = &dynamicInfo;
    createInfo.renderPass = VK_NULL_HANDLE;
    createInfo.layout = builder.pipelineLayout;
    createInfo.basePipelineHandle = VK_NULL_HANDLE;


    //Needed for dynamic rendering

    const VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .colorAttachmentCount = builder.colorFormatCount,
        .pColorAttachmentFormats = builder.colorFormats,
        .depthAttachmentFormat = builder.depthFormat,
    };

    //if(outPipeline.renderPass == VK_NULL_HANDLE)
    createInfo.pNext = &pipelineRenderingCreateInfo;


    VkPipeline pipeline = 0;
    VK_CHECK_CALL(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline));
    ASSERT(pipeline);

    /*
    if (!VulkanShader::createDescriptor(outPipeline))
    {
        printf("Failed to create graphics pipeline descriptor\n");
        return false;
    }
     */
     //setObjectName((uint64_t)pipeline, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, pipelineName);

    return pipeline;
}


bool beginFrame()
{
    VkDevice device = getVkDevice();

    sVkFrameIndex++;
    int64_t frameIndex = sVkFrameIndex % CarpVk::FramesInFlight;
    {
        //ScopedTimer aq("Acquire");
        VK_CHECK_CALL(vkWaitForFences(device, 1, &sVkFences[frameIndex], VK_TRUE, UINT64_MAX));
    }
    if (sVkAcquireSemaphores[frameIndex] == VK_NULL_HANDLE)
    {
        return false;
    }
    VkResult res = (vkAcquireNextImageKHR(device, sVkSwapchain, UINT64_MAX,
        sVkAcquireSemaphores[frameIndex], VK_NULL_HANDLE, &sVkImageIndex));

    //vulk->scratchBufferOffset = vulk->frameIndex * 32u * 1024u * 1024u;
    //vulk->scratchBufferLastFlush = vulk->scratchBufferOffset;
    //vulk->scratchBufferMaxOffset = (vulk->frameIndex + 1) * 32u * 1024u * 1024u;

    //if (res == VK_ERROR_OUT_OF_DATE_KHR)
    //{
    //    if (resizeSwapchain())
    //    {
    //        VK_CHECK(vkDeviceWaitIdle(vulk->device));
    //    }
    //    return false;
    //}
    //else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
    //{
    //    VK_CHECK(res);
    //    return false;
    //}

    return true;

}


bool presentImage(Image& imageToPresent)
{
    VkDevice device = getVkDevice();

    int64_t frameIndex = sVkFrameIndex % CarpVk::FramesInFlight;
    VkCommandBuffer commandBuffer = getVkCommandBuffer();

    VkImage swapchainImage = sVkSwapchainImages[sVkImageIndex];

    /*
    VkImageMemoryBarrier copyBeginBarriers[] =
        {
            imageBarrier(state.image,
                VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),

            imageBarrier(swapchainImage,
                0, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_ASPECT_COLOR_BIT)
        };

//    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZES(copyBeginBarriers), copyBeginBarriers);
*/
    {
        VkImageMemoryBarrier2 imageMemoryBarriers[2] = {
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR,
                .dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT_KHR,
                .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .image = swapchainImage,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .levelCount = 1,
                    .layerCount = 1,
                }
            },

            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR,
                .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, //  VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT, //  VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .image = imageToPresent.image,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .levelCount = 1,
                    .layerCount = 1,
                }
            }
        };

        VkDependencyInfoKHR dep2 = {};
        dep2.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
        dep2.imageMemoryBarrierCount = 2;
        dep2.pImageMemoryBarriers = imageMemoryBarriers;

        vkCmdPipelineBarrier2(commandBuffer, &dep2);

    }




    int width = imageToPresent.width;
    int height = imageToPresent.height;

    VkImageBlit2 imageBlitRegion = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
        .srcSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .layerCount = 1,
        },
        .srcOffsets = {
            VkOffset3D{ 0, 0, 0 },
            VkOffset3D{ width, height, 1 },
        },

        .dstSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .layerCount = 1,
        },
        .dstOffsets = {
            VkOffset3D{ 0, 0, 0 },
            VkOffset3D{ width, height, 1 },
        }


    };

    VkBlitImageInfo2 imageBlitInfo = {
        .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
        .srcImage = imageToPresent.image,
        .srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .dstImage = swapchainImage,
        .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .regionCount = 1,
        .pRegions = &imageBlitRegion,
        .filter = VkFilter::VK_FILTER_NEAREST,
    };


    vkCmdBlitImage2(commandBuffer, &imageBlitInfo);
    //state.image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    //swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlitRegion, VkFilter::VK_FILTER_NEAREST);


// Prepare image for presenting.
/*
    VkImageMemoryBarrier presentBarrier =
        imageBarrier(swapchainImage,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_IMAGE_ASPECT_COLOR_BIT);

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &presentBarrier);
*/
    /*
    {
        // Need to figure out what is the proper barriers for transfering an image from
        // copy target to present
        VkImageMemoryBarrier2 presentBarrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR,
            .srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT_KHR,
            .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR,
            .dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
            .dstAccessMask = VK_ACCESS_2_NONE,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,

            .image = swapchainImage,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };
        VkDependencyInfoKHR dep2 = {};
        dep2.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
        dep2.imageMemoryBarrierCount = 1;
        dep2.pImageMemoryBarriers = &presentBarrier;
        vkCmdPipelineBarrier2(commandBuffer, &dep2);
    }
    */
    VK_CHECK_CALL(vkEndCommandBuffer(commandBuffer));





    // Submit
    {
        //VkPipelineStageFlags submitStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; //VK_PIPELINE_STAGE_TRANSFER_BIT;
        //VkPipelineStageFlags submitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; //VK_PIPELINE_STAGE_TRANSFER_BIT;
        //VkPipelineStageFlags submitStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

        vkResetFences(device, 1, &sVkFences[frameIndex]);

        VkSemaphore acquireSemaphore = sVkAcquireSemaphores[frameIndex];
        VkSemaphore releaseSemaphore = sVkReleaseSemaphores[frameIndex];

        /*

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &sVkAcquireSemaphores[frameIndex];
        submitInfo.pWaitDstStageMask = &submitStageMask;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &releaseSemaphore;
        VK_CHECK_CALL(vkQueueSubmit(sVkQueue, 1, &submitInfo, sVkFences[frameIndex]));




        VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &sVkReleaseSemaphores[frameIndex];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &sVkSwapchain;
        presentInfo.pImageIndices = &sVkImageIndex;

*/

        VkCommandBufferSubmitInfo commandBufferSubmitInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = commandBuffer,
        };


        VkSemaphoreSubmitInfo acquireCompleteInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = acquireSemaphore,
            .stageMask = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL };

        VkSemaphoreSubmitInfo renderingCompleteInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = releaseSemaphore,
            .stageMask = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };

        VkSubmitInfo2 submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .waitSemaphoreInfoCount = 1,
            .pWaitSemaphoreInfos = &acquireCompleteInfo,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &commandBufferSubmitInfo,
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos = &renderingCompleteInfo
        };



        vkQueueSubmit2(sVkQueue, 1, &submitInfo, sVkFences[frameIndex]);

        VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &releaseSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &sVkSwapchain;
        presentInfo.pImageIndices = &sVkImageIndex;

        VkResult res = (vkQueuePresentKHR(sVkQueue, &presentInfo));
        //if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || vulk->needToResize)
        //{
        //    resizeSwapchain();
        //    vulk->needToResize = false;
        //    VK_CHECK(vkDeviceWaitIdle(vulk->device));
        //}
        //else
        {
            VK_CHECK_CALL(res);
        }
    }

    VK_CHECK_CALL(vkDeviceWaitIdle(device));
    return true;
}
