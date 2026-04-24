properties of the `ExprCopy` object.
This copy is allowed to copy a temporary value.
Its 'foo = [MakeLocal]' and temp stack value is allocated by copy expression.
Promote to clone, i.e. this is 'foo := bar' and not 'foo = bar'
