options gen2

module _standalone_dep_gen public

require _standalone_gen_mod public

def public dep_use(x : int) : int {
    return twice(x) + 1
}
