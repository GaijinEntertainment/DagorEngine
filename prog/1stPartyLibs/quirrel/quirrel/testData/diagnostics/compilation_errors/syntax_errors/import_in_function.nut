// EXPECT_ERROR: "import statements must be at the top of the file, after directives"
function foo() {
  import "math" as m
  return m
}
