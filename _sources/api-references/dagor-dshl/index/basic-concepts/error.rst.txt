***************************
Throwing compilation errors
***************************

Using the ``error`` intrinsic, you can cause a compilation error with a message:

.. code-block:: c

  if (gi_quality == only_ao) {
  // ..
  } else if (gi_quality == high) {
    // ..
  } else {
    error("Unimplemented gi quality");
  }

Compiler output:

.. code-block::

  [ERROR] ../../../prog/gameLibs/render/shaders/debugGbuffer.dshl(41,9): "Unimplemented gi quality"