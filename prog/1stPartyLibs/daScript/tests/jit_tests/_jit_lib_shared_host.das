options gen2

require daslib/safe_addr
require _standalone_init_fixture_c

[export]
def main() : int {
    var ctx = standalone_init_fixture_create()
    if (ctx == null) {
        print("create failed: {standalone_init_fixture_last_error(null)}\n")
        return 1
    }
    var bad = 0
    let first = standalone_init_fixture_get_first(ctx)
    let second = standalone_init_fixture_get_second(ctx)
    let stamp = standalone_init_fixture_get_init_fn_stamp(ctx)
    let shared_total = standalone_init_fixture_get_shared_total(ctx)
    if (first != 31 || second != 2 || stamp != 3 || shared_total != 6) {
        print("globals answered first={first} second={second} stamp={stamp} shared={shared_total}\n")
        bad++
    }
    var p = standalone_init_fixture_Pair(a = 40, b = 2)
    let inner = standalone_init_fixture_Inner(weight = 9)
    var o = standalone_init_fixture_Outer(inner = inner, tag = 5)
    let pair_sum = standalone_init_fixture_pair_sum(ctx, safe_addr(p))
    let outer = standalone_init_fixture_outer_weight(ctx, safe_addr(o))
    let head = standalone_init_fixture_head_value(ctx)
    let renamed = standalone_init_fixture_renamed_sum(ctx, 20, 22)
    if (pair_sum != 42 || outer != 14 || head != 41 || renamed != 42) {
        print("calls answered pair_sum={pair_sum} outer={outer} head={head} renamed={renamed}\n")
        bad++
    }
    standalone_init_fixture_destroy(ctx)
    return bad
}
