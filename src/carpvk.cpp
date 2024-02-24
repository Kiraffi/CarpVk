#include "carpvk.h"

#include <stdio.h>
#include <string.h>
//#include <vulkan/vulkan_core.h>

#include <volk.h>



struct VulkanInstanceBuilderImpl
{
    VkApplicationInfo m_appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
    } ;

    VkInstanceCreateInfo m_createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &m_appInfo,
    };

    VkDebugUtilsMessengerCreateInfoEXT m_debugCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    };

};
static_assert(sizeof(VulkanInstanceBuilder) >= sizeof(VulkanInstanceBuilderImpl));
static_assert(alignof(VulkanInstanceBuilder) == alignof(VulkanInstanceBuilderImpl));






#ifndef ARRAYSIZES
#define ARRAYSIZES(theArray) (theArray ? (sizeof(theArray) / sizeof(theArray[0])) : 0)
#endif

#if _MSC_VER
#define DEBUG_BREAK_MACRO() __debugbreak()
#else
#include <signal.h>
#define DEBUG_BREAK_MACRO() raise(SIGTRAP)
#endif

#define ASSERT_STRING(STUFF, STUFFSTRING) \
do { if (STUFF) {} else printf("Assertion: %s\n", STUFFSTRING); } while (0)

#define ASSERT(STUFF) ASSERT_STRING(STUFF, #STUFF)


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
    //VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
    //VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
    VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
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

    const char *type = errorMsg ? "Error" : (warningMsg  ? "Warning" : "Info");

    printf("Type:%s, message: %s\n\n", type, pCallbackData->pMessage);
    if(errorMsg)
    {
        ASSERT(!"Validation error encountered!");
    }
    return VK_FALSE;
}


VulkanInstanceBuilder instaceBuilder()
{
    VulkanInstanceBuilderImpl result;
    VulkanInstanceBuilder builder = {};
    memcpy(&builder, &result, sizeof(result));
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
    casted->m_appInfo.apiVersion = VK_MAKE_VERSION(major, minor, patch);
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


    VkValidationFeaturesEXT validationFeatures = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
    validationFeatures.enabledValidationFeatureCount = ARRAYSIZES(sEnabledValidationFeatures);
    validationFeatures.pEnabledValidationFeatures = sEnabledValidationFeatures;

    casted->m_debugCreateInfo.pNext = &validationFeatures;

    casted->m_createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &casted->m_debugCreateInfo;


    return builder;

}

VulkanInstanceBuilder& instanceBuilderSetDefaultMessageSeverity(VulkanInstanceBuilder &builder)
{
    VulkanInstanceBuilderImpl *casted =  sGetBuilderCasted(builder);

}

VkInstance_T* instanceBuilderFinish(VulkanInstanceBuilder &builder)
{
    VkResult r;
    uint32_t version;
    void* ptr = nullptr;

    /* This won't compile if the appropriate Vulkan platform define isn't set. */
#if 0
    ptr =
#if defined(_WIN32)
        &vkCreateWin32SurfaceKHR;
#elif defined(__linux__) || defined(__unix__)
        &vkCreateXlibSurfaceKHR;
#elif defined(__APPLE__)
    &vkCreateMacOSSurfaceMVK;
#else
    /* Platform not recogized for testing. */
    NULL;
#endif
#endif
    /* Try to initialize volk. This might not work on CI builds, but the
     * above should have compiled at least. */
    r = volkInitialize();
    if (r != VK_SUCCESS) {
        printf("volkInitialize failed!\n");
        return nullptr;
    }

    version = volkGetInstanceVersion();
    printf("Vulkan version %d.%d.%d initialized.\n",
        VK_VERSION_MAJOR(version),
        VK_VERSION_MINOR(version),
        VK_VERSION_PATCH(version));




    VkInstance result = {};
    VulkanInstanceBuilderImpl *casted =  sGetBuilderCasted(builder);
    VK_CHECK_CALL(vkCreateInstance(&casted->m_createInfo, nullptr, &result));

    return result;
}


/*
 *     VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    createInfo.pApplicationInfo = &appInfo;

    PodVector<const char*> extensions = sGetRequiredInstanceExtensions();

    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledExtensionCount = (u32)extensions.size();

    for(auto ext : extensions)
    {
        printf("instance ext: %s\n", ext);
    }

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
#if NDEBUG
#else
    auto &initParams = VulkanInitializationParameters::get();

    if (initParams.useValidationLayers)
    {
        createInfo.ppEnabledLayerNames = sValidationLayers;
        createInfo.enabledLayerCount = ARRAYSIZES(sValidationLayers);

        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = sDebugReportCB;


        VkValidationFeaturesEXT validationFeatures = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
        validationFeatures.enabledValidationFeatureCount = ARRAYSIZES(sEnabledValidationFeatures);
        validationFeatures.pEnabledValidationFeatures = sEnabledValidationFeatures;

        debugCreateInfo.pNext = &validationFeatures;

        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    }
    else
#endif
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    VK_CHECK_CALL(vkCreateInstance(&createInfo, nullptr, &vulk->instance));

    return vulk->instance != VK_NULL_HANDLE;
}

 */