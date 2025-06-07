#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "constant.h"
#include "reference_counter.h"

// the render graph consists of a timeline of instructions - either cameras to draw, or post process steps to run
// each command will have inputs and outputs which read from and write to buffers
// those buffers can then be used as the inputs/outputs of other commands
// one buffer is the framebuffer, and can only be written to once
// cameras will output colour, depth, and extra buffers, and you can choose which you want to keep in buffers
// then any of those can be bound to inputs on post process steps, which can also have multiple outputs

/*
     buf a          buf b        framebuffer
       ^              ^               ^
|      |       |      |       |       |        |
| render cam 1 | render cam 2 | post process 1 |
|              |              |    ^       ^   |
                                   |       |
                                 buf a   buf b
*/

const VkImageUsageFlags IMAGE_USAGE = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
const VkFormat EXTRA_FORMAT = VK_FORMAT_R16G16B16A16_SNORM;
const VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;

typedef PTCountedPointer<class PTRenderPass_T> PTRenderPass;
typedef PTCountedPointer<class PTBuffer_T> PTBuffer;
typedef PTCountedPointer<class PTImage_T> PTImage;
typedef PTCountedPointer<class PTSwapchain_T> PTSwapchain;
typedef PTCountedPointer<class PTMaterial_T> PTMaterial;

// TODO: right now multi camera support is impossible. we would need extra uniform buffers (and descriptor sets, ugh) to support it
// TODO: simple copy step

/**
 * @brief describes a step in the render graph
 */
struct PTRGStep
{
    friend class PTRGGraph_T;
private:
    // descriptor sets to be used, assigned by the graph class, do not touch
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptor_sets;
    // framebuffer to be used, assigned by the graph class, do not touch
    VkFramebuffer framebuffer = VK_NULL_HANDLE;

public:
    int colour_buffer_binding = 0;  // image index to send colour output to
    // clear value for the colour buffer
    PTVector4f colour_clear_value = PTVector4f{ 1.0f, 0.0f, 1.0f, 1.0f };
    int depth_buffer_binding = -1;  // image index to send depth output to
    // clear value for the depth buffer
    float depth_clear_value = 1.0f;
    int normal_buffer_binding = -1; // image index to send normal output to
    // clear value for the normal buffer
    PTVector4f normal_clear_value = PTVector4f{ 0.0f, 0.0f, 0.0f, 1.0f };
    int extra_buffer_binding = -1;  // image index to send extra output to
    // clear value for the extra buffer
    PTVector4f extra_clear_value = PTVector4f{ 0.0f, 0.0f, 0.0f, 1.0f };

    // custom extent, leave set to zero to use swapchain extent
    VkExtent2D custom_extent = VkExtent2D{ 0, 0 };

    bool is_camera_step = true;     // true if this should be a camera render step, false for a post-process step
    size_t camera_slot = 0;         // camera index to use for rendering, if camera-ing (currently does nothing)

    // material to use for rendering, if post-processing
    PTMaterial process_material = nullptr;
    // mappings between image buffer indices to material texture slots
    std::vector<std::pair<int, uint16_t>> process_inputs;
};

/**
 * @brief contains information required to generate vulkan commands for a render graph step
 */
struct PTRGStepInfo
{
    PTRenderPass render_pass = nullptr;         // render pass to be used for rendering
    VkFramebuffer framebuffer = VK_NULL_HANDLE; // framebuffer to be used when rendering (combines assigned image buffers)
    VkExtent2D extent;                          // size of the target buffers (should be used for scissor and viewport)
    std::array<VkClearValue, 4> clear_values;   // clear values to use when starting render pass
};

/**
 * @brief class which manages a render graph and its associated resources
 * 
 * you MUST call `configure` before you start using the render graph
 */
class PTRGGraph_T
{
private:
    VkDevice device = VK_NULL_HANDLE;           // vulkan device reference
    PTSwapchain swapchain = nullptr;            // swapchain reference
    // descriptor pool used to allocate for post-process timeline steps
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;

