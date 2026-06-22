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

enum BooHoo64 : int64 {
    boo = 7l
    hoo = 8l
    boo_hoo = 9l
}

// Exercises high bits beyond int32 to lock in the int64-width cast in
// enum_trait's operator !/bool() — a naive int() cast would truncate
// (1ul << 63) to zero and incorrectly report the value as "no bits set".
enum BooHooU64 : uint64 {
    zero = 0ul
    low_bit = 1ul
    high_bit = 0x8000000000000000ul   // 2^63 — bit 63 set, all bits 0..62 zero
}

// Auto-increment after an expression initializer (lastEnuValue path) —
// requires infer-time folding to be on for the expression visit so that
// the +1 path sees a real ExprConstInt rather than the unfolded ExprOp1.
enum AutoIncrAfterExpr {
    first = -10
    second   // expected: -9 via lastEnuValue + 1
    third    // expected: -8
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

    let result_efn = efn_takeOne_giveTwo(SomeEnum.one)
    t |> equal(result_efn, SomeEnum.two)

    let goo : GooEnum = GooEnum.regular
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

[test]
def test_enum_bool_not_ops(t : T?) {
    // daslib/enum_trait.das `operator !(arg)` and `bool(arg)` for any enum:
    // returns true/false based on the underlying integer being zero / non-zero.
    // Enables flag-op-enum read-side: `if (bool(flags & MASK))` / `if (!(flags & MASK))`.

    // Zen — default int32 backing
    t |> success(!Zen.zero, "!Zen.zero (value 0) must be true")
    t |> success(!!Zen.not_zero, "!!Zen.not_zero (value 1) must be true")
    t |> success(!bool(Zen.zero), "bool(Zen.zero) must be false")
    t |> success(bool(Zen.not_zero), "bool(Zen.not_zero) must be true")

    // Numbers — int16 backing, including negative-valued members
    t |> success(!Numbers.zero, "!Numbers.zero must be true")
    t |> success(!!Numbers.one, "Numbers.one is non-zero")
    t |> success(!bool(Numbers.zero), "bool(Numbers.zero) must be false")
    t |> success(bool(Numbers.one), "bool(Numbers.one) must be true")
    t |> success(bool(Numbers.neg_one), "bool(Numbers.neg_one) must be true (non-zero)")
    t |> success(bool(Numbers.neg_ten), "bool(Numbers.neg_ten) must be true (non-zero)")

    // BooHoo8 — int8 backing, no zero member, all values non-zero
    t |> success(bool(BooHoo8.boo), "bool(BooHoo8.boo=1) must be true")
    t |> success(!!BooHoo8.hoo, "BooHoo8.hoo=2 is non-zero")
    t |> success(!(!BooHoo8.boo_hoo), "BooHoo8.boo_hoo=3 is non-zero")

    // BooHoo16 — int16 backing, same shape
    t |> success(bool(BooHoo16.boo), "bool(BooHoo16.boo=4) must be true")
    t |> success(!!BooHoo16.boo_hoo, "BooHoo16.boo_hoo=6 is non-zero")

    // BooHoo64 — int64 backing
    t |> success(bool(BooHoo64.boo), "bool(BooHoo64.boo=7) must be true")

    // BooHooU64 — uint64 backing, exercises bit 63 (the truncation hazard).
    // A naive `int(arg)` cast would chop high bits and read `high_bit` as 0.
    t |> success(!BooHooU64.zero, "!BooHooU64.zero must be true")
    t |> success(!bool(BooHooU64.zero), "bool(BooHooU64.zero) must be false")
    t |> success(bool(BooHooU64.low_bit), "bool(BooHooU64.low_bit) must be true")
    t |> success(bool(BooHooU64.high_bit),
        "bool(BooHooU64.high_bit=2^63) must be true (int-truncation regression check)")
    t |> success(!!BooHooU64.high_bit,
        "!!BooHooU64.high_bit=2^63 must be true (int-truncation regression check)")

    // Auto-increment after expression initializer — confirms preVisitEnumerationValue
    // folds the `-10` literal so lastEnuValue carries a real ExprConstInt into the
    // +1 path. (Pre-fix: `error[30147] must be integer constant, and not an expression`
    // under lint policies; post-fix: AutoIncrAfterExpr.second resolves to -9.)
    t |> equal(int(AutoIncrAfterExpr.first), -10)
    t |> equal(int(AutoIncrAfterExpr.second), -9)
    t |> equal(int(AutoIncrAfterExpr.third), -8)
}
