.. _builtin_functions:


==================
Built-in Functions
==================

.. index::
    single: Built-in Functions
    pair: Global Symbols; Built-in Functions

Builtin functions are function-like expressions that are available without any modules.
They implement inherent mechanisms of the language, in available in the AST as separate expressions.
They are different from standard functions (see :ref:`built-in functions <stdlib__builtin>`).

^^^^^^
Invoke
^^^^^^

.. das:function:: invoke(block_or_function, arguments)

    ``invoke`` calls a block, lambda, or pointer to a function (`block_or_function`) with the provided list of arguments.

(see :ref:`Functions <functions>`, :ref:`Blocks <blocks>`, :ref:`Lambdas <lambdas>`).

^^^^^^^^^^^^^^
Misc
^^^^^^^^^^^^^^

.. das:function:: assert(x, str)

    ``assert`` causes an application-defined assert if the `x` argument is false.
    ``assert`` can and probably will be removed from release builds.
    That's why it will not compile if the `x` argument has side effects (for example, calling a function with side effects).

.. das:function:: verify(x, str)

    ``verify`` causes an application-defined assert if the `x` argument is false.
    The ``verify`` check can be removed from release builds, but execution of the `x` argument stays.
    That's why verify, unlike ``assert``, can have side effects in evaluating `x`.

.. das:function:: static_assert(x, str)

    ``static_assert`` causes the compiler to stop compilation if the `x` argument is false.
    That's why `x` has to be a compile-time known constant.
    ``static_assert``s are removed from compiled programs.

.. das:function:: concept_assert(x, str)

    ``concept_assert`` is similar to ``static_assert``, but errors will be reported one level above the assert.
    That way applications can report contract errors.

.. das:function:: debug(x, str)

    ``debug`` prints string `str` and the value of `x` (like print). However, ``debug`` also returns the value of `x`, which makes it suitable for debugging expressions::

        let mad = debug(x, "x") * debug(y, "y") + debug(z, "z") // x*y + z





