

let t = {}

let _p = t?.x || {}
let _x = {} || t?.y
let _y = t?.z && []
let _z = [] && t?.g


//-file:expr-cannot-be-null
