class Foo {
    function DoSomething() {
        println("I'm the base")
    }
    function DoIt() {
        this.DoSomething()
    }
}

class SuperFoo(Foo) {
    //overridden method
    function DoSomething() {
        println("I'm the derived")
    }
    function DoIt() {
        base.DoIt()
    }
}

//creates a new instance of SuperFoo
let inst = SuperFoo()

//prints "I'm the derived"
inst.DoIt()
