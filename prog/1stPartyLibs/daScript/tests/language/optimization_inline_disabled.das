options gen2
options rtti
options disable_inline

require daslib/ast
require daslib/rtti
require daslib/ast_boost
require dastest/testing_boost public
require strings
require daslib/strings_boost

// `options disable_inline` escape hatch: calls to [inline] functions stay regular
// calls in this module, while the [inline] contract checks (shape, @@, recursion)
// still lint

[inline]
def add(a, b : int) : int {
    return a + b;
}

[export]
def target_disabled(x, y : int) : int {
    return add(x, y + 1);
}

def has(s, sub : string) : bool {
    return find(s, sub, 0) >= 0;
}

def count_of(s, sub : string) : int {
    return length(split(s, sub)) - 1;
}

[test]
def test_inline_disabled(t : T?) {
    ast_gc_guard() {
        var body = "";
        compiling_module() |> for_each_function("target_disabled") $(func) {
            if (empty(body)) {
                body = "{describe(func.body)}";
            }
        }
        t |> success(!empty(body), "target function found");
        let clean <- [for (ln in split(body, "\n")); ln; where !(ln |> has("coverage("))];
        let b = join(clean, "\n");
        t |> equal(count_of(b, "add("), 1);
        t |> equal(target_disabled(5, 6), 12);
    }
}
