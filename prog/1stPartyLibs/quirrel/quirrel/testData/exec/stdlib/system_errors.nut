// Coverage: system module failure branches in sqstdsystem.cpp - getenv miss
// (returns null), remove()/rename() failure on a nonexistent path.

let { getenv, remove, rename } = require("system")

// getenv of an unset variable returns null (the not-found branch)
assert(getenv("QUIRREL_DEFINITELY_UNSET_VAR_98765") == null)
println("getenv miss: null")

function expectErr(label, fn) {
  try { fn(); println($"FAIL: {label} did not throw") }
  catch (e) { println($"err: {label}") }
}

// remove / rename of a path that does not exist -> "*() failed" branch
expectErr("remove-missing", function() { remove("__no_such_file_ABC_987__.tmp") })
expectErr("rename-missing", function() { rename("__no_such_file_ABC_987__.tmp", "__dst__.tmp") })

println("system errors: OK")
