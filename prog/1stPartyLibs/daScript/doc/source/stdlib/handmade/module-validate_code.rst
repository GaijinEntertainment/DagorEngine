The VALIDATE_CODE module implements AST validation passes that check for
common code quality issues, unreachable code, missing return statements,
and other semantic errors beyond what the type checker verifies.

All functions and symbols are in "validate_code" module, use require to get access to it.

.. code-block:: das

    require daslib/validate_code
