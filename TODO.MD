# TODOs

## Vulkan Stuff

- [ ] resource manager system for things like PTPipeline, PTShader, etc, which tracks dependencies and counts references
- [ ] replace oculib
- [ ] implement proper shader config and reflection, and checking this when creating render passes
- [ ] make sure everything which gets `destroy`ed in application (framebuffers, etc) is treated as a **pointer** when converting to PTSomething (i.e. so `delete` order can be controlled by application)
- [ ] fix semaphore issue on windows

## Engine Stuff

- [ ] fix camera aspect ratio stuff
- [ ] object transform
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
- [ ] google earth replacement

## Misc

- [ ] make Makefile respect header files [https://www.reddit.com/r/C_Programming/comments/xwhmup/is_there_a_good_way_to_automatically_handle/]

## Completed
- [x] make physical device stuff its own file (finding swapchain info, supported features, etc)
- [x] make swapchain stuff (swapchain, swapchain format, extent, images, imageviews, framebuffers) its own file, and handle creation/destruction, maybe recreation (as opposed to just destroy whole thing and recreate)
- [x] implement swapchain/framebuffer resizing
- [x] fix instantly crashing on frame 2 or whatever
- [x] make input file respect new debug system
- [x] integrate STUI for terminal stuff
- [x] make buffer creation/handling its own file (buffer class which gets subclassed?)
- [x] make a property list STUI component (i.e. so we don't need a list of horizontal boxes with two labels in each)
- [x] develop a debugging utility class
- [x] add explicit window resize callback
- [x] make pipeline sutff its own file, properly handle creation and destruction
- [x] make renderpass and subpass stuff its own file, handle creation/destruction
- [x] improve shader module stuff
- [x] handle image management in its own file
- [x] fix swapchain resize being called constantly after resize
- [x] specify required device features (non-solid fill for wireframe, etc)
- [x] make mesh/vertex stuff its own file, handle vertex buffer creation/destruction
- [x] fix vulkan warnings on exit
- [x] implement debug messenger in application to capture vulkan messages
- [x] fix invalid free pointer on exit via window close button