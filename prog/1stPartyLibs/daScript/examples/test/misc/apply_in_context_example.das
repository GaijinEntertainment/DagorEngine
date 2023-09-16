require daslib/apply_in_context

require debugapi

def debug_agent ( ctx : Context )
    install_new_debug_agent(new DapiDebugAgent(), "sample")

var num = 13

[apply_in_context(sample)]
def get_num ( var n:int& implicit )
    debug(n)
    n = ++num

[apply_in_context(sample)]
def get_other_num
    return ++num

[export]
def main
    fork_debug_agent_context(@@debug_agent)
    var inum = 0
    get_num(inum)
    print("num = {num} inum = {inum} other_num = {get_other_num()}\n")

