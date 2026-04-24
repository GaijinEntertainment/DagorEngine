// EXPECTED: no error - return type matches
function fn(): int {
    return 42
}
function fn2(): string {
    return "hello"
}
function fn3(): int|string {
    return 42
}
function fn4(): int|string {
    return "hello"
}
return [fn(), fn2(), fn3(), fn4()]
