require debugapi

class MyDebugAgent : DapiDebugAgent
    def add_ii ( var res:int&; a,b:int )
        print("add method {a} {b}\n")
        res = a + b
    def add ( a,b : int ) : int
        return a + b

def debug_agent ( ctx : Context )
    install_new_debug_agent(new MyDebugAgent(), "sample")

[export]
def add ( var self:MyDebugAgent; var res:int&; a,b : int )
    print("add function {a} {b}\n")
    res = self->add(a,b)

[export]
def main
    fork_debug_agent_context(@@debug_agent)
    var t_m, t_f : int
    unsafe
        invoke_debug_agent_method("sample", "add_ii", addr(t_m), 1, 2)
        invoke_debug_agent_function("sample", "add", addr(t_f), 3, 4)
    print("t_m = {t_m}\n")
    print("t_f = {t_f}\n")

