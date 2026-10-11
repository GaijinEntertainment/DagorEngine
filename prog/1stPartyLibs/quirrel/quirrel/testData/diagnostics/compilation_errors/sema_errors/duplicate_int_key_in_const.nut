// EXPECT_ERROR: "duplicate key"
const t = {
    [1] = "a",
    [1.0] = "b",
    [1] = "c"
}
