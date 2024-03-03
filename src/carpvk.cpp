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




static void sDestroySwapchain(CarpVk& carpVk)
{
    for(int i = 0; i < carpVk.swapchainCount; ++i)
    {
        vkDestroyImageView(carpVk.device, carpVk.swapchainImagesViews[i], nullptr);
    }

    carpVk.swapchainCount = 0u;
    carpVk.swapchainWidth = carpVk.swapchainHeight = 0u;

    if(carpVk.swapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(carpVk.device, carpVk.swapchain, nullptr);
    carpVk.swapchain = VK_NULL_HANDLE;
}





static VkSemaphore sCreateSemaphore(CarpVk& carpVk)
{
    VkSemaphore semaphore = {};
    VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    VK_CHECK_CALL(vkCreateSemaphore(carpVk.device, &semaphoreInfo, nullptr, &semaphore));
    return semaphore;
}

static VkCommandPool sCreateCommandPool(CarpVk& carpVk)
{
    u32 familyIndex = carpVk.queueIndex;
    VkCommandPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    //poolCreateInfo.flags = 0; //VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolCreateInfo.queueFamilyIndex = familyIndex;

    VkCommandPool commandPool = {};
    VK_CHECK_CALL(vkCreateCommandPool(carpVk.device, &poolCreateInfo, nullptr, &commandPool));

    return commandPool;
}



static VkQueryPool sCreateQueryPool(CarpVk& carpVk, u32 queryCount)
{
    VkQueryPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };

    createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    createInfo.queryCount = queryCount;

    VkQueryPool pool = {};
    VK_CHECK_CALL(vkCreateQueryPool(carpVk.device, &createInfo, nullptr, &pool));

    ASSERT(pool);
    return pool;
}


static VkFence sCreateFence(CarpVk& carpVk)
{
    VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkFence result = {};
    VK_CHECK_CALL(vkCreateFence(carpVk.device, &createInfo, nullptr, &result));
    ASSERT(result);
    return result;
}






static void sSetObjectName(uint64_t object, VkDebugReportObjectTypeEXT objectType, const char *name)
{
    /*
    // Check for a valid function pointer
    if (pfnDebugMarkerSetObjectName)
    {
        VkDebugMarkerObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = objectType;
        nameInfo.object = object;
        nameInfo.pObjectName = name;
        pfnDebugMarkerSetObjectName(carpVk.device, &nameInfo);
    }
     */
}




void deinitCarpVk(CarpVk& carpVk)
{
    if (carpVk.instance == nullptr)
    {
        return;
    }
    if (carpVk.device)
    {
        VK_CHECK_CALL(vkDeviceWaitIdle(carpVk.device));

        if(carpVk.destroyBuffers)
        {
            carpVk.destroyBuffers(carpVk);
        }

        vkDestroyCommandPool(carpVk.device, carpVk.commandPool, nullptr);
        sDestroySwapchain(carpVk);



        for(u32 i = 0; i < CarpVk::FramesInFlight; ++i)
        {
            vkDestroyQueryPool(carpVk.device, carpVk.queryPools[i], nullptr);
        }


        for(u32 i = 0; i < CarpVk::FramesInFlight; ++i)
        {
            vkDestroyFence(carpVk.device, carpVk.fences[i], nullptr);
            vkDestroySemaphore(carpVk.device, carpVk.acquireSemaphores[i], nullptr);
            vkDestroySemaphore(carpVk.device, carpVk.releaseSemaphores[i], nullptr);
        }

        if (carpVk.allocator)
        {
            vmaDestroyAllocator(carpVk.allocator);
            carpVk.allocator = nullptr;
        }


        vkDestroyDevice(carpVk.device, nullptr);
        carpVk.device = nullptr;
    }


    if (carpVk.surface)
    {
        vkDestroySurfaceKHR(carpVk.instance, carpVk.surface, nullptr);
        carpVk.surface = nullptr;
    }
    if(carpVk.debugMessenger)
    {
        auto dest = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(carpVk.instance, "vkDestroyDebugUtilsMessengerEXT");
        dest(carpVk.instance, carpVk.debugMessenger, nullptr);
        carpVk.debugMessenger = nullptr;
    }

    vkDestroyInstance(carpVk.instance, nullptr);
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


bool instanceBuilderFinish(VulkanInstanceBuilder &builder, CarpVk& carpVk)
{
    VkInstance result = sCreateInstance(builder);
    if(result == nullptr)
    {
        return false;
    }
    carpVk.instance = result;
    VulkanInstanceBuilderImpl *casted =  sGetBuilderCasted(builder);
    if(casted->m_createInfo.enabledLayerCount > 0)
    {
        VkDebugUtilsMessengerEXT debugMessenger = sRegisterDebugCB(result);
        if (debugMessenger == nullptr)
        {
            return false;
        }
        carpVk.debugMessenger = debugMessenger;
    }

    return true;
}























bool createPhysicalDevice(CarpVk& carpVk, bool useIntegratedGpu)
{
    VkPhysicalDeviceType wantedDeviceType = useIntegratedGpu
        ? VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
        : VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

    VkPhysicalDevice devices[256] = {};
    u32 count = 0;

    VK_CHECK_CALL(vkEnumeratePhysicalDevices(carpVk.instance, &count, nullptr));
    ASSERT(count < 256);
    VK_CHECK_CALL(vkEnumeratePhysicalDevices(carpVk.instance, &count, devices));

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
        int queueIndex = sFindQueueFamilies(physicalDevice, carpVk.surface);
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
        SwapChainSupportDetails swapChainSupport = sQuerySwapChainSupport(physicalDevice, carpVk.surface);
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
        carpVk.physicalDevice = primary;
        carpVk.queueIndex = primaryQueueIndex;
    }
    else
    {
        carpVk.physicalDevice = secondary;
        carpVk.queueIndex = secondaryQueueIndex;
    }

    VkPhysicalDeviceProperties prop;
    vkGetPhysicalDeviceProperties(carpVk.physicalDevice, &prop);

    const char *typeText = prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "discrete" : "integrated";
    printf("Picking %s device: %s\n", typeText, prop.deviceName);
    return true;
}