    // the sequence of render steps to execute
    std::vector<PTRGStep> timeline_steps;
    // transform uniform buffer shared by all post-process timeline steps
    PTBuffer shared_transform_uniforms;
    // array of scene uniform buffers shared by all post-process timeline steps
    std::array<PTBuffer, MAX_FRAMES_IN_FLIGHT> shared_scene_uniforms;
    // array of image buffers which can be used as post process inputs or render targets
    std::vector<std::pair<PTImage, VkImageView>> image_buffers;
    // index in the image buffer array of the image to be shown to the screen (or -1 to use the spare colour buffer)
    int final_image_index = -1;
    // render pass used by all timeline steps
    PTRenderPass render_pass;

    // images and image views for use when attachments are not going to be used as inputs

    PTImage spare_colour_image = nullptr;
    VkImageView spare_colour_image_view = VK_NULL_HANDLE;
    PTImage spare_depth_image = nullptr;
    VkImageView spare_depth_image_view = VK_NULL_HANDLE;
    PTImage spare_normal_image = nullptr;
    VkImageView spare_normal_image_view = VK_NULL_HANDLE;
    PTImage spare_extra_image = nullptr;
    VkImageView spare_extra_image_view = VK_NULL_HANDLE;

    /**
     * @brief create the render pass and the uniform buffers
     */
    void generateRenderPassAndUniformBuffers();
    /**
     * @brief generate an image and imageview appropriate for a specified format
     * 
     * @param target image pointer to initialise
     * @param format format to use for the image. determines some other flags for image and view creation
     * @param extent extent to use for image creation
     * @returns generated image view, appropriate for the format given
     */
    VkImageView prepareImage(PTImage& target, VkFormat format, VkExtent2D extent);
    /**
     * @brief configure the image for a binding, either by creating a new image in the array or
     * by initialising the spare image
     * 
     * @param binding index into image array, or -1 to use the spare image
     * @param spare_image spare image pointer to initialise if binding is -1 and spare image is nullptr
     * @param spare_image_view image view to pair with the spare image
     * @param format format of the image to generate, passed into `prepareImage`
     * @param _extent size of the image to generate
     */
    void createImageBufferForBinding(int& binding, PTImage& spare_image, VkImageView& spare_image_view, VkFormat format, VkExtent2D _extent);
    /**
     * @brief construct the images needed for all the timeline steps and combine them into framebuffers
     */
    void generateImagesAndFramebuffers();
    /**
     * @brief apply textures to a post-process step's material
     * 
     * @param step target step to apply
     */
    void linkTexturesToMaterial(const PTRGStep& step);
    /**
     * @brief configure descriptor sets for each post-process timeline step and apply texture bindings
     */
    void createMaterialDescriptorSets();
    /**
     * @brief destroy all resources associated with the render graph
     */
    void discardAllResources();
    /**
     * @brief destroy image resources (images, views, framebuffers)
     */
    void destroyImages();

public:
    PTRGGraph_T(const PTRGGraph_T& other) = delete;
    PTRGGraph_T(const PTRGGraph_T&& other) = delete;
    PTRGGraph_T operator=(const PTRGGraph_T& other) = delete;
    PTRGGraph_T operator=(const PTRGGraph_T&& other) = delete;
    ~PTRGGraph_T();

    //static inline PTCountedPointer<PTRGGraph_T> createRGGraph(std::string timeline_path)
    //{ return PTCountedPointer<PTRGGraph_T>(new PTRGGraph_T(timeline_path)); }
    static inline PTCountedPointer<PTRGGraph_T> createRGGraph()
    { return PTCountedPointer<PTRGGraph_T>(new PTRGGraph_T()); }

    PTRenderPass getRenderPass() const;
    PTImage getFinalImage() const;
    inline size_t getStepCount() const { return timeline_steps.size(); }
    inline bool getStepIsCamera(size_t step_index) const { return timeline_steps[step_index].is_camera_step; }
    inline size_t getStepCameraSlot(size_t step_index) const { return timeline_steps[step_index].camera_slot; }
    std::pair<PTMaterial, VkDescriptorSet> getStepMaterial(size_t step_index, uint32_t frame_index) const;
    PTRGStepInfo getStepInfo(size_t step_index) const;

    void resize();
    void updateUniforms(const SceneUniforms& scene_uniforms, const TransformUniforms& transform_uniforms, uint32_t frame_index);

    void configure(const std::vector<PTRGStep>& steps, int final_image);

private:
    PTRGGraph_T(std::string timeline_path);
    PTRGGraph_T();
};

typedef PTCountedPointer<PTRGGraph_T> PTRGGraph;
