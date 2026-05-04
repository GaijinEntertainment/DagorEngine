// EXPECTED: no error - destructuring from function result (runtime check only)
function make_pair(): table {
    return {x = 10, y = 20}
}
let {x: int, y: int} = make_pair()
return [x, y]
