// EXPECTED: no error
// Ternary produces table|array, declared as table|array
function get_data(flag: bool): table|array {
    return flag ? { x = 1 } : [1, 2, 3]
}
return get_data
