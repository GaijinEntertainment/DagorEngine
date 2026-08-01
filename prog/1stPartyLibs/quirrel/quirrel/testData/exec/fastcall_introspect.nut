// The `fastcall` attribute must be reported consistently by both introspection
// APIs: closure.getfuncinfos() and debug.get_function_info_table(), and appear
// in debug.get_function_decl_string() for a marked native.

from "math" import abs, hash
local dbg = require("debug")

// marked native: both APIs agree, decl string carries the modifier
println(abs.getfuncinfos().fastcall)
println(dbg.get_function_info_table(abs).fastcall)
println(dbg.get_function_decl_string(abs))

// unmarked native (hash is deliberately not fastcall)
println(hash.getfuncinfos().fastcall)
println(dbg.get_function_info_table(hash).fastcall)

// script closure is never fastcall
local f = @(x) x + 1
println(f.getfuncinfos().fastcall)

println("OK")
