// EXPECT_ERROR: "conflicts with function name"
function f() {
  try { throw 1 } catch (f) println(1)
}
