The AST_USED module implements analysis passes that determine which AST nodes
are actually used in the program. This information is used for dead code
elimination, tree shaking, and optimizing generated output.

All functions and symbols are in "ast_used" module, use require to get access to it.

.. code-block:: das

    require daslib/ast_used
