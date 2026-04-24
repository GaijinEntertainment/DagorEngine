// EXPECTED: no error - default values match declared types
let [a: int = 10, b: string = "default", c: bool = true] = [1, "actual", false]
return [a, b, c]
