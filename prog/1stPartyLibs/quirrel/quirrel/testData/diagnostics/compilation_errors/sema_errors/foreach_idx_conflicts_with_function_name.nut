// EXPECT_ERROR: "conflicts with function name"
function f() {
  foreach (f, v in [1])
    println(v)
}
