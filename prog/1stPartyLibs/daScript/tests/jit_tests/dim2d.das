options gen2
require dastest/testing_boost

let ROOM_SIZE_X = 32
let ROOM_SIZE_Y = 18
let ROOM_SIZE_Z = 5

var arr_xyz : int[ROOM_SIZE_X][ROOM_SIZE_Y][ROOM_SIZE_Z]
var arr_yzx : int[ROOM_SIZE_Y][ROOM_SIZE_Z][ROOM_SIZE_X]
var arr_xzy : int[ROOM_SIZE_X][ROOM_SIZE_Z][ROOM_SIZE_Y]

def walk {
    var pos = 0
    var arr : int[ROOM_SIZE_X + 2][ROOM_SIZE_Y + 2]
    for (y in 0 .. ROOM_SIZE_Y) {
        for (x in 0 .. ROOM_SIZE_X) {
            arr[x + 1][(ROOM_SIZE_Y - 1 - y) + 1] = pos++
        }
    }
    return pos
}

[jit, sideeffects]
def walk_xyz {
    for (x in range(ROOM_SIZE_X)) {
        for (y in range(ROOM_SIZE_Y)) {
            for (z in range(ROOM_SIZE_Z)) {
                arr_xyz[x][y][z] = 1
                arr_yzx[y][z][x] = 1
                arr_xzy[x][z][y] = 1
            }
        }
    }
    return true
}

var x : int[4][5]

[no_jit]
def init_no_jit {
    var val = 0
    for (i in range(4)) {
        for (j in range(5)) {
            x[i][j] = val++
        }
    }
}

def check_order_jit : bool {
    var val = 0
    for (i in range(4)) {
        for (j in range(5)) {
            return false if (x[i][j] != val++)
        }
    }
    return true
}

[test, no_jit]
def test_dim(t : T?) {
    t |> run("multidim access") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@ < () : bool > check_order_jit))
        init_no_jit()
        t |> success(check_order_jit())
        t |> equal(walk(),     576)
        t |> equal(walk_xyz(), true)
    }
}
