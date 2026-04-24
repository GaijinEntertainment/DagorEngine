function coroutine_test(a,b) {
    println($"{a} {b}")
    local ret = suspend("suspend 1")
    println($"the coroutine says {ret}")
    ret = suspend("suspend 2");
    println($"the coroutine says {ret}")
    ret = suspend("suspend 3");
    println($"the coroutine says {ret}")
    return "I'm done"
}

let coro = newthread(coroutine_test)

local susparam = coro.call("test","coroutine") //starts the coroutine

local i = 1
do {
    println($"suspend passed [{susparam}]")
    susparam = coro.wakeup("ciao "+i)
    ++i
}while(coro.getstatus()=="suspended")

println($"return passed [{susparam}]")
