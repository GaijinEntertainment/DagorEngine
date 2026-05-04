properties of the `ExprBlock` object.
Block is a closure, and not a regular expression list.
Block has a return statement.
When invoked, the block result is copied on return.
When invoked, the block result is moved on return.
Block is inside a loop.
Finally is to be visited before the body.
Finally is disabled.
AOT is allowed to skip make block generation, and pass [&]() directly.
AOT should not skip annotation data even if make block is skipped.
Block is eligible for collapse optimization.
Block needs to be collapsed.
Block has make block operation.
Block has early out (break/continue/return).
Block is a for loop body.
Block has exit by label (goto outside).
Block is a lambda block.
Block is a generator block.
