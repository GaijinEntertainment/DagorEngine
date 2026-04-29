options gen2

// Tutorial 10 — Capture macro
//
// Demonstrates AstCaptureMacro (registered via [capture_macro]).
// A capture macro hooks into lambda creation at three points:
//   captureExpression — called per captured variable (can wrap/replace the value)
//   captureFunction  — called once per lambda (can add per-invocation cleanup)
//   releaseFunction  — called once per lambda finalizer (can add destruction cleanup)
//
// The companion module (capture_macro_mod) defines:
//   [audited]          — tag annotation for structs that should be logged
//   CaptureAuditMacro  — capture macro that logs capture, per-call, and release
//
// TIMING of each hook:
//   captureExpression  — fires at lambda CREATION (when the struct is built)
//   captureFunction    — adds code to the lambda function's body finalList,
//                        which runs after EACH INVOCATION (per-call finally).
//                        This is different from the user-written finally{} on the
//                        lambda literal, which runs on DESTRUCTION (once).
//   releaseFunction    — adds code to the lambda FINALIZER function's body,
//                        which runs on DESTRUCTION (after user-written finally{}
//                        but before field cleanup/delete).
//
// Non-[audited] types are silently ignored by the macro.

require capture_macro_mod

// ---- Struct definitions -----------------------------------------------------

[audited]
struct Resource {
    name : string
    id : int
}

struct Plain {
    x : int
}

// ---- Section 1: basic capture auditing --------------------------------------

def section1() {
    print("--- Section 1: basic capture auditing ---\n")
    // Only the [audited] Resource triggers logging; Plain is silent.
    // Variables are captured implicitly (by copy) when referenced in the lambda.
    var res = Resource(name = "texture.png", id = 1)
    var pl = Plain(x = 42)
    var fn <- @() {
        print("  body: res.name={res.name}, pl.x={pl.x}\n")
    }
    fn()
    // captureExpression prints at creation (above).
    // captureFunction's after-call message runs after fn() returns.
    // releaseFunction's releasing message runs when fn is destroyed (below).
    print("  about to delete fn...\n")
    unsafe { delete fn; }
    print("")
}

// ---- Section 2: multiple [audited] fields -----------------------------------

def section2() {
    print("\n--- Section 2: multiple [audited] fields ---\n")
    // Each [audited] field produces its own capture and per-call messages.
    var a = Resource(name = "mesh.obj", id = 2)
    var b = Resource(name = "shader.hlsl", id = 3)
    var fn <- @() {
        print("  body: a.id={a.id}, b.id={b.id}\n")
    }
    fn()
    fn()
    // The after-call message appears twice — once per invocation.
    // The releasing message appears once — on destruction.
    print("  about to delete fn...\n")
    unsafe { delete fn; }
    print("")
}

// ---- Section 3: non-audited types are silent --------------------------------

def section3() {
    print("\n--- Section 3: non-audited types are silent ---\n")
    var x = 10
    var y = 20
    var fn <- @() {
        print("  body: x + y = {x + y}\n")
    }
    fn()
    print("  about to delete fn...\n")
    unsafe { delete fn; }
    print("  (no audit messages for plain int captures)\n")
}

[export]
def main() {
    section1()
    section2()
    section3()
}

// output:
// --- Section 1: basic capture auditing ---
// [audit] captured 'res'
//   body: res.name=texture.png, pl.x=42
// [audit] after-call: 'res' still captured
//   about to delete fn...
// [audit] releasing 'res'
//
// --- Section 2: multiple [audited] fields ---
// [audit] captured 'a'
// [audit] captured 'b'
//   body: a.id=2, b.id=3
// [audit] after-call: 'a' still captured
// [audit] after-call: 'b' still captured
//   body: a.id=2, b.id=3
// [audit] after-call: 'a' still captured
// [audit] after-call: 'b' still captured
//   about to delete fn...
// [audit] releasing 'a'
// [audit] releasing 'b'
//
// --- Section 3: non-audited types are silent ---
//   body: x + y = 30
//   about to delete fn...
//   (no audit messages for plain int captures)
