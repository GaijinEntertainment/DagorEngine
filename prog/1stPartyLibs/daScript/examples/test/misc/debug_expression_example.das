require daslib/ast_boost
require daslib/templates_boost

[export]
def main
    var q <- qmacro_block <|
        let x = 10
        print("x = {x+1}")
        if x != 10
            assert(x==10)
        else
            pass
        return @ <| ( a,b:int )
            return a+b
    print(debug_expression(q))


