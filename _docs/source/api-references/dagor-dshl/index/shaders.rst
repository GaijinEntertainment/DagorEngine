.. _shaders:

*******
Shaders
*******

Dagor Shading Language acts as a preprocessor/compiler for pure HLSL shaders. In DSHL, we can bind resources
for HLSL shaders, configure fixed shader stages (culling, Z test, ...) and more.
Pure HLSL code needs to be contained within ``hlsl{...}`` blocks.

==============================
Defining and compiling shaders
==============================

Let's look at a simple DSHL shader example:

.. code-block:: c

  shader simple_shader
  {
    // this is the description of vertex buffer expected for the vertex shader
    channel float3 pos=pos; // position
    channel float3 vcol=vcol; // vertex color

    hlsl {
      struct VsInput
      {
        float3 pos: POSITION0;
        float3 color: COLOR0;
      };

      struct VsOutput
      {
        float4 pos : SV_POSITION;
        float3 color : COLOR0;
      };

      VsOutput test_vertex(VsInput input)
      {
        VsOutput ret;
        ret.pos = float4(input.pos, 1.0);
        ret.color = input.color;

        return ret;
      }

      float4 test_pixel(VsOutput input) : SV_Target0
      {
        return float4(input.color.rgb, 1.0);
      }
    }
    compile("target_vs", "test_vertex");
    compile("target_ps", "test_pixel");
  }

Here, ``shader (name)`` defines the actual name that the shader will have after its compilation into pure HLSL.

Channels ``pos`` and ``vcol`` describe the vertex buffer data that the vertex shaders expects to recieve.
DSHL preshader creates appropriate layout for the C++ code based on these ``channel`` variables. See :ref:`channels` for more info.

After defining the shader in the ``hlsl`` block, you need to specify its entry point via ``compile("target_(stage)", "entry_function")``, where the
``entry_function`` should be the name of the respective shader function in the ``hlsl`` block and ``stage`` defines one of the following shader stages:

- ``target_vs`` (vertex shader)
- ``target_hs`` (hull shader)
- ``target_ds`` (domain shader)
- ``target_gs`` (geometry shader)
- ``target_ps`` (pixel shader)
- ``target_cs`` (compute shader)
- ``target_ms`` (mesh shader)
- ``target_as`` (amplification shader)
- ``target_vs_for_gs`` (if vertex shader is used together with geometry shader on PS4/PS5, vertex shader must be compiled differently)
- ``target_vs_for_tess`` (if vertex shader is used together with tesselation shader on PS4/PS5, vertex shader must be compiled differently)
- ``target_vs_half`` (vertex shader with half type)
- ``target_ps_half`` (pixel shader with half type)

You can also specify to which specific shader stage will the code from ``hlsl`` block go by specifying the shader stage in the parentheses, e.g. ``hlsl(stage) {...}``
Available shaders are: ``ps``, ``vs``, ``cs``, ``ds``, ``hs``, ``gs``, ``ms``, ``as``. If you omit the specification, the code from ``hlsl{...}`` block will be
sent to all of these shaders.

.. _preshader:

=========
Preshader
=========

In addition to declaring just the shader code itself, DSHL allows you to declare a pre-shader,
which is a script that allows you to easily pipe data from C++ to the shader.

The most common use case for this piping are various bindings of textures and buffers:
instead of doing the classical dance of "pick the slot, set the texture to the slot, remember not to mess up and use the same slot twice",
you can bind variables to a shader through a global ``string`` to ``DSHL data type`` map called *shader variables*.
This map is in a 1-to-1 correspondence with the global DSHL variables you define in .dshl files :ref:`global-variables`, and is RW.

So you can, for example, both read an ``int`` defined inside a shader from C++, and set a texture to a global ``texture`` variable defined inside a shader.
On the C++ side, you simply fill in this map using ``set_texture``, and on the shader side, you ask the preshader system to grab a certain shader
variable and set it to an HLSL variable. The syntax is as follows:

.. code-block:: c

  (shader_stage) {
    hlsl_variable_name @type_suffix = variable|expression [hlsl {/*hlsl text*/}]
  }

This code is then compiled by our shader compiler into a sequence of simple interpreted commands,
which are stored in the shader dump and executed before running a shader.

Acceptable shader stages:

- ``cs`` -- Compute Shader
- ``ps`` -- Pixel Shader
- ``vs`` -- Vertex Shader
- ``ms`` -- Mesh Shader

