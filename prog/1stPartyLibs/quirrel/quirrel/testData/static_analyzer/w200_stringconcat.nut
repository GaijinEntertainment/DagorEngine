//-file:expr-cannot-be-null
//-file:plus-string
//-file:decl-in-expression

// Potentially nullable operand check (w200) explicitly allows nulls in string concatenation

let o = {}
let s = o?.x
let ss = "a" + o + s //-declared-never-used

