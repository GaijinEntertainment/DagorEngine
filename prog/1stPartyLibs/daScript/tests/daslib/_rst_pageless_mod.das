options gen2
options no_aot

module _rst_pageless_mod public

typedef distinct RstPagelessId = int

def rst_pageless_pass(i : RstPagelessId) : RstPagelessId {
    //! Passes an id through.
    return i
}
