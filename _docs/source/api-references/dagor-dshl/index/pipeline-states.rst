.. _pipeline-states:

============================
Pipeline state configuration
============================

DSHL has some state variables that are used for configuring fixed pipeline stages (blending, depth/stencil testing, etc.).
You can optionally define them inside a shader.

--------
Blending
--------

Blending settings can be configured in DSHL shader using the following syntax:

.. code-block:: c

  blend_target = blend_factor;

where ``blend_target`` is one of these state variables

- ``blend_src`` -- blend source
- ``blend_dst`` -- blend destination
- ``blend_asrc`` -- blend alpha source
- ``blend_adst`` -- blend alpha destination

and ``blend_factor`` is one of these keywords:

+-------------------+---------------------------+----------------------------------+--------------------+
| Keyword           | Description               | RGB Blend Factor                 | Alpha Blend Factor |
+===================+===========================+==================================+====================+
| ``zero`` or ``0`` | Zero                      | (0, 0, 0)                        |                  0 |
+-------------------+---------------------------+----------------------------------+--------------------+
| ``one`` or ``1``  | One                       | (1, 1, 1)                        |                  1 |
+-------------------+---------------------------+----------------------------------+--------------------+
| ``sc``            | Source Color              | (R_s, G_s, B_s)                  | A_s                |
+-------------------+---------------------------+----------------------------------+--------------------+
| ``isc``           | Inverse Source Color      | (1 - R_s, 1 - G_s, 1 - B_s)      | 1 - A_s            |
+-------------------+---------------------------+----------------------------------+--------------------+
| ``sa``            | Source Alpha              | (A_s, A_s, A_s)                  | A_s                |
+-------------------+---------------------------+----------------------------------+--------------------+
| ``isa``           | Inverse Source Alpha      | (1 - A_s, 1 - A_s, 1 - A_s)      | 1 - A_s            |
+-------------------+---------------------------+----------------------------------+--------------------+
| ``dc``            | Destination Color         | (R_d, G_d, B_d)                  | A_d                |
+-------------------+---------------------------+----------------------------------+--------------------+
| ``idc``           | Inverse Destination Color | (1 - R_d, 1 - G_d, 1 - B_d)      | 1 - A_d            |
+-------------------+---------------------------+----------------------------------+--------------------+
| ``da``            | Destination Alpha         | (A_d, A_d, A_d)                  | A_d                |
+-------------------+---------------------------+----------------------------------+--------------------+
| ``ida``           | Inverse Destination Alpha | (1 - A_d, 1 - A_d, 1 - A_d)      | 1 - A_d            |
+-------------------+---------------------------+----------------------------------+--------------------+
| ``sasat``         | Source Alpha Saturation   | (f, f, f); f = min(A_s, 1 - A_d) |                  1 |
+-------------------+---------------------------+----------------------------------+--------------------+
| ``bf``            | Blend Constant            | (R_c, G_c, B_c)                  | A_c                |
+-------------------+---------------------------+----------------------------------+--------------------+
| ``ibf``           | Inverse Blend Constant    | (1 - R_c, 1 - G_c, 1 - B_c)      | 1 - A_c            |
+-------------------+---------------------------+----------------------------------+--------------------+

where

- ``(R_s, G_s, B_s)`` and ``A_s`` represent R, G, B, A components of the source (new value which shader just produced)
- ``(R_d, G_d, B_d)`` and ``A_d`` represent R, G, B, A components of the destination (old value in the current render target)
- ``(R_c, G_c, B_c)`` and ``A_c`` represent configurable blend constants

Blending is performed using the following pseudocode:

.. code-block:: c

  if (blendEnable) {
    finalColor.rgb = (srcColorBlendFactor * srcColor.rgb) <colorBlendOp> (dstColorBlendFactor * dstColor.rgb);
    finalColor.a = (srcAlphaBlendFactor * srcColor.a) <alphaBlendOp> (dstAlphaBlendFactor * dstCoilor.a);
  } else {
      finalColor = srcColor;
  }

  finalColor = finalColor & colorWriteMask;

Example:

.. code-block:: c

  blend_src = sa; blend_dst = 1;
  blend_asrc = sa; blend_adst = 1;

.. warning::
  Blending *operations* (as well as blending constants for ``bf`` and ``ibf`` blending modes) need to be configured in C++ code.

If you don't configure blending factors in the shader, these defaults will be used:

.. code-block:: c

  blend_src = 1; blend_dst = 0;
  blend_asrc = 1; blend_adst = 0;

**Independent blending**

Dagor supports independent blending, i.e. different render targets may have different blending settings.
This is done by providing an index to ``blend_src, blend_dst, blend_asrc, blend_adst`` variables with ``[]`` operator.

.. code-block:: c

  blend_src[0] = 1; blend_dst[0] = 0;
  blend_src[1] = 0; blend_dst[1] = 1;

Without an index, the desired blend factor affects all render targets.

-------------
Depth/Stencil
-------------

