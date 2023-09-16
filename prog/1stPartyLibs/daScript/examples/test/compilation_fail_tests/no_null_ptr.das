expect 30402:7

require UnitTest

def doNotPass ( a : TestObjectNotNullPtr?; b: int )
    print("{a} = {b}\n")

[export]
def doNotReturn : TestObjectNotNullPtr?
    return null                             // 30402: can't return null

[export]
def main
    var q : TestObjectNotNullPtr?           // 30402: local variable of type UnitTest::TestObjectNotNullPtr? -const needs to be initialized to avoid null pointer
    q = null                                // 30402: can't assign null pointer to UnitTest::TestObjectNotNullPtr?& -const
    var t : TestObjectNotNullPtr? = null    // 30402: local variable of type UnitTest::TestObjectNotNullPtr? -const can't be initialized with null
    t := null                               // 30402: can't assign null pointer to UnitTest::TestObjectNotNullPtr?& -const
    unsafe
        delete t                            // 30402: can't delete UnitTest::TestObjectNotNullPtr?& -const, it will create null pointer

    doNotPass(null,1)                       // 30402: can't pass null to function doNotPass argument a


    print("hello {t} {q}!\n")
