options gen2
module _operators_parent public

class FooOp {
    a : int = 42

    def operator . Prop : int {
        return a
    }
}
