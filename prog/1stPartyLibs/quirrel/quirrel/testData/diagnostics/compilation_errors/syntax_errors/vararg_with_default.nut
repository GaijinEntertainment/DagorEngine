// EXPECT_ERROR: "function with default parameters cannot have variable number of parameters"
function foo(a = 1, ...) {
    return a
}
