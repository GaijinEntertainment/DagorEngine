options gen2

require UnitTest
require unit_probe/hello

[export]
def main() {
    var foo = makeDummy()
    testFoo(foo)
    print("probe {probe_hello_value()} {foo.fooData}\n")
}
