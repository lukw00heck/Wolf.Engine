#include "w_render_pch.h"
#include "w_graphics_device_manager.h"
#include "w_semaphore.h"

using namespace wolf::graphics;

VkSemaphore* w_semaphore::get()
{
    return &this->_semaphore;
}

W_RESULT w_semaphore::initialize(_In_ const std::shared_ptr<w_graphics_device>& pGDevice)
{
    this->_gDevice = pGDevice;

#ifdef __VULKAN__
    //create semaphore create info
    VkSemaphoreCreateInfo _semaphore_create_info = {};
    _semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    if (vkCreateSemaphore(pGDevice->vk_device,
                          &_semaphore_create_info,
                          nullptr,
                          &this->_semaphore))
    {
        V(W_FAILED, "creating semaphore", "w_semaphore", 3, false);
        return W_FAILED;
    }
#else
    
#endif
    return W_PASSED;
}

ULONG w_semaphore::release()
{
#ifdef __VULKAN__
	if (this->_semaphore)
	{
		vkDestroySemaphore(this->_gDevice->vk_device, this->_semaphore, nullptr);
		this->_semaphore = 0;
	}
#else
    
#endif

    this->_gDevice = nullptr;

    return 0;
}