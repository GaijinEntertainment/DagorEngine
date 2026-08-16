// One C function bound to two slots must keep a decl string and a docstring per
// slot, not per function pointer. The entries live in doc_objects keyed by the
// closure, so they must also follow the closure through bindenv and die with it

from "test.native" import aliased_first, aliased_second, doc_entry_count, raw_cmp
let { doc, get_function_decl_string, get_function_info_table, collectgarbage } = require("debug")

println(get_function_decl_string(aliased_first))
println(get_function_decl_string(aliased_second))

println(doc(aliased_first))
println(doc(aliased_second))

println(get_function_info_table(aliased_first).functionName)
println(get_function_info_table(aliased_second).functionName)
println(get_function_info_table(aliased_first).requiredArgs)
println(get_function_info_table(aliased_second).requiredArgs)

let params1 = aliased_first.getfuncinfos().parameters
let params2 = aliased_second.getfuncinfos().parameters
println($"{params1.len()} {params2.len()} {params2[1]}")
println(aliased_first.getfuncinfos().doc)

// A docstring set without a decl string is keyed the same way
println(doc(doc_entry_count))

// A bound copy keeps both entries of the closure it was made from. Without them
// the decl string falls back to what the closure fields can rebuild (no return
// type) and the docstring is lost entirely
let bound = aliased_first.bindenv({})
println(get_function_decl_string(bound))
println(doc(bound))

// A native with no entries at all stays that way through bindenv
println(doc(raw_cmp.bindenv({})))

// Every copy owns its own pair of entries and must give them up when it dies.
// The copies are held until the round ends so that each one gets its own
// address instead of reusing the block freed by the previous one
function makeAndDropCopies() {
  let copies = []
  for (local i = 0; i < 100; i++)
    copies.append(aliased_first.bindenv({}))
  let live = doc_entry_count()
  copies.clear()
  collectgarbage()
  return [live, doc_entry_count()]
}

let before = doc_entry_count()
let round1 = makeAndDropCopies()
let round2 = makeAndDropCopies()
println(round1[0] - before)
println(round2[1] - round1[1])

println("PASSED")
