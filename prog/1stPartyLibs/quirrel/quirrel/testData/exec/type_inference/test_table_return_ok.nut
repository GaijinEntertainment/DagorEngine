// EXPECTED: no error
function make_table(): table {
    return { a = 1, b = 2 }
}
function make_array(): array {
    return [1, 2, 3]
}
function make_either(flag: bool): table|array {
    return flag ? { a = 1 } : [1, 2]
}
return [make_table(), make_array(), make_either(true)]
