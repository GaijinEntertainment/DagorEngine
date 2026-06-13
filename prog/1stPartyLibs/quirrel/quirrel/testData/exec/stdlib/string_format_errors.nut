// Coverage: format type-mismatch / malformed-spec error branches in
// validate_format (sqstdstring.cpp). The existing format test covers valid
// specifiers and null-argument errors but not these per-conversion checks.

let str = require("string")
let { format } = str

function expectErr(label, fn) {
  try { fn(); println($"FAIL: {label} did not throw") }
  catch (e) { println($"err: {label}") }
}

// missing argument for a conversion
expectErr("missing-arg",      @() format("%d"))
expectErr("missing-arg2",     @() format("%d %d", 1))

// type mismatches per conversion family
expectErr("s-needs-string",   @() format("%s", 123))
expectErr("d-needs-int",      @() format("%d", "x"))
expectErr("x-needs-int",      @() format("%x", "x"))
expectErr("c-needs-int",      @() format("%c", "x"))
expectErr("f-needs-number",   @() format("%f", "x"))
expectErr("g-needs-number",   @() format("%g", {}))

// malformed / oversized specifiers
expectErr("width-too-long",   @() format("%999999999999999999d", 1))
expectErr("prec-too-long",    @() format("%.999999999999999999f", 1.0))

// valid mixed format still works after all the errors above
println(format("%s=%d, %.1f, %x", "n", 7, 2.5, 255))   // n=7, 2.5, ff

println("format errors: OK")
