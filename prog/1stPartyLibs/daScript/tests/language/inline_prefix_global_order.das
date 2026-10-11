// a global read in a call's evaluation prefix must keep left-to-right order when
// the call splices: the load is snapshotted above the spliced body, element and
// scalar reads alike - an element load that delays past the body's write is the
// miscompile this file pins
options gen2
require dastest/testing_boost public

var g_array : array<int>
var g_scalar = 10

[inline]
def clobber_and_one() : int {
    g_array[0] = 99
    g_scalar = 99
    var n = 0   // multi-statement body: the splice must land as statements, which is what forces the prefix hoist
    n++
    return n
}

[test]
def test_prefix_global_order(t : T?) {
    t |> run("global element read keeps left-to-right order") @(t : T?) {
        g_array |> clear()
        g_array |> push(10)
        g_scalar = 10
        let x = g_array[0] + clobber_and_one()
        t |> equal(x, 11)
    }
    t |> run("global scalar read keeps left-to-right order") @(t : T?) {
        g_array[0] = 10
        g_scalar = 10
        let y = g_scalar + clobber_and_one()
        t |> equal(y, 11)
    }
}
