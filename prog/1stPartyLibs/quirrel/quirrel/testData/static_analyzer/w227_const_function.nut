const function bound() { return 1 }
global const function boundGlobal() { return 2 }
const boundExpr = function boundExpr() { return 4 }

function outer(bound) {
  return bound + boundGlobal()
}

println(outer(3) + bound() + boundExpr())
