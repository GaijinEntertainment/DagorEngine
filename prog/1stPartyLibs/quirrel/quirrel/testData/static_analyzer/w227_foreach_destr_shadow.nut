// w227 (ident-hides-ident) must fire for a destructured foreach binding that
// shadows an outer parameter / binding. Covers NameShadowingChecker routing
// through Visitor::visitDestructuringDecl so visitVarDecl runs for each name.
function outer(x) {
  function inner() {
    foreach ({x = 99} in [{x = 1}]) {
      println(x)
    }
  }
  inner()
}
outer(5)
