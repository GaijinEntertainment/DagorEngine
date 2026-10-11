// Pointer arithmetic whose pointee carries a dimension the parser already rejected. The
// dimension is negative, so the element count wrapped and the size read as 2^64-48.
options gen2
expect 30109, 30239

[export]
def pointer_math_on_unsized_pointee {
    var x = new class<block[]>() - 0
}
