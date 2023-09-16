This macro converts yield_from(THAT) expression into::

    for t in THAT
        yield t

The idea is that coroutine or generator can continuesly yield from another sub-coroutine or generator.
