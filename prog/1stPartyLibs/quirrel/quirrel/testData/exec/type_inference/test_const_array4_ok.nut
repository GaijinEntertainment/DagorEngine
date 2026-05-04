const ARR1 = [1, null]
const ARR2 = [4, 3]

function fn(x: table|array|int = true ? ARR1 : {}): null {}

return fn({})