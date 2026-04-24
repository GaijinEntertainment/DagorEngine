New expression (`new Foo`, `new Bar(expr1..)`, but **NOT** `new [[Foo ...]]`)
Location of the expression in source code
Type of the expression
Runtime type information of the class of the expression (i.e "ExprConstant", "ExprCall", etc)
Expression generation flags
Expression flags
Expression print flags
Name of the new expression
List of arguments passed to the constructor
Whether any arguments failed to infer their types
Location of the expression in source code
Pointer to the constructor function being called, if resolved
Stack top at the point of call, if temporary variable allocation is needed
Type expression for the type being constructed
Whether there is an initializer for the new expression, or it's just default construction
