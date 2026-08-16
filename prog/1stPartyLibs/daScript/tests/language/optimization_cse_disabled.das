options gen2
options rtti
options disable_cse

require daslib/ast
require daslib/rtti
require daslib/ast_boost
require dastest/testing_boost public
require strings
require daslib/strings_boost

// `options disable_cse` escape hatch: the pass must not fire in this module

[export]
def target_disabled(x, y : int) : int {
    let a = x * y
    let b = x * y
    return a + b
}

def has(s, sub : string) : bool {
    return find(s, sub, 0) >= 0
}

def count_of(s, sub : string) : int {
    return length(split(s, sub)) - 1
}

[test]
def test_cse_disabled(t : T?) {
    ast_gc_guard() {
        var body = ""
        compiling_module() |> for_each_function("target_disabled") $(func) {
            if (empty(body)) {
                body = "{describe(func.body)}"
            }
        }
        t |> success(!empty(body), "target function found")
        let clean <- [for (ln in split(body, "\n")); ln; where !(ln |> has("coverage("))]
        let b = join(clean, "\n")
        t |> equal(count_of(b, "* y"), 2)
        t |> equal(target_disabled(3, 4), 24)
    }
}
