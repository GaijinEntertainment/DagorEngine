// ambiguous struct name
// verifies compiler rejects unqualified `new` when two modules
// define structs with the same name;
// qualified names (_helper_foo::TestObjectFoo, ::TestObjectFoo) must work
options gen2

require _helper_foo

expect 30302    // too_many_options

struct TestObjectFoo {
    a : int
}

def test {
    new TestObjectFoo               // 30301: undefined type, 2 candidates _helper_foo::TestObjectFoo and ::TestObjectFoo
    new _helper_foo::TestObjectFoo  // this one ok
    new ::TestObjectFoo             // this one ok too
}
