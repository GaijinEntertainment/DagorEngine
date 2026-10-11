// EXPECT_ERROR: "duplicate key"
const KEY = "a"
const t = {
    [KEY] = 1,
    a = 2
}
