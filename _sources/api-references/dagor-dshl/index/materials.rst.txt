.. _materials:

*********
Materials
*********

Material is a combination of a shader and resources (diffuse, normals, other parameters).

Often multiple assets share the same shader, but need to have different textures and properties.
In this case, each asset will have its own material *instance* with its specific resources.

DSHL has a way to access these textures in shaders in a general, non asset-specific way.

--------
Textures
--------

Textures can be referenced by using ``material`` keyword.

You, as a shader creator, specify how many ``material.texture[..]`` slots a shader must have just by referencing these slots in code.
Then, after shader compilation, referenced texture slots will appear in the shader bindump.
For instance, if you choose your shader in Asset Viewer material editor, these slots will be visible and you will be able to assign them the intended textures.

Usage example:

.. code-block:: c

  shader materials_example
  {
    texture diffuse = material.texture.diffuse;
    texture normal = material.texture[1];
    // that's it, material texture channels are now defined
    // and can be seen in asset viewer

    (ps)
    {
      diffuse_tex@static = diffuse;
      normal_tex@static = normal;
    }
    // now these textures are accessible in hlsl{} blocks
  }

.. note::
  ``material.texture.diffuse`` is equivalent to ``material.texture[0]``

.. _material-parameters:

----------
Parameters
----------

In addition to textures, materials can also contain arbitrary parameters.
You declare these parameters as local variables in a shader using ``static`` or ``dynamic`` keyword, check :ref:`local-variables` to know the difference.
It is good practice to initialize them with some meaningful default value.

Similar to textures, these parameters will be saved to the shader bindump and will be available for editing in Asset Viewer once you recompile the shader.

.. code-block:: c

  static float some_parameter = 1.5;
  dynamic float4 another_parameter = (1, 2, 3, 4);

  (ps)
  {
    some_parameter@f1 = some_parameter;
    another_parameter@f4 = another_parameter;
  }

.. warning::
  Do not use ``dynamic`` parameters without the need as it introduces more overhead. See :ref:`local-variables` for more info.

-------------------
Two sided rendering
-------------------

There are two special material parameters available in DSHL: ``two_sided`` and ``real_two_sided``, which are
present in **every** material by default and can be set in Asset Viewer.

These parameters act as booleans in DSHL:
you can do conditional statements :ref:`conditionals` on them (which will result in creation of shader variants).

.. code-block:: c

  if (two_sided)
  {
    cull_mode = none;
  }

``two_sided`` is a hint that each triangle of this material should be rendered from both sides,
so culling should be disabled for this shader (which is done in the example).

``real_two_sided`` is a hint that a special mesh will be used for rendering with this shader:
each triangle will be duplicated and flipped, so both sides of the mesh get to be rendered.
Note that it is not necessary to disable culling in this case as we have a *real* two-sided mesh (hence the name).

----------------------
render_stage directive
----------------------

Render stage can be specified for a shader using ``render_stage <stage_name>``.

.. code-block:: c

  shader materials_example
  {
    render_stage opaque;
    // ...
  }

It is used to distinguish between different materials in the ``ShaderMesh`` class
(which can contain many materials and meshes), based on the render stage.

For example, you can call ``ShaderMesh::getElems(STG_opaque)`` to get all materials from a ``ShaderMesh``, which have ``render_stage opaque`` specified in their DSHL shaders.
This can be useful if you want your materials to be drawn in a specific order.

Available stages:

- ``opaque`` -- opaque
- ``atest`` -- alpha test
- ``imm_decal`` -- immediate decals
- ``decal`` -- decals
- ``trans`` -- transparent
- ``distortion`` -- distortion

There is also a ``render_trans`` legacy alias for ``render_stage trans``.

**Color write mask**

It is possible to use ``static int`` material parameter for a color mask :ref:`color-write-mask`.
Notice that you are specifiyng the mask for **all** render targets at once.

.. code-block:: c

  static int writemask = 1904; // = (0b0111 << 4) | (0b0111 << 8)
  // where 0b0111 is a bitmask for RGB
  color_write = static writemask;

  // writemask of 1904 is equivalent to
  // color_write[0] = rgb;
  // color_write[1] = rgb;

This example sets the color write mask of render targets 1 and 2 to ``rgb`` (if you are using the default value of ``writemask`` material parameter).
Other render targets will have a mask of ``0b0000``, meaning nothing will be drawn to them.
