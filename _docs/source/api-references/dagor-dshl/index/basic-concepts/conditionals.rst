.. _conditionals:

============
Conditionals
============

**Outside HLSL blocks**

``if``, ``else``, ``else if`` directives are used to perform conditional compilation of different shader variants in DSHL.

Example:

.. code-block:: c

  texture tex;
  texture fallback;
  int test_interval = 0;
  interval test_interval:negative<0, less_than_two<2, more_than_two;

  shader test_shader
  {
    if (tex != NULL)
    {
      (ps) { tex@smp2d = tex; }
    }
    else
    {
      (ps) { tex@smp2d = fallback; }
    }

    if (test_interval == positive)
    {
      (vs) { var@f4 = (0, 0, 0, 0); }
    }
    else if (test_interval == less_than_two)
    {
      (vs) { var@f4 = (1, 0, 0, 0); }
    }
    else
    {
      (vs) { var@f4 = (1, 0, 1, 0); }
    }
  }

For each branch of the conditional statement, there will be created some number of shader variants (determined by combinatorics).
If branch is an exact duplicate of another, no variants will be created.
These variants are to be switched in runtime, based on the values in the conditionals.

**Inside HLSL blocks**

It is possible to use similar directives ``##if, ##elif, ##else, ##endif`` inside ``hlsl{...}`` blocks too with a special ``##`` preprocessor tag.
In this case, however, you need to close them with ``##endif`` in the end. Example:

.. code-block:: c

  hlsl
  {
  ##if tex != NULL
    float somefloat = 1.0;
  ##endif
  }

**maybe intrinsic**

``maybe()`` intrinsic can be used in a bool-expression.
Its argument is any identifier. If this identifier is not a boolean variable, the intrinsic will return false, otherwise the value of this boolean variable.

.. code-block:: c

  if (wsao_tex != NULL) {
    if (shader == ssao) {
      bool use_wsao = true;
    }
  }

  (ps) {
    if (maybe(use_wsao)) {
      foo@f4 = foo;
    }
  }
  hlsl(ps) {
    ##if maybe(use_wsao)
      return foo;
    ##endif
  }

If you try to remove the ``maybe()`` in this example, it will cause a compilation error, because the ``use_wsao`` is not declared in all variants.

**Creating multiple shaders with conditionals**

Conditionals can also be used to define multiple shaders at once (for convenience).
You just declare multiple shaders with the ``shader`` keyword and do a conditional statement on the shader's name:

.. code-block:: c

  shader one, two, three
  {
    if (shader == one)
    {
      float var = 1.0;
    }
    else if (shader == two)
    {
      float var = 2.0;
    }
    else
    {
      float var = 0.0;
    }
  }

This example will result in compilation of 3 different shaders, namely ``one``, ``two`` and ``three``,
which will have different values assigned to ``var``.

.. note::
  Do not confuse this feature with intervals :ref:`intervals`.
  It does not create variants of a single shader, but creates different shaders with different names.
