options gen2
options no_aot

module _rst_stub_mod_a

def public rst_stub_alpha(x : int) : int {
    //! First fixture function of the A module.
    return x + 1
}

def public rst_stub_alpha_two(x : int) : int {
    //! Second fixture function of the A module.
    return x + 2
}
