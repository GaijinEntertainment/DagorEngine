Invoke expression (`invoke(fn)` or `invoke(lamb, arg1, arg2, ...)`)
Location of the expression in source code
Type of the expression
Runtime type information of the class of the expression (i.e "ExprConstant", "ExprCall", etc)
Expression generation flags
Expression flags
Expression print flags
Name of the invoke expression
Arguments of the invoke expression
Whether the arguments failed to infer types
Location of the enclosure where the invoke is used
Stack top for invoke, if applicable
Does not need stack pointer
Is invoke of class method
If true, then CMRES aliasing is allowed for this invoke (and stack will be allocated)
