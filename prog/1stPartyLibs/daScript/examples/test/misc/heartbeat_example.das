require daslib/heartbeat


def foo
    print("hello\n")
    for i in range(3)
        print("step {i}\n")
    var q = 1
    while q < 3
        print("q = {q}\n")
        q ++
    while true
        print(".\n")

[export]
def main
    var count = 0
    set_heartbeat <| @
        count ++
        if count == 1000
            breakpoint()
    foo()

options log
