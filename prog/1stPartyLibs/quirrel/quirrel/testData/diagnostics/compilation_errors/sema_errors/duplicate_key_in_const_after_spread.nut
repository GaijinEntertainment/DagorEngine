// EXPECT_ERROR: "duplicate key"
const SOURCE = { a = 1 }
const t = {
    b = 1,
    ...SOURCE,
    b = 2
}
