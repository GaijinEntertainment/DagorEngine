options gen2
options no_aot

module _rst_stub_mod_b

def public rst_stub_beta(x : int) : int {
    //! First fixture function of the B module.
    return x + 3
}

def public rst_stub_beta_two(x : int) : int {
    //! Second fixture function of the B module.
    return x + 4
}
