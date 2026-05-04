// EXPECTED: compile-time error - default value "hello" doesn't match declared type int
// The default value is a string literal, and the declared type is int.
// This CAN be checked at compile time because the default value is known.
let [x: int = "hello"] = [42]
return x
