// Regression: a parameter default value is evaluated (and, if typed, type-checked) at
// closure CREATION time, so closure hoisting (-optCH) must not move/dedupe a closure
// whose default has an observable creation-time effect. A side-effecting or allocating
// default would otherwise run a different number of times (loop) or unconditionally
// (dead branch) once hoisted. An untyped LITERAL default is still safely hoisted.
// See doc/closure_hoisting_block_scope_bug.md ("default-parameter side effects").

local count = 0
function bump() { count += 1; return count }

// side-effecting default inside a loop: created once per iteration, not hoisted out.
function sideEffectInLoop() {
  count = 0
  return function build() {
    let arr = []
    for (local i = 0; i < 3; i += 1)
      arr.append(function(x = bump()) { return x })
    return arr
  }
}

// side-effecting default in a never-taken branch: must not be created at all.
function deadBranch() {
  count = 0
  return function build() {
    if (false)
      return function(x = bump()) { return x }
    return null
  }
}

// allocating default: each closure must keep its own fresh array (not hoisted/shared).
function ownAllocation() {
  return function build() {
    let fns = []
    for (local i = 0; i < 2; i += 1)
      fns.append(function(a = []) { a.append(1); return a.len() })
    return fns
  }
}

// untyped literal default: safe, still hoisted, behaves identically.
function literalDefault() {
  return function build() {
    let fns = []
    for (local i = 0; i < 3; i += 1)
      fns.append(function(x = 5) { return x })
    return fns
  }
}

sideEffectInLoop()()
println("loop=" + count)
deadBranch()()
println("dead=" + count)
let a = ownAllocation()()
println("alloc=" + a[0]() + "," + a[0]() + "," + a[1]())
let l = literalDefault()()
println("literal=" + l[0]() + "," + l[1]() + "," + l[2]())
