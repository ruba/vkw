/**********************************************************************
 Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ********************************************************************/
#pragma once
#include <vulkan/vulkan.h>
#include "vk_scoped_object.h"
#include <vector>


namespace vkw
{
    class DescriptorManager
    {
    public:
        static auto constexpr kMaxSets = 512u;
        static auto constexpr kNumDescriptors = 256u;
        
        DescriptorManager(VkDevice device)
        : device_(device)
        {
            VkDescriptorPoolSize pool_sizes[] =
            {
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, kNumDescriptors },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, kNumDescriptors },
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, kNumDescriptors },
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kNumDescriptors },
                { VK_DESCRIPTOR_TYPE_SAMPLER, kNumDescriptors },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, kNumDescriptors }
            };
            
            VkDescriptorPoolCreateInfo pool_create_info;
            pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_create_info.pNext = nullptr;
            pool_create_info.flags = 0;
            pool_create_info.maxSets = kMaxSets;
            pool_create_info.poolSizeCount = sizeof(pool_sizes) / sizeof(VkDescriptorPoolSize);
            pool_create_info.pPoolSizes = pool_sizes;
            
            VkDescriptorPool pool = nullptr;
            auto res = vkCreateDescriptorPool(device, &pool_create_info, nullptr, &pool);
            
            if (res != VK_SUCCESS)
            {
                throw std::runtime_error("VkDescriptorManager: Cannot create descriptor pool");
            }
            
            pool_ = VkScopedObject<VkDescriptorPool>(pool,
                                                     [device](VkDescriptorPool pool)
                                                     {
                                                         vkDestroyDescriptorPool(device, pool, nullptr);
                                                     });
        }
        
        auto AllocateDescriptorSet(VkDescriptorSetLayout layout)
        {
            VkDescriptorSetAllocateInfo allocate_info;
            allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocate_info.pNext = nullptr;
            allocate_info.descriptorPool = pool_;
            allocate_info.descriptorSetCount = 1u;
            allocate_info.pSetLayouts = &layout;
            
            VkDescriptorSet descriptor_set = nullptr;
            auto res = vkAllocateDescriptorSets(device_, &allocate_info, &descriptor_set);
            
            if (res != VK_SUCCESS)
            {
                throw std::runtime_error("VkDescriptorManager: Cannot allocate descriptors");
            }
            
            return VkScopedObject<VkDescriptorSet>(descriptor_set,
                                                  [this](VkDescriptorSet desctiptor_set)
                                                  {
                                                      vkFreeDescriptorSets(device_, pool_, 1u, &desctiptor_set);
                                                  });
        }
        
        
    private:
        VkDevice device_;
        VkScopedObject<VkDescriptorPool> pool_;
    };
}
