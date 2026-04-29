The IF_NOT_NULL module provides a null-safe call macro. The expression
``ptr |> if_not_null <| call(args)`` expands to a null check followed by
a dereferenced call: ``if (ptr != null) { call(*ptr, args) }``.

All functions and symbols are in "if_not_null" module, use require to get access to it.

.. code-block:: das

    require daslib/if_not_null
