// Typed default parameter safety for closure hoisting:
// Functions with typed defaults (param with both type annotation and default
// value) are never hoisted. The type check at closure creation can throw,
// and hoisting would move that throw to a different scope.

function outer() {
  function inner() {
    // NOT hoisted: has typed default (type + default value)
    let typedDefault = function(x: int = 42) { return x }

    // Hoisted: has default but no type annotation
    let untypedDefault = function(x = 42) { return x }

    // Hoisted: has type annotation but no default
    let typedNoDefault = function(x: int) { return x }

    // Hoisted: no type, no default
    let plain = function(x) { return x }

    return typedDefault() + untypedDefault() + typedNoDefault(1) + plain(1)
  }
  return inner()
}
