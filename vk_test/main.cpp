#include <vulkan/vulkan.h>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <functional>

#include "spirv_msl.hpp"
#include "vk_scoped_object.h"
#include "vk_memory_allocator.h"
#include "vk_memory_manager.h"
#include "vk_shader_manager.h"
#include "vk_descriptor_manager.h"
#include "vk_pipeline_manager.h"

using namespace vkw;

VkScopedObject<VkInstance> create_instance()
{
    // Initialize usual Vulkan crap
    VkApplicationInfo app_info;
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pApplicationName = "VK test";
    app_info.applicationVersion = 1;
    app_info.pEngineName = "Vk test";
    app_info.engineVersion = 1;
    app_info.apiVersion = VK_API_VERSION_1_0;
    
    VkInstanceCreateInfo instance_info;
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pNext = NULL;
    instance_info.flags = 0;
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledExtensionCount = 0;
    instance_info.ppEnabledExtensionNames = NULL;
    instance_info.enabledLayerCount = 0;
    instance_info.ppEnabledLayerNames = NULL;
    
    VkInstance instance = nullptr;
    VkResult res = vkCreateInstance(&instance_info, nullptr, &instance);
    if (res == VK_ERROR_INCOMPATIBLE_DRIVER)
    {
        throw std::runtime_error("Cannot find a compatible Vulkan ICD\n");
    }
    else if (res)
    {
        throw std::runtime_error("Unknown error\n");
    }
    
    return VkScopedObject<VkInstance>(instance,
                                      [](VkInstance instance)
                                      {
                                          vkDestroyInstance(instance, nullptr);
                                      });
}

