// helper module providing TestObjectFoo struct and testFoo function
// used by const_ref, function_not_found_ambiguous, and structure_not_found_ambiguous tests
options gen2
module _helper_foo

struct TestObjectFoo {
    fooData : int
}

def testFoo(var foo : TestObjectFoo) {
    foo.fooData = 1234
}
