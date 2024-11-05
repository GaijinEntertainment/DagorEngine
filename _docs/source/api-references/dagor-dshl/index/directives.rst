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

.. code-block:: c

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
    (ps) { diffuse_tex@static = tex; }
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

.. code-block:: c

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
