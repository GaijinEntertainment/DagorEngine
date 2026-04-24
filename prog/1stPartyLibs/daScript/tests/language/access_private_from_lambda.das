options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

class PrivLambdaA {
    private onLog : lambda
    private y : int
    def PrivLambdaA {
        pass
    }
    def install {
        onLog <- @ {
            y--
        }
    }
    def bang {
        if (onLog != null) {
            onLog |> invoke()
        }
    }
    def operator delete {
        y++
    }
}

[test]
def test_access_private_from_lambda(t : T?) {
    t |> run("class lambda accesses private field") @(t : T?) {
        var a = new PrivLambdaA()
        a->install()
        a->bang()
        t |> success(true)
    }
}
