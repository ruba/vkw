#include "vk_render_target.h"

#include "vk_utils.h"
#include <string>

namespace vkw
{
    RenderTarget::RenderTarget(VkMemoryManager& memory_manager,
                               std::vector<RenderTargetCreateInfo> const& params)
    {
        attachments_.resize(params.size());
        std::vector<VkAttachmentDescription> descriptions(params.size());
        std::vector<VkImageView> views(params.size());
        
        for (auto i = 0; i < params.size(); i++)
        {
            InitAttachment(memory_manager, params[i], attachments_[i]);
            descriptions[i] = attachments_[i].description;
            views[i] = attachments_[i].view;
        }
        
        render_pass_ = memory_manager.CreateDefaultRenderPass(descriptions);
        framebuffer_ = memory_manager.CreateFrameBuffer(views, render_pass_, params[0].width, params[0].height, 1);
    }
    
    void RenderTarget::InitAttachment(VkMemoryManager& memory_manager,
                                      RenderTargetCreateInfo const& params,
                                      RenderTargetAttachment& attachment)
    {
        VkExtent3D size = {params.width, params.height, 1};
        
        attachment.image = memory_manager.CreateImage(size, params.format, params.usage);
        attachment.view  = memory_manager.CreateImageView(attachment.image, params.format, params.usage);
        
        attachment.format = params.format;
        attachment.width = params.width;
        attachment.height = params.height;
        
        attachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.description.storeOp = (params.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.description.format = attachment.format;
        attachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.description.finalLayout = (ContainsDepth(params.format) || ContainsStencil(params.format)) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }    
}

