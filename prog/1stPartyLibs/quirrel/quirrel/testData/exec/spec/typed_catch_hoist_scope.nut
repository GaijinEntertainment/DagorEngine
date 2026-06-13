// Regression: a catch exception variable is block-scoped to its clause body. The
// closure-hoisting optimizer (-optCH, always on in tests) must not let that name leak
// into sibling clauses or post-try code, where the same name can bind to an outer
// local. Leaking it made capture analysis treat that outer binding as function-local
// and hoist the closure past the scope that owns it (-> "Unknown variable"). See
// closureHoisting.cpp visitTryStatement.

// (1) single catch: 'cap' after the catch binds to the outer local, so the leaf
// closure captures it and must hoist no higher than makeSingle's body.
function makeSingle() {
  let cap = "outer-single"
  return function mid() {
    return function leaf() {
      try { throw "x" }
      catch (cap) { }
      return cap
    }
  }
}

// (2) typed sibling clauses: 'cap' in the later clause binds to the outer local, not
// to the earlier typed clause's exception variable.
class MyErr {}
function makeSibling() {
  let cap = "outer-sibling"
  return function mid() {
    return function leaf() {
      try { throw "plain" }
      catch (MyErr cap) { return "wrong" }
      catch (other) { return cap }
    }
  }
}

println(makeSingle()()())
println(makeSibling()()())
