// EXPECTED: no error - chained function calls, final result is float, assigned to float
function to_float(x: int): float {
    return x.tofloat()
}
function double_it(x: float): float {
    return x * 2.0
}
local result: float = double_it(to_float(42))
return result
