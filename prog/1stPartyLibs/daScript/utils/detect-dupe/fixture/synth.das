options gen2


def add_int(a, b : int) : int {
    return a + b
}


def add_int_renamed_args(x, y : int) : int {
    return x + y
}


def add_float(a, b : float) : float {
    return a + b
}


def double_int(a : int) : int {
    return a * 2
}


def loop_sum(arr : array<int>) : int {
    var s = 0
    for (v in arr) {
        s += v
    }
    return s
}


def loop_product(arr : array<int>) : int {
    var p = 1
    for (v in arr) {
        p *= v
    }
    return p
}


def with_lambda(var arr : array<int>) {
    arr |> sort() <| $(a, b : int) => a < b
}


def with_lambda_renamed(var arr : array<int>) {
    arr |> sort() <| $(x, y : int) => x < y
}


[export]
def main() {
    print("dummy main\n")
}
