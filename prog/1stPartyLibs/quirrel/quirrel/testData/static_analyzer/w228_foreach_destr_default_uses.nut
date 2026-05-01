// Identifier referenced inside a destructuring default-value expression must
// be marked as used by the analyzer. Without the fix that routes
// visitForeachStatement through valDestructuring->visit, the analyzer never
// walks the default expression and would emit a spurious w228
// (declared-never-used) for `fallback_value`.
function f() {
  let fallback_value = 42
  foreach ({n = fallback_value} in [{}, {n = 1}]) {
    println(n)
  }
}
f()
