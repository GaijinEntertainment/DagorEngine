This macro converts co_await(sub_coroutine) into::

    for t in subroutine
        yield t

The idea is that coroutine or generator can wait for a sub-coroutine to finish.
