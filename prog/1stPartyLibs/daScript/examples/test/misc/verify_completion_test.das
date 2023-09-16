require daslib/validate_code


[export,verify_completion]
def bad_loop ( all : iterator<int>; cond_const:bool )
    for t in all                        // 40104: we don't know if this loop ever finishes. add explicit range
        print("t = {t}\n")
    for t,r in all,range(100500)        // this one ok
        print("t = {t}\n")
    var cond = true
    while cond                          // 40104: we can't tell if this `while` loop ever finishes
        print("nop\n")
        cond = false
    var tab <- {{1=>1; 2=>2; 3=>3}}
    for x in tab |> keys()              // this one ok - we know keys and values are finite
        print("x = {x}\n")

[export,verify_completion]
def simple_recursive
    print("simple\n")
    simple_recursive()  // 40104: recursive calls are not allowed

def rec_a
    print("a\n")
    rec_b() // 40104: recursive calls are not allowed -> rec_b -> rec_a -> rec_b

def rec_b
    print("b\n")
    rec_a() // 40104: recursive calls are not allowed -> rec_a -> rec_b -> rec_a

def rec_c
    rec_a()

[export,verify_completion]
def rec_d
    rec_c() // 40104: recursive calls are not allowed -> rec_c -> rec_a -> rec_b -> rec_a

[export]
def main
    pass
