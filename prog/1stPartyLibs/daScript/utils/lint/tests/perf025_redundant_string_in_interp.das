options gen2
// PERF025: string(...) inside string interpolation is redundant.
//
// Problem:
//   String interpolation already converts every element to a string via the
//   string builder's DebugDataWalker. Wrapping the element in string(...) just
//   allocates an intermediate string that the builder then copies — a wasted
//   heap allocation per interpolation.
//
// Bad pattern:
//   def f(iv : int) : string {
//       return "{string(iv)}"                    // PERF025
//   }
//
// Good pattern:
//   def f(iv : int) : string {
//       return "{iv}"
//   }
//
// Note for dastest readers: these use function PARAMETERS (not literals) so the
// string() calls survive constant folding. `"{string(42)}"` would be folded to a
// single ExprConstString during inference (visitStringBuilderElement ->
// evalAndFoldString), leaving the rule nothing to attach to — and a folded
// constant carries no runtime cost anyway. (Same approach as the PERF020 fixture.)
//
// Unsigned ints get the ':d' format-tag hint: they interpolate as hex by default
// ("{42u}" -> "0x2a"), so dropping string() changes the output unless ':d' is used.
//
// das_string is also covered (reusing the PERF007 detection idiom: tHandle whose
// annotation is "das_string"). It is not exercised here to keep the fixture free of
// a daslib/ast require — `"{string(fn.name)}"` was verified separately. See the rst.

expect 31208:8

require daslib/perf_lint
require strings

// --- Bad patterns (one PERF025 each) ---

def bad_int(iv : int) : string {
    return "{string(iv)}"                                   // PERF025 — drop
}

def bad_int64(iv : int64) : string {
    return "{string(iv)}"                                   // PERF025 — drop
}

def bad_float(fv : float) : string {
    return "{string(fv)}"                                   // PERF025 — drop
}

def bad_double(dv : double) : string {
    return "{string(dv)}"                                   // PERF025 — drop
}

def bad_string(sv : string) : string {
    return "{string(sv)}"                                   // PERF025 — drop (PERF020 deduped at same loc)
}

def bad_uint(uv : uint) : string {
    return "{string(uv)}"                                   // PERF025 — drop + ':d' hint (interp default is hex)
}

def bad_uint64(uv : uint64) : string {
    return "{string(uv)}"                                   // PERF025 — drop + ':d' hint
}

def bad_uint8(uv : uint8) : string {
    return "{string(uv)}"                                   // PERF025 — drop + ':d' hint
}

// --- Good patterns (no warnings) ---

def good_nested_arg(iv : int) : string {
    return "{length(string(iv))}"                           // string() is an arg to length(), not a direct element
}

def good_bytes_to_text(bytes : array<uint8>) : string {
    return "{string(bytes)}"                                // value-shape conversion (bytes -> text); skipped
}

def good_plain_interp(iv : int) : string {
    return "{iv}"                                           // no string() cast at all
}

def good_outside_interp(iv : int) : string {
    return string(iv)                                       // string() outside any interpolation
}
