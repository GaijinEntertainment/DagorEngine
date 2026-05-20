options gen2
require dastest/testing_boost

def noop_handler(msg : string) {
    pass
}

def do_something(handler : function<(msg : string) : void> = @@noop_handler) {
    invoke(handler, "hello")
}

[test]
def test_default_fn_ptr(t : T?) {
    t |> run("invoke with default function pointer") @(t : T?) {
        t |> success(true)
        do_something()
    }
    t |> run("invoke with explicit function pointer") @(t : T?) {
        t |> success(true)
        do_something(@@noop_handler)
    }
}
