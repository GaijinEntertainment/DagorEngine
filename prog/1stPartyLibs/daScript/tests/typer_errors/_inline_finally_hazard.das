// Fixture for test_inline_errors.das — intentionally fails to compile: the return
// under the loop needs the flag tier, and the flag tier would truncate/wrap the
// tail of the finally-carrying block, hiding declarations from the finally.
// `_`-prefix keeps dastest from auto-running it; `expect` makes lint skip it.
expect 50501

options gen2

[inline]
def bad(src : array<int>) : int {
    var total = 0
    {
        for (v in src) {
            if (v < 0) {
                return -1
            }
        }
        total += 1
    } finally {
        total += 100
    }
    return total
}

[export]
def main() {
    let y = bad([1, 2, 3])
    feint("{y}\n")
}
