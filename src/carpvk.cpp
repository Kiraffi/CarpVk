#include "carpvk.h"
#include <stdio.h>
#include <string.h>
#include <vector>

#include <volk.h>
//#include <vulkan/vulkan_core.h>

//#include <volk.h>

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
        ++i;
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




bool initVolk()
{
    VkResult r;
    uint32_t version;

    r = volkInitialize();
    if (r != VK_SUCCESS)
    {
        printf("volkInitialize failed!\n");
        return false;
    }

    version = volkGetInstanceVersion();
    printf("Vulkan version %d.%d.%d initialized.\n",
        VK_VERSION_MAJOR(version),
        VK_VERSION_MINOR(version),
        VK_VERSION_PATCH(version));

    return true;
}

void deinitCarpVk(CarpVk& carpVk)
{
    if (carpVk.instance == nullptr)
    {
        return;
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

    if (carpVk.device)
    {
        vkDestroyDevice(carpVk.device, nullptr);
        carpVk.device = nullptr;
    }
    vkDestroyInstance(carpVk.instance, nullptr);
}

void printExtensions()
{
    VkExtensionProperties allExtensions[256] = {};
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
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
    u32 count = ARRAYSIZES(devices);

    VK_CHECK_CALL(vkEnumeratePhysicalDevices(carpVk.instance, &count, nullptr));
    ASSERT(count > 256);
    VK_CHECK_CALL(vkEnumeratePhysicalDevices(carpVk.instance, &count, devices));

    VkPhysicalDevice primary = nullptr;
    VkPhysicalDevice secondary = nullptr;

    int primaryQueueIndex = -1;
    int secondaryQueueIndex = -1;

    int requiredExtensionCount = ARRAYSIZES(sDeviceExtensions);

    for(u32 i = 0; i < count; ++i)
    {
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
            ASSERT(extensionCount > 256);

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
