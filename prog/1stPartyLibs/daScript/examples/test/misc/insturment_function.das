options debugger = true

require rtti
require debugapi

class SampleDebugAgent : DapiDebugAgent
    def override onInstrumentFunction ( var ctx:Context; fun:SimFunction?; entering:bool; userData:uint64 ) : void
        print("{entering} {fun.mangledName} - {userData}\n")

[export]
def debug_agent ( ctx : Context )
    assert(this_context().category.debug_context)
    install_new_debug_agent(new SampleDebugAgent(), "debug")

// walker

struct Bar
    i = 1
    b = true

struct Foo
    a: int
    arr = [[auto[] true; false]]
    arr2 = [[Bar[] [[Bar]]; [[Bar]]]]
    ref: Foo?

def operator delete (var a:Foo&)
    print("destroy Foo[{a.a}]\n")

[export]
def main()
    this_context().name := "stackwalk main context"
    var count = 0
    instrument_all_functions(this_context()) <| $ ( fn, sfn )
        let info = get_function_info(this_context(),fn)
        print("instrumenting {sfn.mangledName}\n")
        return uint64(count++)
    fork_debug_agent_context(@@debug_agent)

    var a = [[Foo a=0]]
    unsafe
        a.ref = addr(a)
    delete a
    // breakpoint()

    var arr <- [{ Foo[] [[Foo a=1]]; [[Foo a=2]] }]
    print("arr len = {length(arr)}\n")

    let dim = [[auto[] [[Foo a=100]]; [[Foo a=200]]  ]]
    print("{dim}\n")

    breakpoint()

    var j : int[3]
    for q in j
        q++

    for i in range(length(arr))
        arr[i].a = i + 2

    delete arr // <-- падает тут если пойти step-in
    var arr2 <- [{ Foo[] [[Foo a=3]] }]
    arr <- arr2
    // while true
    //     pass
    delete arr
