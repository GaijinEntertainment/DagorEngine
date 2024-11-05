.. _data-types:

========================
Data types and variables
========================

These are the data types which are native to DSHL (meaning they can be used outside of ``hlsl{...}`` blocks).

- Scalar types: ``float``, ``int``, ``bool``
- Vector 4D types: ``float4``, ``int4``
- Matrix 4x4 types: ``float4x4``
- Textures: ``texture``
- Buffers: ``buffer``, ``const_buffer``
- Raytracing structures: ``tlas``

There are also special *convertable* types in DSHL, which are used for processing vertex data input.
Refer to :ref:`channels` for more information.

.. _global-variables:

----------------
Global variables
----------------

Global variables in DSHL are most often set from C++ code during runtime.
They are declared with the following syntax:

.. code-block:: c

  (type) name [ = initializer ]  [always_referenced];

Example:

.. code-block:: c

  float4 f4 = (1.0, 1.0, 1.0, 1.0);
  int a;
  texture tex;

.. warning::
  Global variables cannot have ``float2`` and ``float3`` types.
  Type ``bool`` must have an initializer.
  Types ``float4x4, texture, buffer, const_buffer`` do not support initializers.

``always_referenced`` flag disallows shader compiler to remove any unused in global shader variables.
This is helpful when we want to access the variable on the CPU, for example.

.. _local-variables:

---------------
Local variables
---------------

Local variables are declared inside a ``shader{...}`` construction in a similar way:

.. code-block:: c

  [specifier] (type) name [ = initializer ] [no_warnings];

Local variables can have the following *specifiers*.

- ``local`` -- basically on-stack variables that are not visible outside the shader and are not visible on the CPU side, they are only needed for some temporary calculations in shader blocks :ref:`shader-blocks`.

- ``static`` -- means that the variable is a material parameter, which is set only once on material instantiation.

- ``dynamic`` -- similarly to ``static`` defines a material parameter, but it can be changed during runtime.

.. warning::
  Using ``dynamic`` (as well as global) shader variables is more expensive than ``static``,
  because stcode (responsible for setting these variables) is executed each time ``setStates()`` is called for the shader.

.. note::
  ``no_warnings`` is a modifier for ``static`` variables only. It is used when we need to access shader variables on the CPU,
  without using them in shaders (which normally triggers warnings).

--------------------
Textures and Buffers
--------------------

Textures and buffers are declared as global variables

.. code-block:: c

  texture some_tex;
  buffer some_buf;
  const_buffer some_constant_buf;

and then piped from C++ code to DSHL via the DSHL preshader, e.g.

.. code-block:: c

  (ps) {
    my_tex@smp2d = some_tex;

    my_buf@buf = some_buf hlsl {
      #include <some_buffer_inc.hlsli>
      // assuming some_buffer_inc.hlsli has SomeBuffer struct defined
      StructuredBuffer<SomeBuffer> my_buf@buf;
    }

    my_cbuf@cbuf = some_constant_buf hlsl {
      cbuffer my_cbuf@cbuf {
        #include <some_constant_buffer_inc.hlsli>
        // assuming some_constant_buffer_inc.hlsli has SomeConstantBuffer struct defined
        SomeConstantBuffer my_constant_buf;
      };
    }

    // my_tex, my_buf, my_constant_buf names are now accessible in HLSL
  }

Refer to :ref:`preshader` for more information.
