options gen2
expect 30305:5

// direct loop: a uses b, b uses a
var g_a : int = g_b + 1
var g_b : int = g_a + 1

// loop via function
def get_d() : int {
    return g_d + 1
}
var g_c : int = get_d()
var g_d : int = g_c + 1
