require scope_lint

[export]
def main
    var a : array<int>              // this one not fine, needs inscope
    var inscope b : table<int; int> // this one fine
    var c : int
    c ++
    print("hello world {a} {b} {c}\n")