Acceptable types:

+-----------------+--------------------------------------------+
| Postfix         | Type                                       |
+=================+============================================+
| @f1             | float                                      |
+-----------------+--------------------------------------------+
| @f2             | float2                                     |
+-----------------+--------------------------------------------+
| @f3             | float3                                     |
+-----------------+--------------------------------------------+
| @f4             | float4                                     |
+-----------------+--------------------------------------------+
| @f44            | float4x4                                   |
+-----------------+--------------------------------------------+
| @i1             | int                                        |
+-----------------+--------------------------------------------+
| @i2             | int2                                       |
+-----------------+--------------------------------------------+
| @i3             | int3                                       |
+-----------------+--------------------------------------------+
| @i4             | int4                                       |
+-----------------+--------------------------------------------+
| @u1             | uint                                       |
+-----------------+--------------------------------------------+
| @u2             | uint2                                      |
+-----------------+--------------------------------------------+
| @u3             | uint3                                      |
+-----------------+--------------------------------------------+
| @u4             | uint4                                      |
+-----------------+--------------------------------------------+
| @tex            | Texture                                    |
+-----------------+--------------------------------------------+
| @tex2d          | Texture2D                                  |
+-----------------+--------------------------------------------+
| @tex3d          | Texture3D                                  |
+-----------------+--------------------------------------------+
| @texArray       | Texture2DArray                             |
+-----------------+--------------------------------------------+
| @texCube        | TextureCube                                |
+-----------------+--------------------------------------------+
| @texCubeArray   | TextureCubeArray                           |
+-----------------+--------------------------------------------+
| @uav            | Unordered Access View flag                 |
+-----------------+--------------------------------------------+
| @smp            | Texture with SamplerState                  |
+-----------------+--------------------------------------------+
| @smp2d          | Texture2D with SamplerState                |
+-----------------+--------------------------------------------+
| @smp3d          | Texture3D with SamplerState                |
+-----------------+--------------------------------------------+
| @smpArray       | Texture2DArray with SamplerState           |
+-----------------+--------------------------------------------+
| @smpCube        | TextureCube with SamplerState              |
+-----------------+--------------------------------------------+
| @smpCubeArray   | TextureCubeArray with SamplerState         |
+-----------------+--------------------------------------------+
| @shd            | Texture2D with SamplerComparisonState      |
+-----------------+--------------------------------------------+
| @shdArray       | Texture2DArray with SamplerComparisonState |
+-----------------+--------------------------------------------+
| @buf            | Buffer/StructuredBuffer                    |
+-----------------+--------------------------------------------+
| @cbuf           | ConstantBuffer                             |
+-----------------+--------------------------------------------+
| @static         | Material Texture2D with SamplerState       |
+-----------------+--------------------------------------------+
| @staticCube     | Material TextureCube with SamplerState     |
+-----------------+--------------------------------------------+
| @staticTexArray | Material Texture2DArray with SamplerState  |
+-----------------+--------------------------------------------+
| @tlas           | Top-level acceleration structure (RT)      |
+-----------------+--------------------------------------------+

.. note::
  All variables declared in ``(vs)`` stage are also visible for ``hlsl(<gs, hs, ds>){...}`` blocks.
  All variables declared in ``(ms)`` stage are also visible for ``hlsl(as){...}`` block.

--------
Examples
--------

Let's create ``float4x4`` matrix:

.. code-block:: c

  (ps) { globtm_psf@f44 = { globtm_psf_0, globtm_psf_1, globtm_psf_2, globtm_psf_3 }; }

Here, the HLSL variable of ``globtm_psf`` will be initialized by the preshader with the values of ``globtm_psf_0..3``,
which are all ``float4`` types, stored inside the global shader variable map.
It is the C++ code's responsibility to call

.. code-block:: c

  set_color4(get_shader_variable_id("get_globtm_psf_X"), Point4(...));

for ``X=0..3`` to fill the rows with adequate values. Yes, the ``color4`` name is very unfortunate.

For ``(vs)`` block there is a built-in ``globtm`` *shader variable* available. You can declare HLSL ``globtm`` directly from it:

.. code-block:: c

  (ps) { globtm@f44 = globtm; }

You can also operate on arrays

.. code-block:: c

  (ps) { my_arr@type[] = {element1, element2, ..., elementN}; }

---------------------
Textures and samplers
---------------------

