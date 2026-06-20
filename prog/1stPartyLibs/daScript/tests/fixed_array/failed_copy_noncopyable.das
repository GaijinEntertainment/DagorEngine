// Stage 0 characterization (FIXED_ARRAY_REWORK.md): plain assignment of a fixed array
// with non-copyable elements (array<int>) is a compile error — use := (clone).
options gen2
expect 30950

[export]
def main {
    var a : array<int>[3]
    var b : array<int>[3]
    b = a
}
