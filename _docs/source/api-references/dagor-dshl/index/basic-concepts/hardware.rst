.. _hardware:

================
Hardware defines
================

DSHL supports some built-in hardware defines.
You can use them in shaders to toggle hardware specific features, change shader behavior depending on the target platform, etc.

These defines are accessed via ``hardware.<platform>`` syntax.
Here is a list of the available defines:

- ``metal`` -- Metal API
- ``metaliOS`` -- Metal API for iOS
- ``vulkan`` -- Vulkan API
- ``pc`` -- deprecated, alias for DirectX 11 API
- ``dx11`` -- DirectX 11 API
- ``dx12`` -- DirectX 12 API
- ``xbox`` -- Xbox One platform
- ``scarlett`` -- Scarlett platform (a.k.a Xbox Series X/S)
- ``ps4`` -- PlayStation 4 platform
- ``ps5`` -- PlayStation 5 platform
- ``fsh_4_0`` -- HLSL Shader Model 4.0 version
- ``fsh_5_0`` -- HLSL Shader Model 5.0 version
- ``fsh_6_0`` -- HLSL Shader Model 6.0 version
- ``fsh_6_6`` -- HLSL Shader Model 6.6 version
- ``mesh`` -- if platform supports mesh shaders
- ``bindless`` -- if platform supports bindless textures

Usage example:

.. code-block:: c

  shader example_shader
  {
    if (hardware.metal) {
      dont_render;
      // this shader will not be compiled for metal platform
    }

    hlsl {
      int var = 0;
  ##if hardware.dx12
      var = 12;
  ##elif hardware.dx11
      var = 11;
  ##endif
      // value of var will be different for dx11 and dx12 platforms
    }
  }
