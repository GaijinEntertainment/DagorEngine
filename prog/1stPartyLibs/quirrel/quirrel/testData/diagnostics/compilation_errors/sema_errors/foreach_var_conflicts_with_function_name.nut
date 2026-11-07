// EXPECT_ERROR: "conflicts with function name"
function f() {
  foreach (f in [1])
    println(1)
}
