properties of the `ExprMakeLocal` object (`ExprMakeArray`, `ExprMakeStruct`, 'ExprMakeTuple', 'ExprMakeVariant').
Use stack reference, i.e. there is an address on the stack - where the reference is written to.
Result is returned via CMRES pointer. Usually this is 'return <- [[ExprMakeLocal]]'
Does not need stack pointer, usually due to being part of bigger initialization.
Does not need field initialization, usually due to being fully initialized via constructor.
Initialize all fields.
Always alias the result, so temp value is always allocated on the stack.
