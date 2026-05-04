options gen2
options persistent_heap = true
options gc

require dastest/testing_boost public


[test]
def test_tombstones(t : T?) {
    var tab : table<int; int>
    unsafe {
        tab[2] = 2          // note : i'm too lazy to change this into tab |> insert(2, 2)
        tab |> erase(2)
        tab[2] = 2
        tab[5] = 5
        tab |> erase(5)
        tab[7] = 7
        tab |> erase(7)
        tab[8] = 8
        tab[9] = 9
        tab |> erase(9)
        tab[10] = 10
        tab |> erase(10)
        tab[11] = 11
        tab |> erase(11)
        tab[12] = 12
        tab |> erase(12)
        tab[13] = 13
        tab |> erase(13)
        tab[14] = 14        // entire table is values or tombstones
        tab |> erase(14)
        tab[15] = 15
    }
    t |> equal(3, length(tab))
    for (key in keys(tab)) {
        if (!(key == 15 || key == 8 || key == 2)) {
            t |> failure("unexpected key {key}")
        }
    }
}



