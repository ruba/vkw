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
#include <unordered_map>
#include <list>

namespace vkw
{
    class ExecutionManager
    {
    public:
        ExecutionManager(VkDevice device, std::uint32_t queue_family_index)
        : device_(device)
        , queue_family_index_(queue_family_index)
        {
        }
        
        void Submit(VkCommandBuffer buffer)
        {
            VkSubmitInfo submit_info;
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.pNext = nullptr;
            submit_info.commandBufferCount = 1u;
            submit_info.pCommandBuffers = &buffer;
            submit_info.pSignalSemaphores = nullptr;
            submit_info.signalSemaphoreCount = 0u;
            submit_info.pWaitSemaphores = nullptr;
            submit_info.waitSemaphoreCount = 0u;
            submit_info.pWaitDstStageMask = nullptr;
            
            VkQueue queue = nullptr;
            vkGetDeviceQueue(device_, queue_family_index_, 0u, &queue);
            
            vkQueueSubmit(queue, 1u, &submit_info, VK_NULL_HANDLE);
        }
        
        void WaitIdle()
        {
            VkQueue queue = nullptr;
            vkGetDeviceQueue(device_, queue_family_index_, 0u, &queue);
            vkQueueWaitIdle(queue);
        }
        
    private:
        VkDevice device_;
        std::uint32_t queue_family_index_;
    };
}
