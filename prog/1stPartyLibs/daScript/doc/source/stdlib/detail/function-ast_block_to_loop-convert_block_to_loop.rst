Converts closure block to loop.
If `failOnReturn` is true, then returns are not allowed inside the block.
If `replaceReturnWithContinue` is true, then `return cond;` are replaced with `if cond; continue;`.
If `requireContinueCond` is false, then `return;` is replaced with `continue;`, otherwise it is an error.
