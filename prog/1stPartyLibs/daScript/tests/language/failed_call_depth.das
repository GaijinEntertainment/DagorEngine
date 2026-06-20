// verifies compiler detects recursive default argument expansion
// struct method calls itself in its own default argument
options max_call_depth = 5
expect 30161, 30237, 30344, 30503, 30805
struct v {
    def var0(var0 = 0; r = var0([var0=0])) {}
}
