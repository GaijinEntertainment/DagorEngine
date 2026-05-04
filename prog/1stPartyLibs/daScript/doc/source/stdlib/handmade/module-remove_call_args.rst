The REMOVE_CALL_ARGS module provides AST transformation macros that remove
specific arguments from function calls at compile time. Used for implementing
optional parameter patterns and compile-time argument stripping.

All functions and symbols are in "remove_call_args" module, use require to get access to it.

.. code-block:: das

    require daslib/remove_call_args