VkScopedObject<VkDevice> create_device(VkInstance instance,
                                       std::uint32_t& queue_family_index,
                                       VkPhysicalDevice* opt_physical_device = nullptr)
{
    // Enumerate devices
    auto gpu_count = 0u;
    auto res = vkEnumeratePhysicalDevices(instance, &gpu_count, nullptr);
    
    if (gpu_count == 0)
    {
        throw std::runtime_error("No compatible devices found\n");
    }
    
    std::vector<VkPhysicalDevice> gpus(gpu_count);
    res = vkEnumeratePhysicalDevices(instance, &gpu_count, gpus.data());
    
    float queue_priority = 0.f;
    VkDeviceQueueCreateInfo queue_create_info;
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.pNext = nullptr;
    queue_create_info.flags = 0;
    queue_create_info.queueCount = 1u;
    queue_create_info.pQueuePriorities = &queue_priority;
    
    auto queue_family_count = 0u;
    vkGetPhysicalDeviceQueueFamilyProperties(gpus[0], &queue_family_count, nullptr);
    
    std::vector<VkQueueFamilyProperties> queue_props(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(gpus[0], &queue_family_count, queue_props.data());
    
    // Look for a queue supporing both compute and transfer
    bool found = false;
    for (unsigned int i = 0; i < queue_family_count; i++)
    {
        if (queue_props[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT))
        {
            queue_create_info.queueFamilyIndex = i;
            found = true;
            break;
        }
    }
    
    if (!found)
    {
        throw std::runtime_error("No compute/transfer queues found\n");
    }
    
    VkDeviceCreateInfo device_create_info;
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = nullptr;
    device_create_info.flags = 0;
    device_create_info.queueCreateInfoCount = 1u;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    device_create_info.enabledLayerCount = 0u;
    device_create_info.ppEnabledLayerNames = nullptr;
    device_create_info.enabledExtensionCount = 0u;
    device_create_info.ppEnabledExtensionNames = nullptr;
    device_create_info.pEnabledFeatures = nullptr;
    
    VkDevice device = nullptr;
    res = vkCreateDevice(gpus[0], &device_create_info, nullptr, &device);
    
    if (res != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan device\n");
    }
    
    if (opt_physical_device)
    {
        *opt_physical_device = gpus[0];
    }
    
    return VkScopedObject<VkDevice>(device,
                                    [](VkDevice device)
                                    {
                                        vkDestroyDevice(device, nullptr);
                                    });
}

VkScopedObject<VkCommandPool> create_command_pool(VkDevice device, std::uint32_t queue_family_index)
{
    VkCommandPoolCreateInfo pool_create_info;
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.pNext = nullptr;
    pool_create_info.flags = 0;
    pool_create_info.queueFamilyIndex = queue_family_index;
    
    VkCommandPool command_pool = nullptr;
    auto res = vkCreateCommandPool(device, &pool_create_info, nullptr, &command_pool);
    
    if (res != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create command pool\n");
    }
    
    return VkScopedObject<VkCommandPool>(command_pool,
                                         [device](VkCommandPool pool)
                                         {
                                             vkDestroyCommandPool(device, pool, nullptr);
                                         });
}


int main(int argc, const char * argv[])
{
    auto instance = create_instance();
    
    VkPhysicalDevice physical_device;
    std::uint32_t queue_family_index = 0u;
    auto device = create_device(instance, queue_family_index, &physical_device);
    
    VkMemoryAllocator allocator(device, physical_device);
    VkMemoryManager memory_manager(device, queue_family_index, allocator);
    VkDescriptorManager descriptor_manager(device);
    VkShaderManager shader_manager(device, descriptor_manager);
    PipelineManager pipeline_manager(device);
    
    auto command_pool = create_command_pool(device, queue_family_index);
    
    VkScopedArray<VkDescriptorSetLayout> layouts;
    std::vector<VkPushConstantRange> push_constant_ranges;
    auto shader = shader_manager.CreateComputeShader("add.comp.spv");
    
    auto pipeline = pipeline_manager.CreateComputePipeline(shader);
    
    std::vector<int> fake_data{1, 2, 3, 4, 5};
    auto buffer_a = memory_manager.CreateBuffer(fake_data.size() * sizeof(int),
                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                fake_data.data());
    
    auto buffer_b = memory_manager.CreateBuffer(fake_data.size() * sizeof(int),
                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                fake_data.data());
    
    auto buffer_c = memory_manager.CreateBuffer(fake_data.size() * sizeof(int),
                                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                              fake_data.data());
    
    shader.SetArg(0u, buffer_a);
    shader.SetArg(1u, buffer_b);
    shader.SetArg(2u, buffer_c);
    shader.CommitArgs();

    auto buffer = memory_manager.CreateBuffer(fake_data.size() * sizeof(int),
                                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                              VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                              fake_data.data());
    
    std::vector<int> data(5);
    memory_manager.ReadBuffer(buffer, 0u, fake_data.size() * sizeof(int), data.data());
    
    std::vector<int> new_data{5, 4, 3, 2, 1};
    memory_manager.WriteBuffer(buffer, 0u, new_data.size() * sizeof(int), new_data.data());
    
    
    memory_manager.ReadBuffer(buffer, 0u, new_data.size() * sizeof(int), data.data());
    
    
    VkCommandBufferAllocateInfo command_buffer_alloc_info;
    command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.pNext = nullptr;
    command_buffer_alloc_info.commandPool = command_pool;
    command_buffer_alloc_info.commandBufferCount = 1u;
    command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    
    VkCommandBuffer command_buffer = nullptr;
    vkAllocateCommandBuffers(device, &command_buffer_alloc_info, &command_buffer);
    
    VkCommandBufferBeginInfo begin_info;
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = nullptr;
    begin_info.pInheritanceInfo = nullptr;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    
    vkBeginCommandBuffer(command_buffer, &begin_info);
    
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
    vkCmdBindDescriptorSets(command_buffer,
                            VK_PIPELINE_BIND_POINT_COMPUTE,
                            pipeline.layout,
                            0,
                            (std::uint32_t)shader.descriptor_sets.size(),
                            shader.descriptor_sets.data(), 0u,
                            nullptr);
    
    std::uint32_t size = (std::uint32_t)data.size();
    vkCmdPushConstants(command_buffer,
                       pipeline.layout,
                       VK_SHADER_STAGE_COMPUTE_BIT,
                       0,
                       sizeof(std::uint32_t),
                       &size);
    
    vkCmdDispatch(command_buffer, 1u, 1u, 1u);
    
    VkBufferMemoryBarrier barrier;
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.buffer = buffer_c;
    barrier.offset = 0u;
    barrier.size = VK_WHOLE_SIZE;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    
    vkCmdPipelineBarrier(command_buffer,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_HOST_BIT,
                         0u,
                         0u,
                         nullptr,
                         1u,
                         &barrier,
                         0u,
                         nullptr);
    
    vkEndCommandBuffer(command_buffer);
    
    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.commandBufferCount = 1u;
    submit_info.pCommandBuffers = &command_buffer;
    submit_info.pSignalSemaphores = nullptr;
    submit_info.signalSemaphoreCount = 0u;
    submit_info.pWaitSemaphores = nullptr;
    submit_info.waitSemaphoreCount = 0u;
    submit_info.pWaitDstStageMask = nullptr;
    
    VkQueue queue = nullptr;
    vkGetDeviceQueue(device, queue_family_index, 0u, &queue);
    
    vkQueueSubmit(queue, 1u, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    
    std::vector<int> result(5);
    memory_manager.ReadBuffer(buffer_c, 0u, 5 * sizeof(int), result.data());
    
    std::cout << result[0];
    
    // cmdbuffer_manager.BeginCommandBuffer();
    // cmdbuffer_manager.BindPipeline(pipeline);
    // shader->SetArg(0u, buffer_a);
    // shader->SetArg(1u, buffer_b);
    // shader->SetArg(2u, buffer_c);
    // cmdbuffer_manager.Dispatch(shader, grid_res);
    
    return 0;
}
