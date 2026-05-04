// sample.inc — Include file providing the madd function
//
// This file is resolved by the include_get callback in project.das_project.
// When main_inc.das uses `include sample.inc`, the compiler calls include_get
// which returns the absolute path to this file.
//
// Defines: madd(a, b, c) = a * b + c
// Used by: main_inc.das to verify madd(1, 2, 3) = 5

def madd(a, b, c : int) {
    return a * b + c
}