bool createDeviceWithQueues(CarpVk& carpVk, VulkanInstanceBuilder& builder)
{
    SwapChainSupportDetails swapChainSupport = sQuerySwapChainSupport(carpVk.physicalDevice, carpVk.surface);
    bool swapChainAdequate = swapChainSupport.formatCount > 0 && swapChainSupport.presentModeCount > 0;
    if(!swapChainAdequate)
        return false;

    carpVk.swapchainFormats.presentColorFormat = VK_FORMAT_UNDEFINED;
    carpVk.swapchainFormats.depthFormat = sDefaultPresent[0].depth;
    carpVk.swapchainFormats.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    carpVk.swapchainFormats.defaultColorFormat = VK_FORMAT_UNDEFINED;

    for(u32 i = 0; i < swapChainSupport.formatCount
    && carpVk.swapchainFormats.presentColorFormat == VK_FORMAT_UNDEFINED; ++i)
    {
        for (const auto& format : sDefaultPresent)
        {
            if(swapChainSupport.formats[i].colorSpace != format.colorSpace)
                continue;
            if(swapChainSupport.formats[i].format != format.color)
                continue;
            carpVk.swapchainFormats.presentColorFormat = format.color;
            carpVk.swapchainFormats.depthFormat = format.depth;
            carpVk.swapchainFormats.colorSpace = format.colorSpace;
        }
    }

    if(carpVk.swapchainFormats.presentColorFormat == VK_FORMAT_UNDEFINED
       && swapChainSupport.formatCount == 1
       && swapChainSupport.formats[0].format == VK_FORMAT_UNDEFINED)
    {
        carpVk.swapchainFormats.presentColorFormat = sDefaultPresent[0].color;
        carpVk.swapchainFormats.colorSpace = sDefaultPresent[0].colorSpace;
        carpVk.swapchainFormats.depthFormat = sDefaultPresent[0].depth;
    }
    ASSERT(carpVk.swapchainFormats.presentColorFormat != VK_FORMAT_UNDEFINED);
    if(carpVk.swapchainFormats.presentColorFormat == VK_FORMAT_UNDEFINED)
    {
        return false;
    }
    for (const auto &format : sDefaultFormats)
    {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(carpVk.physicalDevice, format.color, &formatProperties);
        if (((formatProperties.optimalTilingFeatures) & sFormatFlagBits) == sFormatFlagBits)
        {
            carpVk.swapchainFormats.defaultColorFormat = format.color;
            break;
        }
    }

    ASSERT(carpVk.swapchainFormats.defaultColorFormat != VK_FORMAT_UNDEFINED);
    if(carpVk.swapchainFormats.defaultColorFormat == VK_FORMAT_UNDEFINED)
    {
        return false;
    }

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = carpVk.queueIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    static constexpr VkPhysicalDeviceFeatures deviceFeatures = {
        //.fillModeNonSolid = VK_TRUE,
        .samplerAnisotropy = VK_FALSE,
    };
    static constexpr VkPhysicalDeviceShaderObjectFeaturesEXT shaderObjectFeature = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT,
        .shaderObject = VK_TRUE,
    };

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

    VK_CHECK_CALL(vkCreateDevice(carpVk.physicalDevice, &createInfo,
        nullptr, &carpVk.device));

    ASSERT(carpVk.device);

    if (!carpVk.device)
        return false;

    vkGetDeviceQueue(carpVk.device, 0, carpVk.queueIndex, &carpVk.queue);
    ASSERT(carpVk.queue);


    if (!carpVk.device || !carpVk.queue)
        return false;
    // Init VMA
    {
        VmaVulkanFunctions vulkanFunctions = {};
        vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.vulkanApiVersion = sVulkanApiVersion;
        allocatorCreateInfo.physicalDevice = carpVk.physicalDevice;
        allocatorCreateInfo.device = carpVk.device;
        allocatorCreateInfo.instance = carpVk.instance;
        allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

        VK_CHECK_CALL(vmaCreateAllocator(&allocatorCreateInfo, &carpVk.allocator));
        if (!carpVk.allocator)
            return false;
    }

    //if(optionals.canUseVulkanRenderdocExtensionMarker)
    //    sAcquireDeviceDebugRenderdocFunctions(vulk->device);
    return true;
}



