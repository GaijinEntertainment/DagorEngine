// EXPECTED: compile-time error
// Ternary produces table|array, assigned to table
// The array branch violates the constraint
function get_data(flag: bool): table {
    return flag ? { x = 1 } : [1, 2, 3]
}
return get_data
