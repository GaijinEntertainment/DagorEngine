Collect all finally blocks in the expression.
Returns array of ExprBlock? with all the blocks which have `finally` section
Does not go into 'make_block' expression, such as `lambda`, or 'block' expressions
