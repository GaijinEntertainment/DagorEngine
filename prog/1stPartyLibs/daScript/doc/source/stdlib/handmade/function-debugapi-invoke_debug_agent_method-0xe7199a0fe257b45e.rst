Calls a method on the debug agent's class instance by name.  The first argument is the agent category, the second is the method name, followed by up to 10 user arguments.  The agent's ``self`` is passed automatically.

When *category* is an empty string (``""``), the call targets the **thread-local** debug agent instead of a globally named one.  There can be only one thread-local agent per thread — that is why it needs no name.  The thread-local path is faster than a named agent lookup because it skips the global map search.
