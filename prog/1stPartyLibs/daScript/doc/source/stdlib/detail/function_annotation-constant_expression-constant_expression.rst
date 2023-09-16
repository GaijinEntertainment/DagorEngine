This function annotation implments constant expression folding for the given arguments.
When argument is specified in the annotation, and is passed as a contstant expression,
custom version of the function is generated, and an argument is substituted with a constant value.
This allows using of static_if expression on the said arguments, as well as other optimizations.
For example::

    [constant_expression(constString)]
    def take_const_arg(constString:string)
        print("constant string is = {constString}\n")   // note - constString here is not an argument

