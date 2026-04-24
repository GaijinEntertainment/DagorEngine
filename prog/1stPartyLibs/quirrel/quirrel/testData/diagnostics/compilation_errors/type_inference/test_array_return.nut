// EXPECTED: compile-time error - function returns array, declared as returning table
function make_array(): table {
    return [1, 2, 3]
}
return make_array
