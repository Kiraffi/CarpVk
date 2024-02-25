#include "carpvk.h"
#include <stdio.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "common.h"

uint32_t sVulkanApiVersion = VK_API_VERSION_1_3;

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
    // VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
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










void deinitCarpVk(CarpVk& carpVk)
{
    if (carpVk.instance == nullptr)
    {
        return;
    }
    if (carpVk.device)
    {
        VK_CHECK_CALL(vkDeviceWaitIdle(carpVk.device));
        sDestroySwapchain(carpVk);


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
#if 0
    VulkanDeviceOptionals optionals = sGetDeviceOptionals(carpVk.physicalDevice);
    // createdeviceinfo
    {
        static constexpr VkPhysicalDeviceFeatures deviceFeatures = {
            //.fillModeNonSolid = VK_TRUE,
            .samplerAnisotropy = VK_FALSE,
        };
        static constexpr  VkPhysicalDeviceVulkan13Features deviceFeatures13 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .dynamicRendering = VK_TRUE,
        };

        static constexpr  VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = VulkanApiVersion >= VK_API_VERSION_1_3 ? (void*)&deviceFeatures13 : nullptr,
            .features = deviceFeatures,
        };

#endif
    VkDeviceCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    //createInfo.pNext = &physicalDeviceFeatures2;

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
        createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;// | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
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

    return true;
}
