The QUOTE module provides quasiquotation support for AST construction.
It allows building AST nodes using daslang syntax with ``$``-prefixed
splice points for inserting computed values, making macro writing more
readable and less error-prone than manual AST construction.

All functions and symbols are in "quote" module, use require to get access to it.

.. code-block:: das

    require daslib/quote
