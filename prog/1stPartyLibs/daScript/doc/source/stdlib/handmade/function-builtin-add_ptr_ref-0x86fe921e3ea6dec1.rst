Wraps a raw pointer ``src`` of type ``TT?`` into a ``smart_ptr<TT>`` by incrementing the reference count.
Commonly used to bridge AST node fields (which are raw pointers like ``Structure?``, ``Enumeration?``) to API functions that expect ``smart_ptr<T>``.
The overload accepting ``smart_ptr<auto(TT)>`` adds an additional reference to an existing smart pointer, returning a new ``smart_ptr<TT>`` that shares ownership.
