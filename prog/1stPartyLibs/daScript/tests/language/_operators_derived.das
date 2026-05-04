options gen2
module _operators_derived public

require _operators_parent public    // public is very important here, to make sure stuff in _operators_parent is visible here

class BarOp : FooOp {
    def BarOp() {
        pass
    }
}
