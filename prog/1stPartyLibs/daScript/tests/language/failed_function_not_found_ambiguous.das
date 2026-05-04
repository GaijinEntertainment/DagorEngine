// ambiguous function call
// verifies compiler rejects an unqualified call when two modules
// define functions with the same name and signature;
// qualified calls (::testFoo, _helper_foo::testFoo) must work
options gen2
expect 30304    // function_not_found

require _helper_foo

def testFoo(var a : TestObjectFoo) {
    a.fooData = 13
}

def callFoo {
    var foo : TestObjectFoo
    testFoo(foo)            //  two candidates ::testFoo and _helper_foo::testFoo
    ::testFoo(foo)
    _helper_foo::testFoo(foo)
}
