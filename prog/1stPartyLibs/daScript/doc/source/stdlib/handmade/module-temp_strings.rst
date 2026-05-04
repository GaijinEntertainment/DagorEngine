The TEMP_STRINGS module provides temporary string construction that avoids heap
allocations. Temporary strings are allocated on the stack or in scratch memory
and are valid only within the current scope, offering fast string building
for formatting and output.

All functions and symbols are in "temp_strings" module, use require to get access to it.

.. code-block:: das

    require daslib/temp_strings
