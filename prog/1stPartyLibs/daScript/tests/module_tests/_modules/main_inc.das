// main_inc.das — Include resolution test
//
// This test validates the include_get callback defined in project.das_project.
// When the compiler encounters `include sample.inc`, it calls include_get("sample.inc", ...)
// which returns the absolute path to _modules/include/sample.inc.
//
// sample.inc defines: def madd(a, b, c) { return a * b + c }
// So madd(1, 2, 3) = 1 * 2 + 3 = 5.
//
// Compiled with: project.das_project (usePak=true)

options gen2
include sample.inc

[export]
def test {
    feint("madd(1,2,3) = {madd(1,2,3)}\n")
    return true
}
