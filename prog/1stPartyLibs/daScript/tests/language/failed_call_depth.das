// verifies compiler detects recursive default argument expansion
// struct method calls itself in its own default argument
options max_call_depth = 5
expect 41000:1
expect 30301:50
expect 30104:1
expect 30113:1
expect 31100:1
expect 30101:1
expect 30103:1
expect 0:1
struct v {
    def var0(var0 = 0; r = var0([var0=0])) {}
}
