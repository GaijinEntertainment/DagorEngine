// Stage 0 characterization (FIXED_ARRAY_REWORK.md): `new` of a fixed-array type is
// a compile error — only tuples, variants, structures and handled types heap-allocate.
options gen2
expect 30214

typedef IntArr4 = int[4]

[export]
def main {
    var p = new IntArr4
}
