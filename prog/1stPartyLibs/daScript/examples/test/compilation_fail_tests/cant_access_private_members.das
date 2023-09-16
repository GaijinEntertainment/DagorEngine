expect 30503:3, 30301:1

class A
    private name : string
    def A ( n:string )
        name = n
    def private func()
        print("123")
    def bang
        func()

[export]
def main
    var a = new A()
    a.name = "boo"                  //  30503: can't access private field name of A? outside of member functions.
    let name = a?.name ?? "boo"     //  30503: can't access private field name of A? outside of member functions.
    a->func()                       //  30503: can't access private field func of A? outside of member functions.
                                    //  30301: casting to undefined type auto
    a->bang()
