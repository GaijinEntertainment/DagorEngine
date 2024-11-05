/*

This test shows that it is possible to convert plus and\or concat operations to string substituation
Which is faster

*/

let {clock} = require("datetime")

function profile_it(cnt, f) {//for quirrel version
  local res = 0
  for (local i = 0; i < cnt; ++i) {
    let start = clock()
    f()
    let measured = clock() - start
    if (i == 0 || measured < res)
      res = measured
  }
  return res / 1.0
}

const foo = "foo"
local bar = "bar"
local num = 1234
local somestring = " somestring"

let plus = @(i) foo + "_" +bar+"_"+num +"_" +somestring + i
let concat = @(i) "".concat(foo, "_",bar,"_",num,"_",somestring, i)
let fstring = @(i) $"{foo}_{bar}_{num}_ {somestring} {i}"

function test(iters, func) {
  local res
  local i = iters
  while(--i){
    res = func(i)
  }
  return res
}


const numTests = 20
const iters = 200000
function profile(name, func){
  println("".concat(name, ", ", profile_it(numTests, func), $", {numTests}"))
}
profile("\"string plus\"", @() test(iters, plus))
profile("\"string concat\"",@() test(iters, concat))
profile("\"string format\"", @() test(iters, fstring))
