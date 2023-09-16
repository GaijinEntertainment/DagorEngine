This macro is used to remove unused (template) arguments from the instantiation of the generic function.
When [template(x)] is specified, the argument x is removed from the function call, but the type of the instance remains.
The call where the function is instanciated is adjusted as well.
For example::

    [template (a), sideeffects]
    def boo ( x : int; a : auto(TT) )   // when boo(1,type<int>)
        return "{x}_{typeinfo(typename type<TT>)}"
    ...
    boo(1,type<int>) // will be replaced with boo(1). instace will print "1_int"

