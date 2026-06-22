// Stage 0 characterization (FIXED_ARRAY_REWORK.md): dimension must be a compile-time
// constant; a runtime value is a compile error.
options gen2
expect 30109, 30838

[export]
def main {
    var n = 4
    var a : int[n]
    a[0] = 1
}
