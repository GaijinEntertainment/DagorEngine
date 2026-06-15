// Regression (SILENT miscompile): when a block-scoped name shadows a GLOBAL, the
// leaked-name bug did not error -- the over-hoisted leaf closure silently bound to
// the global instead of the intended outer local. With block scope respected, the
// reference correctly binds to the outer local 'gn'.
// See doc/closure_hoisting_block_scope_bug.md (case 4).

function gn() { return "GLOBAL" }

function makeOuter() {
  let gn = "OUTER-LOCAL"
  return function middle() {
    return function leaf() {
      { let gn = "shadow" }
      return gn
    }
  }
}

println(makeOuter()()())
