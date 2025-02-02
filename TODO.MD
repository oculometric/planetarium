# TODOs

## Vulkan Stuff

- [x] make physical device stuff its own file (finding swapchain info, supported features, etc)
- [ ] fix instantly crashing on frame 2 or whatever
- [ ] specify required device features (non-solid fill for wireframe, etc)
- [ ] make swapchain stuff (swapchain, swapchain format, extent, images, imageviews, framebuffers) its own file, and handle creation/destruction, maybe recreation (as opposed to just destroy whole thing and recreate)
- [ ] make renderpass and subpass stuff its own file, handle creation/destruction
- [ ] implement proper shader config and reflection, and checking this when creating render passes
- [ ] improve shader module stuff
- [ ] make pipeline sutff its own file, properly handle creation and destruction
- [ ] make mesh/vertex stuff its own file, handle vertex buffer creation/destruction
- [ ] replace oculib
- [ ] make sure everything which gets `destroy`ed in application (framebuffers, etc) is treated as a **pointer** when converting to PTSomething (i.e. so `delete` order can be controlled by application)

## Engine Stuff

- [ ] integrate STUI for terminal stuff
- [ ] develop a debugging utility class
- [ ] scene tree system
- [ ] UI system (use STUI to generate charbufs, then pass it into a shader maybe????)
- [ ] object and vertex picking
- [ ] scene editing
- [ ] mesh editing
- [ ] proper input handling from both keyboard and mouse
- [ ] live shader editor

## Art Stuff

- [ ] planet generation based on parameters
- [ ] planet art shaders
- [ ] stylised post-processing
- [ ] spaceship terminal interface/look