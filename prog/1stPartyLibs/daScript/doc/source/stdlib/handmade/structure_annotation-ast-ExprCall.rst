Anything which looks like call (`call(expr1,expr2)`).
Location of the expression in source code
Type of the expression
Runtime type information of the class of the expression (i.e "ExprConstant", "ExprCall", etc)
Expression generation flags
Expression flags
Expression print flags
Name of the call
List of arguments passed to the function
Whether any arguments failed to infer their types
Location of the expression in source code
Pointer to the function being called, if resolved
Stack top at the point of call, if temporary variable allocation is needed
If the call does not need stack pointer
If the call uses CMRES (Copy or Move result) aliasing, i.e would need temporary
If the call result is not discarded
