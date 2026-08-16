// verifies the compiler detects a recursive default argument expansion:
// a struct method whose default argument calls itself. The recursive named call
// can't resolve (30341) and the auto return type can't be inferred through the
// recursion, so the field type stays unresolvable.
expect 30161, 30237, 30341, 30344, 30805
struct v {
    def var0(var0 = 0; r = var0([var0=0])) {}
}
