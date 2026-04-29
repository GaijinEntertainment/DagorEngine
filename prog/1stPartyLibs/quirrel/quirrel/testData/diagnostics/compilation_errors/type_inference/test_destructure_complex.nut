// EXPECTED: compile-time error - default value type doesn't match declared type
// In array destructuring, the default for z is a float but declared as string
function get_data(): array {
    return [1, "hello"]
}
let [x: int = 0, y: string = "default", z: string = 3.14] = get_data()
return [x, y, z]
