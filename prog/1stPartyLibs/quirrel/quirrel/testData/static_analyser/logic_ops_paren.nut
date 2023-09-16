//-file:declared-never-used
//-file:const-in-bool-expr
local x = 1, y = 2, z = 3;

let a = x && y || z
let b = x || y && z
let c = (x && y) || z
let d = x || (y && z)

let e = x || y | z
let f = x || y & z
let g = x && y | z
let h = x && y & z