// main.das — Module resolution test: foo + bar with project file
//
// This test validates two module resolution mechanisms simultaneously:
//
//   1. .das_module resolution (foo): The project.das_project includes foo/foo.das_module,
//      which registers a custom resolver. `require foo` resolves to foo/foo_main.das,
//      which in turn requires foo.random (resolved to foo/foo_random.das).
//      foo::get_test_value() delegates to foo_random::get_test_value() → returns 13.
//
//   2. Relative path resolution (bar): `require bar.bar` is handled by module_default
//      in common.das_project. The dotted name "bar.bar" splits into ["bar","bar"],
//      producing the path bar/bar.das relative to this file's directory.
//      bar::get_test_value() = left(9) + middle(1) + right(3) = 13.
//
// Both values must equal 13 for the test to pass.
//
// Compiled with: project.das_project (usePak=true)

options gen2
require foo
require bar.bar

[export]
def test {
    let ftv = foo::get_test_value()
    let btv = bar::get_test_value()
    assert(ftv == btv)
    assert(ftv == 13)
    return true
}
