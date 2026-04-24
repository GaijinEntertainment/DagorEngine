options gen2
expect 30508, 30124

class A {
    private onLog : lambda
    private y : int
    def A {
        onLog <- @ {            //  30508: can't move from a constant value, lambda<>& <- lambda<void> const
            y --                //  30124: can't capture variable self
            feint("onLog\n")
        }
    }
    def bang {
        if (onLog != null) {
            onLog |> invoke()
        }
    }
    def operator delete {
        y ++
    }
}


