Apply module implements `defer` and `defer_delete` pattern, i.e. ability to attach a bit of code or a delete operation to a finally section of the block, without leaving the context of the code.

All functions and symbols are in "defer" module, use require to get access to it. ::

    require daslib/defer

