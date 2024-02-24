
struct VulkanInstanceBuilder;
struct alignas(8) VulkanInstanceBuilder
{
    char size[1024];
};

struct VkInstance_T;

VulkanInstanceBuilder instaceBuilder();

VulkanInstanceBuilder& instanceBuilderSetApplicationVersion(
    VulkanInstanceBuilder &builder, int major, int minor, int patch);

VulkanInstanceBuilder& instanceBuilderSetExtensions(
    VulkanInstanceBuilder &builder, const char** extensions, int extensionCount);

VulkanInstanceBuilder& instanceBuilderUseDefaultValidationLayers(VulkanInstanceBuilder &builder);
VulkanInstanceBuilder& instanceBuilderSetDefaultMessageSeverity(VulkanInstanceBuilder &builder);


VkInstance_T* instanceBuilderFinish(VulkanInstanceBuilder &builder);