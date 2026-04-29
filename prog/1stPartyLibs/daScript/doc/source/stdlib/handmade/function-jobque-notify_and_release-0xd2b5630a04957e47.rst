Decreases the entry count **and** the reference count of a ``Channel`` or ``JobStatus``
in a single operation.  After the call the channel/status variable is set to ``null``.

Use ``notify_and_release`` inside lambdas that captured the channel.  Capturing adds a
reference, so the lambda must release it when done.  This function combines
``notify`` + ``release`` into one atomic step and nulls the variable to prevent
accidental reuse.

If the caller does **not** own a reference (e.g. the channel was passed as a plain
argument via ``invoke_in_context``, with no lambda capture), use ``notify`` instead —
calling ``notify_and_release`` in that case would release a reference the caller never
added, leading to a premature free.
