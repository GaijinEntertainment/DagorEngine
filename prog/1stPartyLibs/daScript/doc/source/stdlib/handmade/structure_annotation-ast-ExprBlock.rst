Any block expression, including regular blocks and all types of closures.
Location of the expression in source code
Type of the expression
Runtime type information of the class of the expression (i.e "ExprConstant", "ExprCall", etc)
Expression generation flags
Expression flags
Expression print flags
List of expressions in the main body of the block
List of expressions in the 'finally' section of the block
Declared return type of the block, if any (for closures)
List of arguments for the block (for closures)
Stack top offset for the block declaration
Where variables of the block start on the stack
Where variables of the block end on the stack
Variables which are to be zeroed, if there is 'finally' section of the block. If there is 'inscope' variable after the return, it should be zeroed before entering the block.
Maximum label index used in this block (for goto statements)
AnnotationList - Annotations attached to this block
Opaque data associated with block
Opaque data source unique-ish id associated with block
Block expression flags
Which function this block belongs to
