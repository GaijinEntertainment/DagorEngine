properties of the `ExprReturn` object.
Its 'return <- ...'.
Return a reference. Function result is a reference.
Return in block, not in function.
Take over right stack, i.e its 'return [MakeLocal]' and temp stack value is allocated by return expression.
Return call CMRES, i.e. 'return call(...)'.
Return CMRES, i.e. 'return [MakeLocal]' or 'return [CmresVariable]'
From yield.
From comprehension.
Skip lock checks.
