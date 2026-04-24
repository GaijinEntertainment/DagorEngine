Marks argument `a` as consumed, signaling to the compiler that it will not be used after this call, which enables move optimizations and avoids unnecessary clones. Equivalent to the `<-arg` syntax.
