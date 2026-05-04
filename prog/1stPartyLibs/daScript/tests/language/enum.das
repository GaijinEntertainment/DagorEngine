options gen2
options rtti

require UnitTest

require daslib/enum_trait
require dastest/testing_boost public

enum Numbers : int16 {
    zero
    one
    two
    ten = 10
    sixteen = 0x10
    neg_ten = -10
    neg_one = 0xffffffff
    neg_one_i = -1
}

enum Zen {
    zero
    not_zero
}

struct Foo {
    a : int
    b : float
}

enum QQ {
    size_of_Foo_Padded = typeinfo sizeof(type<Foo>) + 8
}

enum BooHoo {
    boo
    hoo
    boo_hoo = 5
}

enum BooHoo8 : int8 {
    boo = 1
    hoo = 2
    boo_hoo = 3
}

enum BooHoo16 : int16 {
    boo = 4
    hoo = 5
    boo_hoo = 6
}

[sideeffects]
def getZenOne {
    return Zen.not_zero
}

[export]
def testBindEnum(arg : SomeEnum) {
    feint("hello, world!\n")
}

[test]
def test_enum(t : T?) {
    t |> equal(int(QQ.size_of_Foo_Padded), 16)
    t |> equal(int(Numbers.zero), 0)
    t |> equal(int(Numbers.two), 2)
    t |> equal(int(Numbers.ten), 10)
    t |> equal(int(Numbers.sixteen), 16)
    t |> equal(int(Numbers.neg_ten), -10)
    t |> equal(int(Numbers.neg_one), -1)
    t |> equal(int(Numbers.neg_one_i), -1)
    let n : Numbers
    t |> equal(n, Numbers.zero)
    var q = Numbers.one
    t |> equal(int(q), 1)
    t |> equal(q, Numbers.one)
    t |> success(n != q)
    q = Numbers.zero
    t |> equal(n, q)
    t |> success(n != Numbers.one)
    t |> success(Zen.zero != Zen.not_zero)
    t |> equal(Zen.zero, Zen.zero)
    t |> equal(int(n), 0)
    t |> equal(int(Zen.not_zero), 1)
    t |> equal(getZenOne(), Zen.not_zero)
    t |> equal(free_takeOne_giveTwo(SomeEnum.one), SomeEnum.two)
    t |> equal(free_takeOne_giveTwo(SomeEnum.two), SomeEnum.zero)
    t |> equal(efn_takeOne_giveTwo(SomeEnum.one), SomeEnum.two)
    t |> equal(efn_takeOne_giveTwo_98(SomeEnum98.SomeEnum98_one), SomeEnum98.SomeEnum98_two)

    var result_efn = efn_takeOne_giveTwo(SomeEnum.one)
    t |> equal(result_efn, SomeEnum.two)

    var goo : GooEnum
    goo = GooEnum.regular
    t |> equal(efn_flip(goo), GooEnum.hazardous)
    var foo16 : TestObjectFoo
    t |> equal(foo16.e16, SomeEnum_16.SomeEnum_16_zero)
    foo16.e16 = SomeEnum_16.SomeEnum_16_one
    t |> equal(foo16.e16, SomeEnum_16.SomeEnum_16_one)
    t |> equal(int(foo16.e16), 1)
    t |> equal(typeinfo sizeof(foo16.e16), 2)
    for (e, tt in type<BooHoo>, fixed_array<int>(0, 1, 5)) {
        t |> equal(int(e), tt)
    }
    for (e, tt in type<BooHoo8>, fixed_array<int>(1, 2, 3)) {
        t |> equal(int(e), tt)
    }
    for (e, tt in type<BooHoo16>, fixed_array<int>(4, 5, 6)) {
        t |> equal(int(e), tt)
    }
    // binds
    let bind_ok = testBindEnumFunction()
    t |> success(bind_ok, "testBindEnumFunction failed")
}
