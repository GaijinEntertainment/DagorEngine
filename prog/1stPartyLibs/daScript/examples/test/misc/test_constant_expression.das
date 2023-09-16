require daslib/constant_expression

[constant_expression(constString)]
def take_const_arg(boo:bool;constString:string)
    print("constant string is = {constString}\n")

[export]
def main
    take_const_arg(true,"hello world")
    take_const_arg(true,"goodbye world")

options log


