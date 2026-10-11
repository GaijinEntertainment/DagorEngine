// EXPECT_ERROR: "duplicate key"
let t = const {
    a = 1,
    a = 2
}