Depth and stencil tests can also be configured in DSHL using similar syntax.

**Depth state variables**

``z_write`` -- enables/disables writing to depth buffer. Valid values: ``true`` / ``false``. Default value: ``true``.

``z_test`` -- enables/disables depth testing. Valid values: ``true`` / ``false``. Default value: ``true``.

``z_func`` -- specifies comparison function for depth testing. Valid values:

- ``equal`` evaluates to :math:`\text{reference} = \text{test}`
- ``notequal`` evaluates to :math:`\text{reference} \neq \text{test}`
- ``always`` always evaluates to ``true``

Default value is ``GREATER_OR_EQUAL`` (evaluates to :math:`\text{reference} \geq \text{test}`).
Note that this value cannot be set explicitly, as it is the default behavior,
because Dagor follows the convention that **closer** objects have **more** depth value.

Example:

.. code-block:: c

  z_write = false;
  z_test = true;
  z_func = always;

**Stencil state variables**

``stencil`` -- enables/disables stencil test. Valid values: ``true`` / ``false``. Default value: ``false``.

``stencil_func`` -- specifies comparison function for stencil testing. Valid values:

- ``never`` always evaluates to ``false``
- ``less`` evaluates to :math:`\text{reference} < \text{test}`
- ``equal`` evaluates to :math:`\text{reference} = \text{test}`
- ``lessequal`` evaluates to :math:`\text{reference} \leq \text{test}`
- ``greater`` evaluates to :math:`\text{reference} > \text{test}`
- ``notequal`` evaluates to :math:`\text{reference} \neq \text{test}`
- ``greaterequal`` evaluates to :math:`\text{reference} \geq \text{test}`
- ``always`` always evaluates to ``true``

``stencil_ref`` -- specifies reference value for the stencil test. Valid values: ``int``, clamped to ``[0, 255]`` range.

``stencil_pass`` -- specifies the action performed on samples that pass both stencil and depth tests.

``stencil_fail`` -- specifies the action performed on samples that fail the stencil test.

``stencil_zfail`` -- specifies the action performed on samples that pass the stencil test and fail the depth test.

Valid actions:

- ``keep`` -- keeps the current value.
- ``zero`` -- sets the value to ``0``.
- ``replace`` -- sets the value to reference ``stencil_ref`` value
- ``incrsat`` -- increments the current value and clamps to ``[0, 255]``
- ``decrsat`` -- decrements the current value and clamps to ``[0, 255]``
- ``incr`` -- increments the current value and wraps to ``0`` when the maximum value would have been exceeded.
- ``decr`` -- decrements the current value and wraps to ``255`` the maximum possible value when the value would go below ``0``.

Example:

.. code-block:: c

  stencil = true;
  stencil_func = always;
  stencil_pass = replace;
  stencil_ref = 255;
  stencil_zfail = keep;
  stencil_fail = keep;

-------
Culling
-------

``cull_mode`` specifies culling mode.

- ``ccw`` -- counterclockwise.
- ``cw`` -- clockwise.
- ``none`` -- no culling is done.

Default value is ``ccw``.

Example:

.. code-block:: c

  cull_mode = cw;

-----------------
Alpha to coverage
-----------------

Alpha to coverage (A2C) maps the alpha output value from a pixel shader
to the coverage mask of MSAA.
This feature might be helpful for smoothing out the edges of alpha-tested texture (e.g. vegetation).

For example, with 4x MSAA and A2C enabled, if the output alpha of a pixel is 0.5,
only 2 of the 4 coverage samples will store the color.

Can be toggled by setting ``alpha_to_coverage`` to ``true`` / ``false``.

---------------
View instancing
---------------

View instancing feature allows a shader to be run multiple times in a single draw call to draw different view instances.
The ``SV_ViewID`` semantic is provided to the shader which defines the index of the view instance.
This feature is supported only on DX12 and enforces Shader Model 6.1 compilation.

Maximum number of view instances is defined by ``MAX_VIEW_INSTANCES = 4``.
For example, using view instancing, you can capture a cube shadow map for a point light in 2 render passes with 3 view instances per pass.

Valid syntax is ``view_instances = 1..4``.

``view_instances = 1`` means one instance (which is the default case).

.. _color-write-mask:

----------------
Color write mask
----------------

Color write mask can be configured with ``color_write`` state variable.
You can set the mask via RGBA swizzle or by an ``int`` number (in ``[0, 15]`` range).
So ``r = 0b0001``, ``g = 0b0010``, ``b = 0b0100``, ``a = 0b1000``.

For example,

.. code-block:: c

  color_write = rg;
  // color_write = 3 is the same
  // 3 = 0b0011 = rg

Color mask supports multiple render targets, i.e. for each render target the mask can be different.
You can use ``[]`` operator to specify the mask for the specific render target.

.. code-block:: c

  color_write = true; // sets RGBA for all RT
  color_write[1] = rg; // sets RG for RT[1]

By default, color mask is RGBA for all render targets.
