// EXPECTED: compile-time error
// Multiple functions with known return types composed together
// make_pair returns array, get_name returns string
// ternary: array|string => not subset of int
function make_pair(a: int, b: int): array {
    return [a, b]
}
function get_name(): string {
    return "hello"
}
function pick(flag: bool): int {
    return flag ? make_pair(1, 2) : get_name()
}
return pick
