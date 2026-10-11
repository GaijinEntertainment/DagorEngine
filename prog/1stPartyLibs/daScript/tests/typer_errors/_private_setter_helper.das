// helper module for the private-property-setter hint cases in test_operator_errors
options gen2
module _private_setter_helper public

class Gate {
    private storage : bool
    def const operator . flag() : bool { return storage }
    def private operator . flag :=(value : bool) { storage = value }
}
