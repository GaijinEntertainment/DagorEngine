// EXPECT_ERROR: "directives must be at the top of the file, before imports"
function foo() {
  #allow-delete-operator
}
