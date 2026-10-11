// A global whose fixed-array dimension names the variable it declares. The dimension expression
// sits inside the type it dimensions, so a reference that takes the variable's own type takes a
// type holding that reference; with two dimensions each infer pass squares the last one's copy.
// The array clone in main is what keeps the passes coming.
options gen2
expect 30109:2

let zz : int[zz][zz]

[export]
def main {
    var cb : array<int>
    cb := cb
}
