// Untyped protos skip masks; scripts read defaults
from "debug" import get_function_info_table

function untyped(a, b = 2) { return a + b }
function untypedVararg(a, ...) { return vargv.len() + a }
function typed(a: int, b: string = "x") { return $"{a}{b}" }
function typedVararg(a, ...: int) { return vargv.len() + a }
let lambda = @(x) x
class C {
  function method(p) { return p }
}

foreach ([name, f] in [["untyped", untyped], ["untypedVararg", untypedVararg], ["typed", typed],
    ["typedVararg", typedVararg], ["lambda", lambda], ["method", C.method]]) {
  let infos = f.getfuncinfos()
  let t = get_function_info_table(f)
  println($"{name}: typecheck {", ".join(infos.typecheck.map(@(m) $"{m}"))}; varargs {infos.varargs_type_mask}; object {t.objectTypeMask}; args {", ".join(t.argTypeMask.map(@(m) $"{m}"))}; ellipsis {t.ellipsisArgTypeMask}")
}

println($"calls: {untyped(1)} {untyped("s", "t")} {untypedVararg(1, "a", null)} {typed(3)} {typedVararg(1, 2, 3)} {lambda(7)} {C().method(null)}")
try {
  typedVararg(1, 2, "x")
}
catch (e) {
  println($"error: {e}")
}
