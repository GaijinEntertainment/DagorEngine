local counter = 123

function external() {
  return 0
}

function foo() {
  external()
}

function [pure] bar() {
  external()
}

let a = @() external()+123
let b = @ [pure] () external()+123

println($"{foo.getfuncinfos().pure}")
println($"{bar.getfuncinfos().pure}")
println($"{a.getfuncinfos().pure}")
println($"{b.getfuncinfos().pure}")
