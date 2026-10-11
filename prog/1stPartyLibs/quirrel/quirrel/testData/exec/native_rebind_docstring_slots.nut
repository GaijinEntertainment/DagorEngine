// Each compilestring binds the base lib to new native closures, as each require
// does. The new closures must reuse the doc and decl string slots of the first ones.

from "test.native" import aliased_first, set_object_docstring
let { getbuildinfo, get_function_decl_string, doc } = require("debug")
let slots = @() getbuildinfo().docstring_registry_slots

let before = slots()
for (local i = 0; i < 10; i++)
  compilestring("return 1")
println(slots() - before)

let rebound = compilestring("return print")()
println(rebound != print)
println(get_function_decl_string(rebound) == get_function_decl_string(print))

// A pair with both a doc and a decl string is shared too, but only while equal
let first = aliased_first.bindenv({})
let second = aliased_first.bindenv({})
set_object_docstring(first, "same doc")
let afterFirst = slots()
set_object_docstring(second, "same doc")
println(slots() - afterFirst)
set_object_docstring(second, "other doc")
println(doc(first))
println(doc(second))
println(doc(aliased_first))
println(get_function_decl_string(second))
