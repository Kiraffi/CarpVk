#include <vulkan/vulkan_core.h>

#include <stdio.h>

#include "carpvk.h"

/*

bool initVulkan()
{
    VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.apiVersion = VulkanApiVersion;
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);


    VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
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

    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &vulk->instance));

    return vulk->instance != VK_NULL_HANDLE;

}
*/





int main()
{

#if NDEBUG
    const char* extensions[] = {
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    };
#else
    const char* extensions[] = {
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
    };
#endif


    VulkanInstanceBuilder builder = instaceBuilder();
    instanceBuilderSetApplicationVersion(builder, 0, 0, 1);
    instanceBuilderSetExtensions(builder, extensions, sizeof(extensions) / sizeof(const char*));
    instanceBuilderUseDefaultValidationLayers(builder);
    if(!instanceBuilderFinish(builder))
    {
        printf("Failed to initialize vulkan.\n");
        return 1;
    }

    printf("Hello world!\n");
    return 0;
}
