The CONSUME module implements the ``consume`` pattern, which moves ownership
of containers and other moveable values while leaving the source in a
default-constructed state. This enables efficient ownership transfer.

All functions and symbols are in "consume" module, use require to get access to it.

.. code-block:: das

    require daslib/consume
