let str = "abc" // not a constant, so the call is not folded
try { println(str.slice("1")) } catch (e) { println(e) }

let NotAClass = 42
try {
  try { throw "boom" }
  catch (NotAClass e) { println("typed catch matched") }
}
catch (e) { println(e) }
