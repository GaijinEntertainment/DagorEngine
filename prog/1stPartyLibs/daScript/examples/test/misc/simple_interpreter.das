require llvm/llvm_jit

class Node
    def eval ( i : int )
        pass

// TODO: early out of empty array causes lock underflow
//  need to go to appropriate source out, not just endloop

[jit]
def eval ( var a : array<Node?> )
    for n,i in a,count()
        (*n)->eval(i)

[export]
def main
    var A <- [{int 1;2;3;4;5;6;7;8;9}]
    for a,c in A,count(0,2)
        print("a = {a} c = {c}\n")

// options log_cpp
