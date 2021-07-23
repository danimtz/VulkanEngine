#pragma once



#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <iostream>
#include <vector>
#include <map>
#include <deque>
#include <set>
#include <cmath>
#include <algorithm>
#include "Engine/Logger.h"
#include "Engine/Renderer/RenderBackend.h"

#include <vk_mem_alloc.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>



constexpr int FRAME_OVERLAP_COUNT = 2;


struct GPUinfo_t {
    VkPhysicalDevice                        device;
    VkPhysicalDeviceProperties              properties;
    VkPhysicalDeviceMemoryProperties        memory_properties;
    VkPhysicalDeviceFeatures                features;
    VkSurfaceCapabilitiesKHR                surface_capabilities;
    std::vector<VkSurfaceFormatKHR>         surface_formats;
    std::vector<VkPresentModeKHR>           present_modes;
    std::vector<VkQueueFamilyProperties>    queue_family_properties;
    std::vector<VkExtensionProperties>      extension_properties;
};

struct DeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void pushFunction(std::function<void()> function) {
        deletors.push_back(function);
    }

    void executeDeletionQueue() {
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
            (*it)(); //call function
        }
        deletors.clear();
    }
};

//Move this to a separate class eventually  -> image or vulkanimage. 
//Its only used for depth stencil image
struct VmaImage {
    VkImage m_image;
    VmaAllocation m_allocation;
};


struct VulkanFrameData
{
    VkCommandPool           m_command_pool;
    VkCommandBuffer         m_command_buffer;
    VkSemaphore             m_present_semaphore, m_render_semaphore;
    VkFence                 m_render_fence;
};

class VulkanRenderBackend : public RenderBackend {
public:

    virtual void init(GLFWwindow* window) override;
    virtual void shutdown() override;

    //Void pointers to work with abstraction. Needs static cast when called
    virtual void* getDevice() const override { return m_device; }; 
    virtual void* getSwapchainExtent() override { return &m_swapchain_extent; };
    virtual void* getRenderPass() const override { return m_render_pass; };
    virtual void* getAllocator() const override { return m_allocator; }; //CONSIDER MOVING ALLOCATOR TO SEPARATE CLASS

    virtual uint32_t getFrameNumber() const override { return m_frame_count; };

    virtual void pushToDeletionQueue(std::function<void()> function) override;

//render commands/functions that use vulkan commands
    virtual void RC_beginFrame() override;
    virtual void RC_endFrame() override;

    //NOTE TO FUTURE ME should renderbackend have these funcitons? or should GraphicsPipeline and VertexBUffer etc have a  Bind() function that does them
    virtual void RC_bindGraphicsPipeline(const std::shared_ptr<GraphicsPipeline>) override; 
    virtual void RC_pushConstants(const std::shared_ptr<GraphicsPipeline> pipeline, const MatrixPushConstant push_constant) override;
    virtual void RC_bindVertexBuffer(const std::shared_ptr<VertexBuffer> vertex_buffer) override;
    virtual void RC_bindIndexBuffer(const std::shared_ptr<IndexBuffer> index_buffer) override;

    virtual void RC_drawIndexed(uint32_t size) override;
    virtual void RC_drawSumbit(uint32_t size) override;

private://main vulkan setup
    void initContext_VK();
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void choosePhysicalDevice();
    void createDeviceAndQueues();
    void createVmaAllocator();
    void createSwapchainAndImages();
    void createCommandPoolAndBuffers();
    void createDefaultRenderPass();
    void createFramebuffers();
    void createSemaphoresAndFences();

    
    VulkanFrameData& getCurrentFrame() { return m_frame_data[m_frame_count % FRAME_OVERLAP_COUNT]; };


  
private:
//context
    GLFWwindow*                     m_glfw_window; //could be abstracted to use different library other than glfw, hard coded for now
    VkInstance                      m_instance;
    VkDebugUtilsMessengerEXT        m_debug_messenger;
    GPUinfo_t                       m_gpu_info;
    VkDevice                        m_device;
    uint32_t                        m_present_family_idx;
    uint32_t                        m_graphics_family_idx;
    VkQueue                         m_graphics_queue;
    VkQueue                         m_present_queue;
    VkSurfaceKHR                    m_surface;

    VmaAllocator                    m_allocator;

//renderpass
    VkRenderPass                    m_render_pass;
    std::vector<VkFramebuffer>      m_framebuffers;

//swapchain
    VkSwapchainKHR                  m_swapchain;
    VkFormat                        m_swapchain_format;
    VkExtent2D                      m_swapchain_extent;
    VkPresentModeKHR                m_swapchain_present_mode;
    std::vector<VkImage>            m_swapchain_images;
    std::vector<VkImageView>        m_swapchain_views;
    uint32_t                        m_swapchain_img_idx;

//depth buffer
    VmaImage                        m_depth_image;
    VkImageView                     m_depth_image_view;
    VkFormat                        m_depth_format;

//Per frame data(command buffers, pool and sync structures)
    VulkanFrameData                 m_frame_data[FRAME_OVERLAP_COUNT];

   
    uint32_t                    m_frame_count{0};


    std::vector<const char*>    m_device_extensions;
    std::vector<const char*>    m_validation_layers;

    bool                        m_isInitialized{false};

    DeletionQueue               m_deletion_queue;
};
