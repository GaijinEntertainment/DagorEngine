.. _intervals:

=========
Intervals
=========

Intervals are a way to generate multiple variants of a single ``shader``, based on whether the value of a special variable falls into specific range.
They can be created from an ``int`` or ``float`` shader variable using the ``interval`` keyword.
Consider this example:

.. code-block:: c

  int var;
  interval var:below_zero<0, zero<1, positive_less_than_four<4, four<5, five_or_higher;
  // same goes for floats

This declaration will create 5 permutations of the shader (``below_zero``, ``zero``, ``positive_less_than_four``, ``four`` and ``five_or_higher``),
and the ``var`` interval itself can be accessed in hlsl blocks with special preprocessor tag ``##``.

.. code-block:: c

  int var;
  interval var:below_zero<0, zero<1, positive_less_than_four<4, four<5, five_or_higher;

  shader example_shader {
    hlsl {
      ##if var == five_or_higher
        ...
      ##endif
    }
  }

It is essential to understand that this construction doesn't mean that ``var`` equals to ``5``, but
it means ``var`` is within the declared interval ``five_or_higher``, so this syntax applies to ``float`` numbers as well.

.. warning::
  Notice that intervals can be created not only from global variables (as in example above), but also from ``static`` and ``dynamic`` variables (described in :ref:`local-variables`).

  Intervals on ``static`` variables are resolved (appropriate shader variant is selected) only once during material instantiation, so
  shader variants dependant on ``static`` variables are not switched during runtime.

  Intervals on ``dynamic`` and global variables, however, are resolved each time the shader is used for rendering (on ``setStates()`` call), because such variables can change during runtime.
  This has worse CPU performance impact, compared to ``static`` intervals.

------------------
Optional intervals
------------------

If the interval is used in HLSL code blocks, you can make this interval ``optional``.
All conditions in HLSL code which use ``optional`` intervals will be replaced with HLSL branches, thus reducing the number of shader variants.

.. warning::
  Shaders with ``optional`` intervals can be used only for conditionals in HLSL blocks and need to be compiled with ``-optionalAsBranches`` flag
  (otherwise ``optional`` intervals will act the same as regular intervals).
  This feature should only be used for dev build.

.. code-block:: c

  int test_optional = 0;
  optional interval test_optional : zero < 1, one < 2, two < 3, three;

  shader testMaterial
  {
    // ...
    hlsl
    {
      float3 color = float3(0, 0, 0);
  ##if test_optional == one
      color = float3(1, 0, 0);
  ##elif test_optional == two || test_optional == three
      color = float3(0, 0, 1);
  ##if test_optional == two
      color *= 0.5;
  ##endif

  ##else
      color = float3(0, 0, 1);
  ##endif
    }
  }

-------
Assumes
-------

Shader variables can be assigned a fixed value when the shader is compiled by *assuming*. Such shader vars may not be changed at runtime, their values will be constant in the binary.
This allows to reduce number of shader variants or disable specific features for specific platforms.

You can *assume* intervals inside a config .blk file for the shader compiler.
To do this, create an ``assume_vars`` block inside a ``Compile`` block and then specify the assumes you want, following regular .blk syntax:

.. code-block:: c

  Compile
  {
    // ...
    assume_vars
    {
      include common_assumes.blk
      supports_sh_6_1:i = 0
      static_shadows_custom_fxaa:i=0
      grass_use_quads:i=0
      bloom_tex:i = 1
    }
  }

.. note::
  By assuming a texture, e.g. ``bloom_tex:i = 1``, you declare that this texture is never ``NULL``.

**Assume statement**

You can use ``assume interval_name = interval_value;`` statement in DSHL shaders to force an interval to be fixed.
This can be useful to disable unnecessary shader variants when the same interval is shared among different shaders.

.. code-block:: c

  include "deferred_shadow_common.dshl"

  shader deferred_shadow_classify_tiles
  {
    assume use_ssss = off;
    // ...
  }
