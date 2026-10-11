options gen2

module _standalone_gen_mod public

// A generic the entry module and a required module both instantiate: one C++ body, two
// mangled-name hashes.
def public twice(a) {
    return a + a
}
