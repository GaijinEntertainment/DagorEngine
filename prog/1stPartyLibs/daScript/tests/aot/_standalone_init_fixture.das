options gen2

var g_stamp = 0

def next_stamp : int {
    g_stamp ++
    return g_stamp
}

var g_first = next_stamp()
var g_second = next_stamp()
var g_init_fn_stamp = 0

[init]
def record_init {
    g_init_fn_stamp = next_stamp()
}

[export]
def get_init_stamps : int3 {
    return int3(g_first, g_second, g_init_fn_stamp)
}
