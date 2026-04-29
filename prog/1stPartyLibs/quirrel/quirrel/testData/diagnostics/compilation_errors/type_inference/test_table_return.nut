// EXPECTED: compile-time error - function returns table, declared as returning array
function make_table(): array {
    return { a = 1, b = 2 }
}
return make_table
