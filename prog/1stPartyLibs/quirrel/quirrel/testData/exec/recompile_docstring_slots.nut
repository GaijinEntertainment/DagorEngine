// A soft reload compiles each module again. Its script docstrings must reuse
// the registry slots of the first compile.

let { getbuildinfo, doc } = require("debug")
let slots = @() getbuildinfo().docstring_registry_slots

let src = "function f() {\n  @@\"doc of f\"\n}\nclass C {\n  @@\"doc of C\"\n}\nreturn [f, C]"
let first = compilestring(src)()
let before = slots()
for (local i = 0; i < 10; i++)
  compilestring(src)()
println(slots() - before)

let again = compilestring(src)()
println(again[0] != first[0])
println(doc(again[0]))
println(doc(again[1]))
