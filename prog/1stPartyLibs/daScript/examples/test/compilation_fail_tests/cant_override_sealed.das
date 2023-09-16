expect 30115:2

class A
    sealed name : string = "foobar"
    def sealed set_name ( n:string )
        name = n

class B : A
    override name = "barbar"                //  30115: structure field name is sealed
    def override set_name ( n:string )      //  30115: structure field set_name is sealed
        name = "___{n}___"

[export]
def main
    let b = new B()
    print("b = {b.name}\n")
