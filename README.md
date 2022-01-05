# VulkanEngine
Vulkan rendering engine written with the objective of learning how the basics of the graphics API.  
Currently a work in progress, but the objective is to be able to set up forward rendering, deferred rendering and maybe forward+/clustered rendering.

## Current features:
### Engine:
- Spir-V reflection of glsl shader to create descriptor set layouts.
- Command buffer recording abstraction.
- Free camera movement using quaternions.
- GLTF2.0 model loading
- Uses GLFW for window mangement
- ImGUI integration (only demo for now)
- Entity component system (Currently in progress)


### Renderer:
- Forward renderer
- Double buffering
- Directional and Point lights
- Physically Based Rendering (Cook Torrance BRDF)
- Image-Based Lighting (calculates diffuse irradince maps, BRDF LUTs and pre-filtered environment map using compute shaders)
- Cubemapped skybox

## Example images:
**Image Based Lighting. Only ambient light is being used in this example**     
<img src="https://raw.githubusercontent.com/danimtz/VulkanEngine/master/assets/RenderImages/IBL.png" width="950" height=100%>     

**Simple PBR shading with directional light(sun) and two point lights**     
<img src="https://raw.githubusercontent.com/danimtz/VulkanEngine/master/assets/RenderImages/SkyBox_PBRtest2.PNG" alt="Skybox sample" width="950" height=100%>     

## Resources:
 Main resources being used are:
 - [Vulkan tutorial](https://vulkan-tutorial.com/)   
 - [VKguide by vblanco20-1](https://vkguide.dev/)

## External libraries used:
- [stbimage](https://github.com/nothings/stb) for texture/image loading.
- [dearImgui](https://github.com/ocornut/imgui) for GUI.
- [GLFW](https://www.glfw.org/) for hardware window abstraction.
- [glm](https://github.com/g-truc/glm) math library originally written for openGL.
- [vma](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) Vulkan Memory Allocator.
- [Tinygltf](https://github.com/syoyo/tinygltf) Header only file for loading gltf parser.
- [Spirv-Cross](https://github.com/KhronosGroup/SPIRV-Cross) Used for automatic shader reflection of descriptor layouts.



## Models and Textures:
- [Water Bottle](https://github.com/KhronosGroup/glTF-Sample-Models/tree/master/2.0/WaterBottle) from Khronos group sample GTLF2.0. models.
- [Damaged Helmet](https://github.com/KhronosGroup/glTF-Sample-Models/tree/master/2.0/DamagedHelmet) from Khronos group sample GTLF2.0. models.
- [Ryfjället skybox](https://www.humus.name/index.php?page=Textures&ID=37) from HUMUS textures.
- [Venice HDR sunset](https://polyhaven.com/a/venice_sunset) from polyhaven.
