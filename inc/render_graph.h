#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "resource.h"
#include "resource_manager.h"

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

class PTRenderPass;
class PTImage;
class PTMaterial;
class PTSwapchain;

// TODO: right now multi camera support is impossible. we would need extra uniform buffers (and descriptor sets, ugh) to support it
// TODO: support non-swapchain-shaped texture rendering
// TODO: support custom clear values for each image/buffer
// TODO: simple copy step

struct PTRGStep
{
    friend class PTRGGraph;
private:
    // assigned by the graph class, do not touch
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptor_sets;

public:
    int colour_buffer_binding = 0;  // image index to send colour output to
    int depth_buffer_binding = -1;  // image index to send depth output to
    int normal_buffer_binding = -1; // image index to send normal output to
    int extra_buffer_binding = -1;  // image index to send extra output to

    bool is_camera_step = true;     // true if this should be a camera render step, false for a post-process step
    size_t camera_slot = 0;         // camera index to use for rendering, if camera-ing (currently does nothing)

    // material to use for rendering, if post-processing
    PTMaterial* process_material = nullptr;
    // mappings between image buffer indices to material texture slots
    std::vector<std::pair<int, uint16_t>> process_inputs;
};

struct PTRGStepInfo
{
    PTRenderPass* render_pass = nullptr;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkExtent2D extent;
    std::array<VkClearValue, 4> clear_values;
};

class PTRGGraph : public PTResource
{
    friend class PTResourceManager;
private:
    VkDevice device = VK_NULL_HANDLE;
    PTSwapchain* swapchain = nullptr;
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;

    std::vector<PTRGStep> timeline_steps;
    PTBuffer* shared_transform_uniforms;
    std::array<PTBuffer*, MAX_FRAMES_IN_FLIGHT> shared_scene_uniforms;
    std::vector<std::pair<PTImage*, VkImageView>> image_buffers;
    int final_image_index = -1;
    std::vector<VkFramebuffer> framebuffers;
    PTRenderPass* render_pass;

    PTImage* spare_colour_image = nullptr;
    VkImageView spare_colour_image_view = VK_NULL_HANDLE;
    PTImage* spare_depth_image = nullptr;
    VkImageView spare_depth_image_view = VK_NULL_HANDLE;
    PTImage* spare_normal_image = nullptr;
    VkImageView spare_normal_image_view = VK_NULL_HANDLE;
    PTImage* spare_extra_image = nullptr;
    VkImageView spare_extra_image_view = VK_NULL_HANDLE;

    PTRGGraph(VkDevice _device, PTSwapchain* _swapchain, std::string timeline_path);
    PTRGGraph(VkDevice _device, PTSwapchain* _swapchain);
    ~PTRGGraph();

    PTRGGraph(const PTRGGraph& other) = delete;
    PTRGGraph(const PTRGGraph&& other) = delete;
    PTRGGraph operator=(const PTRGGraph& other) = delete;
    PTRGGraph operator=(const PTRGGraph&& other) = delete;

    void generateRenderPasses();
    VkImageView prepareImage(PTImage*& target, VkFormat format);
    void createImageBufferForBinding(int& binding, PTImage*& spare_image, VkImageView& spare_image_view, VkFormat format);
    void generateImagesAndFramebuffers();
    void createMaterialDescriptorSets();
    void discardAllResources();
    void destroyImages();

public:
    inline PTRenderPass* getRenderPass() const { return render_pass; }
    inline PTImage* getFinalImage() const { return final_image_index < 0 ? spare_colour_image : image_buffers[final_image_index].first; }
    inline size_t getStepCount() const { return timeline_steps.size(); }
    inline bool getStepIsCamera(size_t step_index) const { return timeline_steps[step_index].is_camera_step; }
    //inline size_t getStepCameraSlot(size_t step_index) const { return timeline_steps[step_index].camera_slot; }
    inline std::pair<PTMaterial*, VkDescriptorSet> getStepMaterial(size_t step_index, uint32_t frame_index) const
    {
        return std::pair<PTMaterial*, VkDescriptorSet>(timeline_steps[step_index].process_material, timeline_steps[step_index].descriptor_sets[frame_index]);
    }
    PTRGStepInfo getStepInfo(size_t step_index) const;

    void resize();
    void updateUniforms(const SceneUniforms& scene_uniforms, const TransformUniforms& transform_uniforms, uint32_t frame_index);

    void configure(const std::vector<PTRGStep>& steps, int final_image);
};