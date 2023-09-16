require daslib/rtti
require fio

require daslib/profiler_boost

[sideeffects]
def fibR(n:int)
    if (n < 2)
        return n
    return fibR(n - 1) + fibR(n - 2)

[sideeffects]
def wait_1_msec
    let t0 = ref_time_ticks()
    while get_time_usec(t0) < 1000
        pass

struct Foo
    i : int

[sideeffects]
def use_heap
    for i in range(10)
        var f = new [[Foo i=i]]
        print("f = {f}\n")

[export]
def main()
    print("hello world: this context is {intptr(unsafe(addr(this_context())))} {this_context().name} // {this_context().category}\n")
    let t0 = ref_time_ticks()
    let f = fibR(13)
    wait_1_msec()
    let dt = get_time_usec(t0)
    print("hello, world {dt}usec, {double(dt)/1000000.0lf}sec {f}\n")
    use_heap()


