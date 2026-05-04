// EXPECT_ERROR: "foreach() key and value names are the same"
let arr = [1, 2, 3]
foreach (x, x in arr) {
    print(x)
}
