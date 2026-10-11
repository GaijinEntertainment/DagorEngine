// helper module for private_property_setter / failed_private_property_setter
// a class whose property setter and a second property getter are private
options gen2
module _private_property_helper public

class Gate {
    private storage : bool
    private hidden_storage : int = 7
    def const operator . flag() : bool { return storage }
    def private operator . flag :=(value : bool) { storage = value }
    def private const operator . hidden() : int { return hidden_storage }
    def read_hidden() : int { return self.hidden }
}

def set_flag_from_helper(var g : Gate?; value : bool) {
    g.flag = value
}

class private Hidden {
    def const operator . flag() : bool { return true }
}

def make_hidden() : Hidden? {
    return new Hidden()
}

def read_hidden_flag(h : Hidden?) : bool {
    return h.flag
}
