require daslib/coroutines

[coroutine]
def subcr(name:string)
    print("{name} sub 1\n")
    co_continue()
    print("{name} sub 2\n")
    co_continue()
    print("{name} sub 3\n")

[coroutine]
def coroutine_example(name:string;count:int)
    print("{name} step 1\n")
    co_continue()
    print("{name} step 2\n")
    co_continue()
    co_await <| subcr(name)
    co_continue()
    print("{name} step 3\n")
    for i in range(3,3+count)
        print("{name} step {i}\n")
        co_continue()

[export]
def main
    var crs <- [{auto
        coroutine_example("co1",1);
        coroutine_example("co2",2)
    }]
    cr_run_all(crs)


