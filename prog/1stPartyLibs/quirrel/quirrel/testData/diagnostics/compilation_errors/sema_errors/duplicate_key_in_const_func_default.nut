// EXPECT_ERROR: "duplicate key"
const function f(t = { a = 1, a = 2 }) {
    return t
}
