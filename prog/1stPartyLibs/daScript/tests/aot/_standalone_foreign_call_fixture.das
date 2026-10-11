options gen2

require _standalone_dep_call

[export]
def call_foreign(x : int) : int {
    var probe = new DepProbe()
    var filled : array<int>
    dep_fill(filled, x)
    let pair = dep_pair(x)
    let result = dep_probe(x) + probe->method(x) + dep_hybrid(x) + dep_jit(x) + pair.a + pair.b + dep_text("{x}") + filled[0]
    unsafe {
        delete probe
    }
    return result
}