Default ``float4`` HLSL textures are defined via ``@tex2d, @tex3d, @texArray, @texCube, @texCubeArray`` postfixes.
For example, this code

.. code-block:: c

  (ps) {
    hlsl_texture@tex2d = some_texture;
    hlsl_texarray@texArray = some_texarray;
  }

will be compiled to

.. code-block:: c

  Texture2D hlsl_texture: register(t16);
  Texture2D hlsl_texarray: register(t17);
  // registers are automatically chosen by the compiler

Postfixes ``@smp2d, @smp3d, @smpArray, @smpCube, @smpCubeArray`` ensure that a ``SamplerState`` object gets defined with texture/textures,
assigned to the same register number.

For ``@shd, @shdArray`` postfixes, a ``SamplerComparisonState`` object also gets defined in addition to ``SamplerState``
(``shd`` stands for shadow, as these textures are often used for shadows).

For example, this code

.. code-block:: c

  (ps) {
    hlsl_texture@smp2d = some_texture;
    hlsl_texarray@smpArray = some_texarray;
    hlsl_shdtexture@shd = some_shdtexture;
  }

will be compiled to

.. code-block:: c

  SamplerState hlsl_texture_samplerstate: register(s0);
  SamplerState hlsl_texarray_samplerstate: register(s1);
  SamplerState hlsl_shdtexture_samplerstate: register(s2);

  SamplerComparisonState hlsl_shdtexture_cmpSampler:register(s2);

  Texture2D hlsl_texture: register(t0);
  Texture2DArray hlsl_texarray: register(t1);
  Texture2D hlsl_shdtexture: register(t2);

Note that you can use ``<texture_name>_samplerstate`` or ``<texture_name>_cmpSampler``, generated by the shader compiler, in ``hlsl{...}`` blocks
(e.g. ``hlsl_shdtexture_cmpSampler`` from the example).

Postfixes ``@tex`` and ``@smp`` define a texture of a specific type and must be followed by ``hlsl{...}`` block
(which defines the texture type).

.. code-block:: c

  (ps) {
    // textures without samplers
    uint_texture@tex = uint_texture hlsl { Texture2D<uint> uint_texture@tex; }
    float_texarray@tex = float_texarray hlsl { Texture2DArray<float> float_texarray@tex; }

    // textures with samplers
    uint_texture@smp = uint_texture hlsl { Texture2D<uint> uint_texture@smp; }
    float_texarray@smp = float_texarray hlsl { Texture2DArray<float> float_texarray@smp; }
  }

-----------------
Material textures
-----------------

Textures bound to a material (diffuse, normals, etc.) are called *material textures*.
In preshader, these textures must be treated differently than global or dynamic textures,
using ``@static, @staticCube, @staticTexArray`` postfixes.

.. code-block:: c

  shader example_shader
  {
    texture diffuse_tex = material.texture.diffuse;
    texture normal_tex = material.texture[1];
    texture cube_tex = material.texture[2];
    texture some_texarray = material.texture[3];

    (ps) {
      diffuse_tex@static = diffuse_tex;
      normal_tex@static = normal_tex;
      cube_tex@staticCube = cube_tex;
      some_texarray@staticTexArray = some_texarray;
    }
  }

Material textures are automatically used as bindless textures if you are compiling for DX12;
bindless is also supported for Vulkan and PlayStation (with special ``-enableBindless:on`` compiler flag).

Inside HLSL blocks, material textures should be referenced by their getters ``get_<texture_name>()``,
instead of their names:

.. code-block:: c

  hlsl(ps) {
    float4 albedo = tex2DBindless(get_diffuse_tex(), input.diffuseTexCoord.uv);
  }

.. note::
  Even if bindless textures feature is disabled, the aformentioned syntax still applies.

In case when bindless textures are used, ``MaterialProperties`` constant buffer will be filled with ``uint2``
indices of such textures (first component indexes the texture, second component indexes the sampler).

These indices are then used to retrieve the corresponding texture and sampler from ``static_textures[]`` and ``static_samplers[]`` arrays.

This is what ``get_<texture_name>()`` essentialy does for you.

-------
Buffers
-------

``Buffer`` and ``ConstantBuffer`` declarations must be followed with ``hlsl{...}`` block. For example

