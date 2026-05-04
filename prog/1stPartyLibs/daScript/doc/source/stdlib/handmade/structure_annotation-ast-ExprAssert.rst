Assert expression (`assert(x<13)`, or `assert(x<13, "x is too big")`, or `verify(foo()!=0)`)
Location of the expression in source code
Type of the expression
Runtime type information of the class of the expression (i.e "ExprConstant", "ExprCall", etc)
Expression generation flags
Expression flags
Expression print flags
Name of the asserted expression
Arguments of the assert expression
Whether the arguments failed to infer types
Location of the enclosure where the assert is used
Whether the assert is a verify expression (verify expressions have to have sideeffects, assert expressions cant)
