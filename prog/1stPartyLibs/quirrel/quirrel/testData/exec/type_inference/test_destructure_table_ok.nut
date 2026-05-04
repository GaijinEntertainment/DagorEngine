// EXPECTED: no error - destructured values match declared types
let {x: int, y: string} = {x = 42, y = "hello"}
let {a: int|null, b: float} = {a = null, b = 3.14}
return [x, y, a, b]
