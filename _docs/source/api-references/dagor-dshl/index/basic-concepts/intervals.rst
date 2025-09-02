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

Assumes are evaluated irrespective of static and dynamic intervals: if you put an assume statement inside a
static or dynamic condition, it will be applied, and it will only be ignored if the condition is constant
irrespective of variants.

Assumes to the same interval override each other in program order, and assumes in code override assumes from the blk.
If you need an assume that doesn't override ones from blk or ones declared before as a default fallback, use
``assume_if_not_assumed interval_name = interval_value;``

.. code-block:: c

    include "deferred_shadow_common.dshl"

    assume use_ssss = off;

    shader deferred_shadow_classify_tiles
    {
      assume_if_not_assumed use_ssss = on; // use_ssss is still assumed to be off
      // ...
    }