.. code-block:: c

  (ps) {
    some@buf = my_buffer hlsl {
      #include <myStruct.h>
      StructuredBuffer<MyStruct> some@buf;
    }
  }

  (ps) {
    my_buf@cbuf = my_const_buffer hlsl {
      #include <myStruct.h>
      cbuffer my_buf@cbuf
      {
        MyStruct data;
      };
    }
  }

-------------------
Hardcoded registers
-------------------

You can bind any resource to a hardcoded register, while all auto resources will not overlap with it.
Also, the ``always_referenced`` keyword is not required, the integer variable will be saved in the dump and will be readable on the CPU side.

.. code-block:: c

  int reg_no = 3;

  shader sh {
    (ps) {
      foo_vec@f4 : register(reg_no);
      foo_tex@smp2d : register(reg_no);
      foo_buf@buf : register(reg_no) hlsl { StructuredBuffer<uint> foo_buf@buf; };
      foo_uav@uav : register(reg_no) hlsl { RWStructuredBuffer<uint> foo_uav@uav; };
    }
  }

Register number must be declared as a global ``int`` variable.

.. note::
  With this method of declaring a resource, no stcode will be generated.

---------------------
Unordered Access View
---------------------

Unordered Access View ``@uav`` postfix provides a hint for the shader compiler that the resource should be bound to the appropriate ``u`` register.
Note that such declaration must be followed with the ``hlsl{...}`` block to define the actual type of the UAV resource.

.. code-block:: c

  buffer some_buffer;
  texture some_texture;

  shader some_shader {
    (cs) {
      hlsl_buffer@uav = some_buffer hlsl {
        RWStructuredBuffer<uint> hlsl_buffer@uav;
      }
      hlsl_texture@uav = some_texture hlsl {
        RWTexture2D<float4> hlsl_texture@uav;
      }
    }
    // ...
  }

--------------------------------
Top-level Acceleration Structure
--------------------------------

For raytracing purposes, you can also declare a TLAS (top-level acceleration structure) like this:

.. code-block:: c

  tlas some_tlas;

  shader some_shader {
    (cs) {
      hlsl_tlas@tlas = some_tlas;
    }
    // ...
  }

In HLSL terms, ``hlsl_tlas`` will have the type ``RaytracingAccelerationStructure``.

.. _shader-blocks:

=============
Shader blocks
=============

Shader Blocks are an extension of the Preshader idea and define variables/constants which are common for multiple shaders that ``support`` them.
The intent is to optimize constant/texture switching.
For example:

.. code-block:: c

  float4 world_view_pos;

  block(global_const|frame|scene|object) name_of_block
  {
    (ps) { world_view_pos@f3 = world_view_pos; }
    (vs) { world_view_pos@f3 = world_view_pos; }
  }

Note that a ``block``, just like a ``shader``, defines a preshader script.
This is basically the main gist of why blocks are useful:
they allow you to extract a part of a preshader common to multiple shaders and execute it once when setting the block, not every time a shader is executed.
In this example, ``world_view_pos`` would be visible within pixel and vertex shader in each shader that supports this block.

-------------------
Shader block layers
-------------------

Specifier in ``block(...)`` parentheses is called a layer. It indicates how often the values inside the block are supposed to change.
Available layers are:

- ``global_const`` (for global constants, supposed to change rarely)
- ``frame`` (for shader variables that are supposed to change once per frame)
- ``scene`` (for shader variables that are supposed to change when the rendering mode changes, within one frame)
- ``object`` (for shader variables that are supposed to change for each object)

.. warning::
  Per-object blocks are evil and should be avoided at all costs.
  They imply a draw-call-per-object model, which has historically proven itself antagonistic to performance.

Rendering modes mentioned in the ``frame`` layer are defined by the user and can be specific for each shader.
For example, there are ``4`` scene blocks in ``rendinst_opaque_inc.dshl`` shader, that are switched throughout the rendering of a single frame:

- ``rendinst_scene`` for color pass
- ``rendinst_depth_scene`` for depth pass
- ``rendinst_grassify_scene`` for grassify pass
- ``rendinst_voxelize_scene`` for voxelization pass

------------------------------
Using shader blocks in shaders
------------------------------

Syntax for using such blocks in shaders is as follows:

.. code-block:: c

  shader shader_name
  {
    supports some_block;
    supports some_other_block;

    hlsl(ps) {
      // assuming world_view_pos was defined in one of these blocks
      float3 multiplied_world_pos = 2.0 * world_view_pos;
      ...
    }
  }

With the support of multiple blocks you can use only variables from intersection of these blocks.
