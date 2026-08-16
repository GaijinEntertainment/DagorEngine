options gen2

module _string_clone_init_helper public

require strings

def public same_string_storage(a, b : string) : bool {
    var same = false
    peek_data(a) $(aa) {
        peek_data(b) $(bb) {
            same = addr(aa[0]) == addr(bb[0])
        }
    }
    return same
}
