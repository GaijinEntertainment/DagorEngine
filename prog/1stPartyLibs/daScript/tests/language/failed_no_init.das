options gen2
expect 40214:3

options no_init

var a = 13

def foo {
    feint("hello, world!\n")
    return 1
}

var b = foo()   // 40214: [init] is disabled in the options or CodeOfPolicies

[init]          // 40214: [init] is disabled in the options or CodeOfPolicies
def bar {
    feint("hello, world\n")
}

[finalize]
def car {// 40214: [init] is disabled in the options or CodeOfPolicies
    feint("goodbye, world\n")
}

[export]
def main {
    feint("hello, world! {a}\n")
}


