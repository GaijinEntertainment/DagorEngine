let class Foo {
    //constructor

    testy = null
    constructor(a) {
        this.testy = ["stuff",1,2,3,a]
    }
    //member function
    function PrintTesty() {
        foreach(i,val in this.testy) {
            println("idx = "+i+" = "+val)
        }
    }
    //property
}

let foo = Foo(42)

foo.PrintTesty()
