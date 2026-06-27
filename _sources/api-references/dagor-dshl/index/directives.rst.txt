.. _directives:

*****************
Common directives
*****************

------------
no_dynstcode
------------

``no_dynstcode`` disallows the shader to use any kind of dynamic stcode in its body.
This means that the shader can only access ``material`` variables, or variables defined in a shader block :ref:`shader-blocks`, which is supported by the shader.
Consider the following example:

.. code-block:: text

    float4 some_global_var;
    float4 another_global_var;

    block(scene) common_scene_block
    {
      (vs) { some_global_var@f4 = some_global_var; }
    }

    shader test_shader
    {
      no_dynstcode;

      texture tex = material.texture.diffuse;
      (ps) { diffuse_tex@staticSmp = tex; }
      // diffuse_tex is now accessible inside hlsl(ps){...} blocks

      supports common_scene_block;
      // some_global_var is now accessible inside hlsl(vs){...} blocks

      // will cause compilation error
      (vs) { another_global_var@f4 = another_global_var; }
    }

-----------
dont_render
-----------

``dont_render`` is used to disable shader variants.
Most common use-case is to disable redundant variants of a shader, which is using some shared ``interval``.

A special value of ``NULL`` will be assigned to the shader variant that has ``dont_render`` and it will not be compiled.
When ``NULL`` dynamic variant is selected during runtime, no rendering or dispatching (in case of compute shaders) happens.
If a ``ShaderElement`` is being created from a ``NULL`` static shader variant, it will just return ``NULL``.

.. code-block:: text

    include "render_pass_modes.dshl"

    shader decals_shader
    {
      if (render_pass_mode == render_pass_impostor)
      { dont_render; }

      // ...
    }

.. note::
  ``dont_render`` does not directly decrease the number of shader variants.
  Although compilation of ``NULL`` shaders does not happen and stcode for them is not created, they are still present in shader variant lookup table.

---------
no_ablend
---------

``no_ablend`` forcefully disables blending.
Specifying ``blend_src, blend_dst`` or ``blend_asrc, blend_adst`` will have no effect.

--------------------
supports directives
--------------------

In addition to supporting :ref:`shader blocks <shader-blocks>`, the ``supports`` keyword can reference
several built-in directives that reserve special resources for the bindless materials system and multidraw rendering.

supports __static_cbuf
======================

``supports __static_cbuf`` reserves a constant buffer at register **b1** for material parameters.
This cbuffer is used by the bindless materials system to pass per-material static constants
(such as bindless texture/sampler indices from ``@staticSmp``/``@staticTex`` declarations) to the shader.

Use this directive in **shader blocks** when shaders supporting the block declare material textures or other
static material properties. The reserved register (b1) will not be assigned to any other resource by the compiler.

.. note::
  If a shader inherits the b1 reservation from a supported block but does not actually declare any static
  constants, the compiler will automatically unreserve the slot. This only happens at the shader level;
  blocks always keep the reservation.

.. code-block:: text

    block(scene) my_scene
    {
      supports __static_cbuf;

      // block-level bindings go here
      (vs) { world_view_pos@f3 = world_view_pos; }
    }

    shader my_shader
    {
      supports my_scene;

      // material textures -- their bindless indices are stored in the b1 cbuffer
      texture diffuse_tex = material.texture.diffuse;
      texture normal_tex = material.texture[1];
      (ps) {
        diffuse_tex@staticSmp = diffuse_tex;
        normal_tex@staticSmp = normal_tex;
      }
    }

.. note::
  This directive is unrelated to user-defined ``@cbuf`` bindings. ``supports __static_cbuf`` only reserves the
  internal material-params cbuffer at b1. You can still use ``@cbuf`` for your own constant buffers as usual.

supports __static_multidraw_cbuf
================================

``supports __static_multidraw_cbuf`` does everything ``__static_cbuf`` does (reserves b1 for material parameters),
but additionally enables **multidraw-optimized constant buffer packing**. With this directive, material parameters
from multiple draw calls are packed into a single buffer, enabling indirect multidraw rendering where many objects
with different materials can be drawn in one API call.

This directive is only meaningful on platforms that support bindless rendering:

- DX12
- PS4 / PS5
- Vulkan with bindless enabled

.. warning::
  When ``__static_multidraw_cbuf`` is enabled, **dynamic stcode must not depend on material parameters**.
  With multidraw, dynamic stcode runs once per combined draw call (not per individual draw).
  Mixing dynamic and static (material) operands in the same stcode expression will produce a compilation error.

.. code-block:: text

    if (hardware.dx12 || hardware.ps4 || hardware.ps5 || (hardware.vulkan && hardware.bindless))
    {
      supports __static_multidraw_cbuf;
    }

supports __draw_id
==================

``supports __draw_id`` provides the ``get_draw_id()`` HLSL function, which returns a per-draw-call identifier.
If the per-draw data is small enough to fit in a dword, it can be passed directly as the draw ID value.
Otherwise, the draw ID is used to index into buffers that store per-draw or per-material data.

Currently only supported on **DX12**, where the compiler generates a dedicated cbuffer at register b7 (space1)
containing a ``uint __draw_id``, and a ``get_draw_id()`` helper that reads from it.
The driver fills this value for each draw call automatically.

On **PS4/PS5** and **Vulkan**, ``get_draw_id()`` is not provided by this directive. Instead, the draw ID
is emulated through instance offsets and wave operations.
See the ``SUPPORT_MULTIDRAW`` / ``ENABLE_MULTI_DRAW`` macros in shader include files for platform-specific setup.

.. code-block:: text

    if (hardware.dx12)
    {
      supports __draw_id;
      hlsl(vs) {
        // get_draw_id() is now available
        // draw ID packs both instance offset and material offset
        uint draw_id = get_draw_id();
        uint inst_offset = (draw_id >> MATRICES_OFFSET_SHIFT) << 2;
        uint material_offset = draw_id & MATERIAL_OFFSET_MASK;
      }
    }
