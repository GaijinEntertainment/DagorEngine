New expression for ExprMakeLocal (`new [[Foo fld=val,...]]` or `new [[Foo() fld=...]]`, but **NOT** `new Foo()`)
Location of the expression in source code
Type of the expression
Runtime type information of the class of the expression (i.e "ExprConstant", "ExprCall", etc)
Expression generation flags
Expression flags
Expression print flags
Subexpression being ascended (newed)
Type being made
Location on the stack where the temp object is created, if necessary
Flags specific to `ExprAscend` expressions
