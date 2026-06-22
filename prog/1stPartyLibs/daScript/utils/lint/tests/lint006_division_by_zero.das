options gen2
// LINT006: Division by zero (constant zero divisor)
//
// Problem:
//   `x / 0` and `x % 0` produce a runtime panic or undefined division.
//   When the right-hand side is a literal zero, this is almost always a typo.
//
// Bad pattern:
//   let y = x / 0
//
// Good pattern:
//   let y = x / divisor

expect 50503:10

require daslib/lint

// --- Bad: division or modulo by literal zero across supported const types ---

def bad_div_int(x : int) : int {
    return x / 0                                    // LINT006
}

def bad_div_uint(x : uint) : uint {
    return x / 0u                                   // LINT006
}

def bad_div_int64(x : int64) : int64 {
    return x / 0l                                   // LINT006
}

def bad_div_uint64(x : uint64) : uint64 {
    return x / 0ul                                  // LINT006
}

def bad_div_float(x : float) : float {
    return x / 0.0f                                 // LINT006
}

def bad_div_double(x : double) : double {
    return x / 0.0lf                                // LINT006
}

def bad_mod_int(x : int) : int {
    return x % 0                                    // LINT006
}

def bad_mod_uint(x : uint) : uint {
    return x % 0u                                   // LINT006
}

def bad_div_assign(x : int) : int {
    var y = x
    y /= 0                                          // LINT006
    return y
}

def bad_mod_assign(x : int) : int {
    var y = x
    y %= 0                                          // LINT006
    return y
}

// --- Good: non-zero constant or variable divisor ---

def good_const_divisor(x : int) : int {
    return x / 7
}

def good_var_divisor(x : int; y : int) : int {
    return x / y
}

def good_modulo(x : int) : int {
    return x % 10
}

def good_float_nonzero(x : float) : float {
    return x / 2.0f
}
