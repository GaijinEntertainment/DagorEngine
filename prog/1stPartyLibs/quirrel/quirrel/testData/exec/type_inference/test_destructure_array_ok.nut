// EXPECTED: no error - destructured values with defaults match declared types
let [x: int = 4, y: float = 6.0] = [100, 200.4]
return [x, y]