bool createSwapchain(CarpVk& carpVk, VSyncType vsyncMode, int width, int height)
{
    VkPresentModeKHR findPresentMode = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
    switch (vsyncMode)
    {
        case VSyncType::FIFO_VSYNC: findPresentMode = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR; break;
        case VSyncType::IMMEDIATE_NO_VSYNC: findPresentMode = VkPresentModeKHR::VK_PRESENT_MODE_IMMEDIATE_KHR; break;
        case VSyncType::MAILBOX_VSYNC: findPresentMode = VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR; break;
    }
    {
        SwapChainSupportDetails swapChainSupport = sQuerySwapChainSupport(carpVk.physicalDevice, carpVk.surface);
        ASSERT(swapChainSupport.formatCount > 0);
        VkSurfaceFormatKHR surfaceFormat = swapChainSupport.formats[0];
        bool found = false;
        for (const auto& availableFormat : swapChainSupport.formats)
        {
            if (availableFormat.format == carpVk.swapchainFormats.presentColorFormat
                && availableFormat.colorSpace == carpVk.swapchainFormats.colorSpace)
            {
                surfaceFormat = availableFormat;
                found = true;
                break;
            }
        }
        if(!found && swapChainSupport.formatCount == 1
            && swapChainSupport.formats[0].format == VK_FORMAT_UNDEFINED)
        {
            surfaceFormat.colorSpace = (VkColorSpaceKHR)carpVk.swapchainFormats.colorSpace;
            surfaceFormat.format = (VkFormat)carpVk.swapchainFormats.presentColorFormat;
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
        createInfo.surface = carpVk.surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT
            //| VK_IMAGE_USAGE_SAMPLED_BIT
            //| VK_IMAGE_USAGE_STORAGE_BIT
            | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.oldSwapchain = carpVk.swapchain;

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
        VkResult res = vkCreateSwapchainKHR(carpVk.device, &createInfo, nullptr, &swapchain);
        VK_CHECK_CALL(res);
        if (res != VK_SUCCESS)
        {
            printf("Failed to initialize swapchain\n");
            return false;
        }
        carpVk.swapchain = swapchain;

        carpVk.swapchainFormats.colorSpace = surfaceFormat.colorSpace;
        carpVk.swapchainFormats.presentColorFormat = surfaceFormat.format;

        carpVk.swapchainWidth = extent.width;
        carpVk.swapchainHeight = extent.height;

    }





    u32 swapchainCount = 0;
    VK_CHECK_CALL(vkGetSwapchainImagesKHR(carpVk.device, carpVk.swapchain, &swapchainCount, nullptr));

    VK_CHECK_CALL(vkGetSwapchainImagesKHR(carpVk.device, carpVk.swapchain, &swapchainCount, carpVk.swapchainImages));
    carpVk.swapchainCount = swapchainCount;

    for(int i = 0; i < swapchainCount; ++i)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = carpVk.swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = (VkFormat)carpVk.swapchainFormats.presentColorFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VK_CHECK_CALL(vkCreateImageView(carpVk.device, &createInfo, nullptr, &carpVk.swapchainImagesViews[i]));
    }

    return true;
}

