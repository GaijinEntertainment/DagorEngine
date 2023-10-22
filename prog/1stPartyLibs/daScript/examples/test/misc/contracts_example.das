
require daslib/contracts
require daslib/safe_addr

require UnitTest

[expect_any_vector(blah)]                           // dasvector`.... or similar C++ types
def print_vec ( blah )
    for i in range(length(blah))
        print("{blah[i]}\n")

[expect_any_vector(blah)]
def print_vec_length ( blah : auto(DummyName) )     // note, dummy name here so that we can differentiate from another print_vec_length
    print("length(blah) is {length(blah)}\n")

def print_vec_length ( blah )
    print("blah is not a vector\n")

[expect_any_array(blah)]                            // array<foo>, [], or dasvector`.... or similar
def print_arr ( blah )
    for i in range(length(blah))
        print("{blah[i]}\n")

[!expect_ref(arg)]
def test_accept_ref(var arg : int)
    print("{typeinfo(typename arg)}\n")

[expect_ref(arg)]
def test_accept_ref(var arg : int&)
    print("{typeinfo(typename arg)}\n")

[expect_pointer(arg)]
def test_accept_pointer(var arg : int?)
    print("{typeinfo(typename arg)}\n")

[export]
def main
    testPoint3Array() <| $(arr)
        print_vec(arr)
        print_vec_length(arr)
        print_arr(arr)
    // print_vec(13)
    // 30304: no matching functions or generics print_vec ( int const )
    //  candidate function:
    //        print_vec ( blah : auto const ) : auto at hello_world.das:6:4
    //                argument blah is expected to be a vector template
    var i : int[3]          // this one ok, because i is 'any_array'
    print_arr(i)
    print_vec_length(13)    // this one ok, because there is print_vec_length unspecialized

    var a: int
    test_accept_ref(a)
    test_accept_ref(1)

    var a_ptr :int? = safe_addr(a)
    test_accept_pointer(a_ptr)
    // test_accept_pointer(1)
    // 30304: no matching functions or generics test_accept_pointer ( int const )
    // candidate function:
    //    test_accept_pointer ( arg : int? -const ) : void at generics.das:13:4
    //            invalid argument arg (0). expecting int? -const, passing int const
    //            pointer types can only add constness
    //            argument arg is not a pointer, it is int const

    return true
