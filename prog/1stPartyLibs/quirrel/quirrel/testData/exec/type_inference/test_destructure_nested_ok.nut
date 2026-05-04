// EXPECTED: no error - nested destructuring with types
let {config} = {config = {width = 800, height = 600}}
let {width: int, height: int} = config
return [width, height]
