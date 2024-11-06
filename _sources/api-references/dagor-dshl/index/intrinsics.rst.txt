==========
Intrinsics
==========

Apart from common HLSL functions, DSHL supports special *intrinsic* functions,
which are handled by the shader compiler.

.. note::
  These functions are evaluated at runtime by the stcode system (unless they are known at compile time),
  so they cannot be used outside the :ref:`preshader` section.

--------------------------------------------
float time_phase(float period, float offset)
--------------------------------------------

Computes global time phase (number in range ``[0, 1)``) for specified period and offset.

Returns ``frac((shadersGlobalTime + offset) / period)`` or ``shadersGlobalTime`` in case ``period == 0``,
where ``shadersGlobalTime`` is just the time from the start of the game (in seconds).

.. code-block:: c

  (ps) { current_time@f1 = time_phase(0, 0); }

------------------
float sin(float x)
------------------

Computes ``sin`` function.

.. code-block:: c

  (ps) { foo@f1 = sin(3.14); }

------------------
float cos(float x)
------------------

Computes ``cos`` function.

.. code-block:: c

  (ps) { foo@f1 = cos(3.14); }

---------------------------
float pow(float x, float y)
---------------------------

Raises ``x`` to the power of ``y``.

.. code-block:: c

  (ps) { foo@f1 = pow(1.2, 3.4); }

--------------------------------
float4 vecpow(float4 v, float a)
--------------------------------

Raises each component of vector ``v`` to the power of ``a``.

.. code-block:: c

  local float4 v = (1, 2, 3, 4);
  local float a = 3.33;
  (ps) { foo@f1 = vecpow(v, a); }

-------------------
float sqrt(float x)
-------------------

Computes the square root of ``x``.

.. code-block:: c

  (ps) { foo@f1 = sqrt(1.3); }

---------------------------
float min(float x, float y)
---------------------------

Finds the minimum of two values.

.. code-block:: c

  (ps) { foo@f1 = min(-1, 1); }

---------------------------
float max(float x, float y)
---------------------------

Finds the maximum of two values.

.. code-block:: c

  (ps) { foo@f1 = max(-1, 1); }

-------------------------------------
float fsel(float a, float b, float c)
-------------------------------------

Returns ``(a >= 0.0f) ? b : c``

.. code-block:: c

  (ps) { foo@f1 = fsel(1, 2, 3); }

-------------------------
float4 sRGBread(float4 v)
-------------------------

Raises RGB components of ``v`` to the power of ``2.2``.

Returns ``float4(pow(v.r, 2.2f), pow(v.g, 2.2f), pow(v.b, 2.2f), v.a)``

.. code-block:: c

  (ps) { srgb_color@f3 = sRGBread(some_color); }
  // alpha will be discarded when casting to float3

-----------------------------------------
float4 get_dimensions(texture t, int mip)
-----------------------------------------

Fetches texture ``t`` information on mip level ``mip``.

Returns ``float4(width, height, depth_or_array_slices, mip_levels)``.

``depth_or_array_slices`` stores depth for volumetric textures or number of array slices for texture arrays.

For cube textures, ``depth_or_array_slices = 1``.


.. code-block:: c

  texture ssao_tex;
  (ps) { ssao_size@f4 = get_dimensions(ssao_tex, 0); }

------------------------
float get_size(buffer b)
------------------------

Fetches the size of buffer ``b`` in elements.

Returns ``float(buffer_size_in_elements)``.

.. code-block:: c

  buffer some_buf;
  (ps) { buf_size@f1 = get_size(some_buf); }

.. warning::
  ``get_size`` of ``(RW)ByteAddressBuffer`` will return the number of DWORDs (4 byte chunks) in a buffer.
  This is contrary to ``(RW)ByteAddressBuffer::GetDimensions`` pure HLSL function, which returns the number of bytes.

---------------------
float4 get_viewport()
---------------------

Fetches viewport information.

Returns ``float4(top_left_x, top_left_y, width, height)``.

.. code-block:: c

  (ps) { viewport@f4 = get_viewport(); }

-------------------------
int exists_tex(texture t)
-------------------------

At runtime, checks if the texture ``t`` exists (is bound to the shader from C++ side).
You can think of it as a run time equivalent of ``t != NULL`` preshader check.
Return value can be saved to a variable and used for uniform HLSL branching.

Usage:

.. code-block:: c

  texture example_texture;

  (ps) {
    example_texture@tex2d = example_texture;
    example_texture_exists@i1 = exists_tex(example_texture);
  }

  hlsl(ps) {
    if (example_texture_exists) { /* ... */ }
  }

Returns ``1`` if texture exists, ``0`` otherwise.

------------------------
int exists_buf(buffer b)
------------------------

Similar to ``exists_tex``.
At runtime, checks if the buffer ``b`` exists (is bound to the shader from C++ side).
You can think of it as a run time equivalent of ``b != NULL`` preshader check.
Return value can be saved to a variable and used for uniform HLSL branching.

Usage:

.. code-block:: c

  buffer example_buffer;

  (ps) {
    example_buffer@buf = example_buffer hlsl {
      StructuredBuffer<float> example_buffer@buf;
    };
    example_buffer_exists@i1 = exists_buf(example_buffer);
  }

  hlsl(ps) {
    if (example_buffer_exists) { /* ... */ }
  }

Returns ``1`` if buffer exists, ``0`` otherwise.