bool finalizeInit(CarpVk& carpVk)
{
    for(u32 i = 0; i < CarpVk::FramesInFlight; ++i)
    {
        carpVk.queryPools[i] = sCreateQueryPool(carpVk, CarpVk::QueryCount);
        ASSERT(carpVk.queryPools[i]);
        if(!carpVk.queryPools[i])
        {
            printf("Failed to create vulkan query pool!\n");
            return false;
        }
    }


    for(u32 i = 0; i < CarpVk::FramesInFlight; ++i)
    {
        carpVk.acquireSemaphores[i] = sCreateSemaphore(carpVk);
        ASSERT(carpVk.acquireSemaphores[i]);
        if(!carpVk.acquireSemaphores[i])
        {
            printf("Failed to create vulkan acquire semapohore!\n");
            return false;
        }

        carpVk.releaseSemaphores[i] = sCreateSemaphore(carpVk);
        ASSERT(carpVk.releaseSemaphores[i]);
        if(!carpVk.releaseSemaphores[i])
        {
            printf("Failed to create vulkan release semaphore!\n");
            return false;
        }

        carpVk.fences[i] = sCreateFence(carpVk);
        ASSERT(carpVk.fences[i]);
        if(!carpVk.fences[i])
        {
            printf("Failed to create vulkan fence!\n");
            return false;
        }
    }
    carpVk.commandPool = sCreateCommandPool(carpVk);
    ASSERT(carpVk.commandPool);
    if(!carpVk.commandPool)
    {
        printf("Failed to create vulkan command pool!\n");
        return false;
    }


    VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocateInfo.commandPool = carpVk.commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = CarpVk::FramesInFlight;

    {
        VK_CHECK_CALL(vkAllocateCommandBuffers(carpVk.device, &allocateInfo, carpVk.commandBuffers));
        for(u32 i = 0; i < CarpVk::FramesInFlight; ++i)
        {
            if(!carpVk.commandBuffers[i])
            {
                printf("Failed to create vulkan command buffer!\n");
                return false;
            }

            static const char* s = "Main command buffer";
            sSetObjectName((uint64_t)carpVk.commandBuffers[i], VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, s);
        }
    }


    return true;
}











VkImageView_T* createImageView(CarpVk& carpVk, VkImage_T* image, int64_t format)
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
    VK_CHECK_CALL(vkCreateImageView(carpVk.device, &createInfo, nullptr, &view));

    ASSERT(view);
    return view;
}


bool createImage(CarpVk& carpVk,
    uint32_t width, uint32_t height,
    int64_t imageFormat, int64_t usage,
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
    uint32_t indices[] = { (uint32_t)carpVk.queueIndex };
    createInfo.pQueueFamilyIndices = indices;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    outImage.format = imageFormat;
    outImage.layout = VK_IMAGE_LAYOUT_UNDEFINED;

    VK_CHECK_CALL(vmaCreateImage(carpVk.allocator,
        &createInfo, &allocInfo, &outImage.image, &outImage.allocation, nullptr));

    ASSERT(outImage.image && outImage.allocation);

    if (!outImage.image || !outImage.allocation)
        return false;

    outImage.view = createImageView(carpVk, outImage.image, (VkFormat)imageFormat);
    ASSERT(outImage.view);
    if (!outImage.view)
        return false;

    //MyVulkan::setObjectName((u64)outImage.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, imageName);
    //outImage.imageName = imageName;
    //outImage.width = width;
    //outImage.height = height;
    //outImage.format = format;
    //outImage.layout = createInfo.initialLayout;
    return true;
}
void destroyImage(CarpVk& carpVk, Image& image)
{
    if (image.view)
        vkDestroyImageView(carpVk.device, image.view, nullptr);
    if (image.image)
        vmaDestroyImage(carpVk.allocator, image.image, image.allocation);
    image = Image{};
}




VkImageMemoryBarrier imageBarrier(Image& image,
    VkAccessFlags dstAccessMask, VkImageLayout newLayout)
{
    return imageBarrier(image,
        (VkAccessFlags)image.accessMask, (VkImageLayout)image.layout,
        (VkAccessFlags)dstAccessMask, (VkImageLayout)newLayout);
}

VkImageMemoryBarrier imageBarrier(Image& image,
    VkAccessFlags srcAccessMask, VkImageLayout oldLayout,
    VkAccessFlags dstAccessMask, VkImageLayout newLayout)
{
    VkImageAspectFlags aspectMask = sGetAspectMaskFromFormat((VkFormat)image.format);
    VkImageMemoryBarrier barrier =
        imageBarrier(image.image, srcAccessMask, oldLayout, dstAccessMask, newLayout, aspectMask);
    image.accessMask = dstAccessMask;
    image.layout = newLayout;
    return barrier;
}

VkImageMemoryBarrier imageBarrier(VkImage image,
    VkAccessFlags srcAccessMask, VkImageLayout oldLayout,
    VkAccessFlags dstAccessMask, VkImageLayout newLayout,
    int64_t aspectMask)
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


bool createShader(CarpVk& carpVk,
    const char* code, int codeSize, VkShaderModule& outModule)
{
    VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    createInfo.codeSize = codeSize;
    createInfo.pCode = (uint32_t*)code;
    VK_CHECK_CALL(vkCreateShaderModule(carpVk.device, &createInfo, nullptr, &outModule));
    ASSERT(outModule);
    if (!outModule)
        return false;

    return true;
}
