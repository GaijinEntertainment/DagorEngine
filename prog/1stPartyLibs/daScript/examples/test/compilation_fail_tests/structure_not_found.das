// options logInferTypePasses=true

require UnitTest

expect 30301    // type_not_found

struct TestObjectFoo
    a:int

def test
    new TestObjectFoo               // 30301: undefined type, 2 candidates UnitTest::TestObjectFoo and ::TestObjectFoo
    new UnitTest::TestObjectFoo     // this one ok
    new ::TestObjectFoo             // this one ok too



