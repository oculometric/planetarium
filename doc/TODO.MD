# TODOs

## Vulkan Stuff

- [ ] material system
      - [x] read material from file
      - [x] implement proper shader reflection
      - [ ] texture descriptors                           (3)
      - [x] respect priority system
- [ ] runtime shader compilation                          (2)
- [ ] mesh data updating
- [ ] support materials loading as a scene resource       (1)

## Engine Stuff

- [ ] make input run on its own thread                    (0)
- [ ] make float isDown and bool wasPressed and bool wasReleased functions for each keyboard key
- [ ] better way of specifying which keyboard key to check the state of (A vs a, etc)
- [ ] show debug axes of objects currently hovered over (this should be a new Node type)
- [ ] cross platform controller support
- [ ] mouse input support
- [ ] UI system (use STUI to generate charbufs, then pass it into a shader maybe????)
- [ ] fix bug with quaternions where something goes to infinity
- [ ] object and vertex picking
- [ ] scene editing
- [ ] mesh editing
- [ ] live shader editor
- [ ] COMPILED node editing
- [ ] 3d reconstruction using meshing/shared topology solving (i.e. you draw some geometry from one view, then you draw some from another view and say what's what, then the engine solves the relative orientations of the view and the vertices)

## Art Stuff

- [ ] graphite shader import
- [ ] planet generation based on parameters
- [ ] planet art shaders
- [ ] stylised post-processing
- [ ] spaceship terminal interface/look
- [ ] google earth replacement
- [ ] dernacueilliette walkaround scene

## Misc

- [ ] add a fuck ton of comments everywhere
	- [ ] application
	- [x] buffer
	- [x] camera node
	- [ ] debug
	- [ ] deserialiser
	- [x] fly camera node
	- [ ] gamepad
	- [ ] gizmo node
	- [ ] resource manager
	- [ ] transform
	- [ ] all header files
- [ ] array handling for deserialiser argument parsing
- [ ] actual error handling?

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
- [x] resource manager system for things like PTPipeline, PTShader, etc, which tracks dependencies and counts references
- [x] make sure everything which gets `destroy`ed in application (framebuffers, etc) is treated as a **pointer** when converting to PTSomething (i.e. so `delete` order can be controlled by application)
- [x] implement reference-recounter in resource manager
- [x] fix camera aspect ratio & matrix stuff
- [x] clean up todos
- [x] make Makefile respect header files [https://www.reddit.com/r/C_Programming/comments/xwhmup/is_there_a_good_way_to_automatically_handle/]
- [x] replace oculib
- [x] actually implement deserialisers for camera and mesh
- [x] scene tree system with builder/deserialisation system (0)
- [x] scene/transform parent-child system (0)
- [x] object drawing system
- [x] object transform (quaternions, reverse param update, transformation functions) (0)
- [x] make all input functions return a float instead for easy maths (from controller)
- [x] fix semaphore issue when resizing the framebuffer
- [x] screenshot button
- [x] 'q' shortcut to quit via stui
- [x] fix console log scroll to end
- [x] make custom node type creation easier and more fluid (template, # defines, way of checking type of instance. also, make the makefile search for node types and import them)
- [x] move functional code out of scene
- [x] add ability to read vertex colours from OBJ files
- [x] swapchain resizing
- [x] pipeline updating
- [x] synchronisation for renderer state!
- [x] common uniforms should carry an object id
- [x] improve mesh draw call queueing system to allow for unlimited objects and choice of shader/render pass/pipeline
      - [x] specify the transform pointer to track
      - [x] organise objects by material (i.e. the non-common uniform buffer is all the same)
      - [x] draw objects with indexed descriptor sets?
      - [x] descriptor set pool needs to be able to support dynamic numbers of different descriptor set layouts
      - [x] store allocated descriptor sets on the object draw-request (create when the draw-request is created, destroy when the draw-request is removed)
      - [x] material specifies the shader, pipeline, descriptor set layout, descriptor set values, textures
- [x] add uniform buffer for scene-constants (screen size, time, lights)
- [x] change the way PTInput initialises to match PTResourceManager
- [x] separate graphics stuff into RenderingServer (simplify Application)
