require UnitTest

enum Numbers : int16
    zero
    one
    two
    ten = 10
    sixteen = 0x10
    neg_ten = -10
    neg_one = 0xffffffff
    neg_one_i = -1

enum Zen
    zero
    not_zero

struct Foo
    a : int
    b : float

enum QQ
    size_of_Foo_Padded = typeinfo(sizeof type<Foo>) + 8

enum BooHoo
    boo
    hoo
    boo_hoo = 5

enum BooHoo8 : int8
    boo = 1
    hoo = 2
    boo_hoo = 3

enum BooHoo16 : int16
    boo = 4
    hoo = 5
    boo_hoo = 6

[sideeffects]
def getZenOne
    return Zen not_zero

[export]
def test
    assert ( int(QQ size_of_Foo_Padded)==16 )
    assert ( int(Numbers zero)==0 )
    assert ( int(Numbers two)==2 )
    assert ( int(Numbers ten)==10 )
    assert ( int(Numbers sixteen)==16 )
    assert ( int(Numbers neg_ten)==-10 )
    assert ( int(Numbers neg_one)==-1 )
    assert ( int(Numbers neg_one_i)==-1 )
    let t : Numbers
    assert ( t == Numbers zero )
    var q = Numbers one
    assert ( int(q)==1 )
    assert ( q == Numbers one )
    assert ( t != q )
    q = Numbers zero
    assert ( t == q )
    assert ( t != Numbers one )
    assert ( Zen zero != Zen not_zero )
    assert ( Zen zero == Zen zero )
    assert ( int(t)==0 )
    assert ( int(Zen not_zero)==1 )
    verify ( getZenOne()==Zen not_zero )
    assert ( free_takeOne_giveTwo(SomeEnum one)==SomeEnum two )
    assert ( free_takeOne_giveTwo(SomeEnum two)==SomeEnum zero )
    verify ( efn_takeOne_giveTwo(SomeEnum one)==SomeEnum two )
    verify ( efn_takeOne_giveTwo_98(SomeEnum98 SomeEnum98_one)==SomeEnum98 SomeEnum98_two )

    var result_efn = efn_takeOne_giveTwo(SomeEnum one)
    assert(result_efn == SomeEnum two)

    var goo : GooEnum
    goo = GooEnum regular
    verify ( efn_flip(goo)==GooEnum hazardous )
    var foo16 : TestObjectFoo
    assert(foo16.e16==SomeEnum_16 SomeEnum_16_zero)
    foo16.e16 = SomeEnum_16 SomeEnum_16_one
    assert(foo16.e16==SomeEnum_16 SomeEnum_16_one)
    assert(int(foo16.e16)==1)
    assert(typeinfo(sizeof foo16.e16)==2)
    for e,tt in each_enum(BooHoo boo),[[int 0;1;5]]
        assert(int(e)==tt)
    for e,tt in each_enum(BooHoo8 boo),[[int 1;2;3]]
        assert(int(e)==tt)
    for e,tt in each_enum(BooHoo16 boo),[[int 4;5;6]]
        assert(int(e)==tt)
    return true
