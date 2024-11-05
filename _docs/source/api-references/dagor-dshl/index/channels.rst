.. _channels:

========
Channels
========

Channels are used to declare the data that the vertex shader is going to get from the vertex buffer.
Channels must be defined within a ``shader{...}`` block via the following syntax:

.. code-block:: c

  channel (type) (usage_dst) = (usage_src) [ modifier ];

**Data types**

Data types that follow the ``channel`` keyword differ from those native DSHL types described in :ref:`data-types`.
These types are considered *convertable*, meaning that they are always casted to native HLSL data types when piped to
variables in ``hlsl{...}`` blocks.

This cast always happens to a 4-component vector of some basic scalar type, even if the source DSHL type is not a 4-component type.
For example, ``float2`` type with ``(x, y)`` components is casted to 4-component float vector via the rule ``(x, y, 0, 1)``.

The list below describes these *convertable* types as well as their casts to 4D vectors.

- ``float1`` -- 1D float ``(x, 0, 0, 1)``
- ``float2`` -- 2D float ``(x, y, 0, 1)``
- ``float3`` -- 3D float ``(x, y, z, 1)``
- ``float4`` -- 4D float ``(x, y, z, w)``
- ``short2`` -- 2D signed short ``(x, y, 0, 1)``
- ``short4`` -- 4D signed short ``(x, y, z, w)``
- ``ubyte4`` -- 4D uint8_t ``(x, y, z, w)``
- ``color8`` -- 4-byte ``(R, G, B, A)`` color in ``[0..1]`` range
- ``half2`` -- 2D 16-bit float ``(x, y, 0, 1)``
- ``half4`` -- 4D 16-bit float ``(x, y, z, w)``
- ``short2n`` -- 2D signed short normalized ``(x/32767.0, y/32767.0 , 0, 1)``
- ``short4n`` -- 4D signed short normalized ``(x/32767.0, y/32767.0 , z/32767.0, w/32767.0)``
- ``ushort2n`` -- 2D unsigned short normalized ``(x/32767.0, y/32767.0 , 0, 1)``
- ``ushort4n`` -- 4D unsigned short normalized ``(x/32767.0, y/32767.0 , z/32767.0, w/32767.0)``
- ``udec3`` -- 3D unsigned 10-bit float ``(x, y, z, 1)``
- ``dec3n`` -- 3D signed 10-bit float normalized ``(x/511.0, y/511.0, z/511.0, 1)``

**Data usages**

Shader resource builder needs to know which data to extract from the given mesh that will be sent to your shader.
This is why you need to specify channel's *usage*.

- ``pos`` -- position
- ``norm`` -- normal
- ``vcol`` -- vertex color
- ``tc`` -- texture coordinates
- ``lightmap`` -- lightmap, deprecated (legacy from the times when lightmaps were used)
- ``extra`` -- extra, can be used only as ``usage_src`` (used for providing additional info for vertices)

Usually, ``usage_src`` and ``usage_dst`` match. But sometimes, e.g. when the ``extra`` channel is used as ``usage_src``,
you must remap it to other intended ``usage_dst``.

**Data modifiers**

Data modifiers might be useful in some cases where the data from the channel should be rescaled or multiplied by some constant.

- ``signed_pack`` -- converts data from ``[0..1]`` range to ``[-1..1]`` range
- ``unsigned_pack`` -- converts data from ``[-1..1]`` range to ``[0..1]`` range
- ``bounding_pack`` -- rescales data from ``[min..max]`` to ``[0..1]`` if ``usage_dst`` is unsigned or ``[-1..1]`` if it is signed
- ``mul_1k`` -- multiplies data by ``1024``
- ``mul_2k`` -- multiplies data by ``2048``
- ``mul_4k`` -- multiplies data by ``4096``
- ``mul_8k`` -- multiplies data by ``8192``
- ``mul_16k`` -- multiplies data by ``16384``
- ``mul_32767`` -- multiplies data by ``32767`` and clamps it to ``[-32767..32767]`` range

Here is a simple usage example:

.. code-block:: c

  shader some_shader
  {
    /*
    Here we declare channels for the vertex shader, which
    will be used by the shader compiler to generate appropriate
    vertex shader declaration for this particular shader.
    */
    channel float3 pos=pos;       // vertex position
    channel color8 vcol=vcol;     // vertex color
    channel float3 tc[0] = tc[0]; // vertex texture coordinate
    channel float3 tc[1] = tc[1]; // another vertex texture coordinate

    hlsl(vs) {
      /*
      This VsInput struct declares vertex information in HLSL-style.
      Note that for each DSHL channel declared above, there should be a corresponding
      HLSL semantic matching the declared channel.
      */
      struct VsInput
      {
        float4 pos : POSITION;               // vertex position
        float4 color : COLOR0;               // vertex color
        float3 texcoord : TEXCOORD0;         // vertex texture coordinate
        float3 another_texcoord : TEXCOORD1; // another vertex texture coordinate
      };
    }

    hlsl(vs) {
      VsOutput some_vs(VsInput input)
      {
        // ...
      }
    }
  }
