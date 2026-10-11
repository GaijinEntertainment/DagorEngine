// A defer at the top level of a lambda (or generator) body would land in the body block's
// finally section, which the lambda lowering turns into the finalizer - the cleanup would
// then run once on delete instead of once per call. The macro rejects the shape outright.
options gen2
expect 50503:2

require daslib/defer

var g_count = 0

def defer_in_lambda() {
    var lam <- @() {
        defer() {
            g_count++
        }
        g_count += 10
    }
    invoke(lam)
}

def defer_in_generator() {
    var gen <- generator<int>() <| $() {
        defer() {
            g_count++
        }
        yield 1
        return false
    }
    for (x in gen) {
        g_count += x
    }
}

[export]
def main() {
    defer_in_lambda()
    defer_in_generator()
}
