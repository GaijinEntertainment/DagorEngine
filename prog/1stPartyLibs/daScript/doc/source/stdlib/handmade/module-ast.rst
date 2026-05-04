The AST module provides access to the abstract syntax tree representation of daslang programs.
It defines node types for all language constructs (expressions, statements, types, functions,
structures, enumerations, etc.), visitors for tree traversal, and utilities for AST
construction and manipulation. This module is the foundation for writing macros, code
generators, and source-level program transformations.

All functions and symbols are in "ast_core" module, use require to get access to it.

.. code-block:: das

    require daslib/ast
