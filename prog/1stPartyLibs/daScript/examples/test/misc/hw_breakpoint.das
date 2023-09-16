options debugger
require daslib/debug

require rtti
require debugapi

[export]
def main
    var i = 5
    var bp = 0
    unsafe  // set breakpoint when data of 4 bytes is modified
        bp = set_hw_breakpoint(this_context(), addr(i), 4, true)
    print("before i = {i}\n")
    i = 6                           // this would trigger breakpoint
    print("after i = {i}\n")        // debugger will stop right here
    i = 8                           // this would trigger breakpoint
    print("after i = {i}\n")        // debugger will stop right here again
    print("and we are done\n")
    unsafe  // clear breakpoint
        clear_hw_breakpoint(bp)
    print("after clear")
    i = 7                           // this wont trigger breakpoint
    print("finished\n")
    print("!\n")

