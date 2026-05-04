The STRINGIFY module provides the ``%stringify~`` reader macro for embedding
multi-line string literals verbatim. Text between ``%stringify~`` and ``%%``
is captured as-is without requiring escape sequences for quotes, braces,
or other special characters.

All functions and symbols are in "stringify" module, use require to get access to it.

.. code-block:: das

    require daslib/stringify
