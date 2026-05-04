options gen2

// Shader-like mode demo — rejects heap-allocating constructs at simulation time.
//
// `options shader_like` (from daslib/validate_code) enforces a restricted subset
// of daScript: no global arrays (heap), no lambdas (heap), no `new` (heap).
// Errors are reported via simulate_macro, so they appear at simulation time,
// not compilation time. Run standalone to see the expected errors:
//
//   daslang.exe shader_like_validation.das
//
// Expected output: 6x error 40104 (global requires heap, lambda requires heap,
// can't ascend, new requires heap, can't new).

require daslib/validate_code

options shader_like

// global array requires heap → error 40104
var arr : array<int>

def add(a, b) {
    return a + b
}

struct Foo {
    a : int = 13
}

[export]
def main() {
    var i = add(1, 2)
    print("i = {i}\n")
    print("arr = {arr}\n")

    // lambda requires heap → 2x error 40104
    var lmb <- @ {
        print("hello\n")
    }
    invoke(lmb)

    // new requires heap → 2x error 40104
    var f = new Foo()
}
