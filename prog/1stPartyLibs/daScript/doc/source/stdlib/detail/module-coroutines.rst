The COROUTINES module exposes coroutine infrastructure, as well as additional yielding facilities.

The following example illustrates iterating over the elements of a tree. `each_async_generator` implements straight up iterator,
where 'yield_from' helper is used to continue iterating over leafs. `[coroutine]` annotation converts function into coroutine.
If need be, return type of the function can specify coroutine yield type::

    require daslib/coroutines

    struct Tree
        data : int
        left, right : Tree?

    // yield from example
    def each_async_generator(tree : Tree?)
        return <- generator<int>() <|
            if tree.left != null
                yeild_from <| each_async_generator(tree.left)
            yield tree.data
            if tree.right != null
                yeild_from <| each_async_generator(tree.right)
            return false

    // coroutine as function
    [coroutine]
    def each_async(tree : Tree?) : int
        if tree.left != null
            co_await <| each_async(tree.left)
        yield tree.data
        if tree.right != null
            co_await <| each_async(tree.right)

All functions and symbols are in "coroutines" module, use require to get access to it. ::

    require daslib/coroutines
