Decreases the channel's entry count, signaling that one unit of work has completed.

Use ``notify`` when the caller does **not** own a reference to the channel — for example
when a ``Channel?`` is passed as a plain function argument via ``invoke_in_context``.
In that scenario no lambda captures the channel, so no extra reference was added and
there is nothing to release.

Compare with ``notify_and_release``, which additionally releases a reference and should
be used inside lambdas that captured the channel (adding a reference).
