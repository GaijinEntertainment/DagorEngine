Calls an ``[export, pinvoke]`` function in the named agent's context.  Similar to ``invoke_in_context`` but resolves the agent context automatically from the category name.

When *category* is an empty string (``""``), the call targets the **thread-local** debug agent's context instead of a globally named one.  There can be only one thread-local agent per thread, so no name is needed.  The thread-local path is faster because it skips the global agent map lookup.
