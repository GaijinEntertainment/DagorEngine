[instance_function(generic_name,type1=type1r,type2=type2r,...)] macro creates instance of the generic function with a particular set of types.
In the followin example body of the function inst will be replaced with body of the function print_zero with type int::

    def print_zero ( a : auto(TT) )
        print("{[[TT]]}\n")
    [export, instance_function(print_zero,TT="int")]
    def inst {}
