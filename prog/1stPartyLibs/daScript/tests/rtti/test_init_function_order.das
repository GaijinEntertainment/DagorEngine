options gen2

require daslib/rtti
require dastest/testing_boost public

var g_touch = 0

[init]
def init_alpha {
    g_touch ++
}

[init]
def init_beta {
    g_touch ++
}

[init(late = true)]
def init_gamma_late {
    g_touch ++
}

[test]
def test_init_function_order(t : T?) {
    var order : array<uint64>
    this_context() |> for_each_init_function($(mnh : uint64) {
        order |> push(mnh)
    })
    let alpha_pos = find_index(order, get_function_mangled_name_hash(@@init_alpha))
    let beta_pos = find_index(order, get_function_mangled_name_hash(@@init_beta))
    let gamma_pos = find_index(order, get_function_mangled_name_hash(@@init_gamma_late))
    t |> success(alpha_pos >= 0 && beta_pos >= 0 && gamma_pos >= 0, "all three [init] functions are yielded")
    t |> success(alpha_pos < beta_pos, "plain [init] functions keep declaration order")
    t |> success(gamma_pos > beta_pos, "[init(late = true)] runs after the plain [init] functions")
}
