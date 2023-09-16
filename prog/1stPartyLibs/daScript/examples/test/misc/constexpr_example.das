options log

require daslib/constant_expression

[constexpr (a)]
def foo ( t:string; a : int )
    print("{t} = {a}\n")

var BOO = 13

[export]
def main
    foo("blah", 1)
    foo("ouch", BOO)
