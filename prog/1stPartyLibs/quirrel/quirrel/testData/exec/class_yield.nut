class A {
    constructor() {
        this.call_yield()
    }

    function call_yield() {
        this.do_yield.call(0)
    }

    function do_yield() {
        yield
    }
}

A()
println("OK")