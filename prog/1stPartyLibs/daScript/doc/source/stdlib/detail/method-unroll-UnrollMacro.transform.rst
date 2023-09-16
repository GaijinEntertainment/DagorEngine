This macro implements loop unrolling in the form of `unroll` function.
Unroll function expects block with the single for loop in it.
Moveover only range for is supported, and only with the fixed range.
For example:::

    var n : float4[9]
    unroll <|   // contents of the loop will be replaced with 9 image load instructions.
        for i in range(9)
            n[i] = imageLoad(c_bloom_htex, xy + int2(0,i-4))
