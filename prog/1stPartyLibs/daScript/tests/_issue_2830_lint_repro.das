options gen2

// Regression fixture for https://github.com/GaijinEntertainment/daScript/issues/2830
// `[decs_template]` synthesizes a per-field `decs::set(cmp, name, src.field)` call
// chain; `decs::set` is a generic with a `concept_assert(typeinfo sizeof(value) <=
// typeinfo sizeof(cmp[0].data), ...)`. Under lint flags
// (`no_infer_time_folding=true` plus `no_optimizations=true`) the `int_const <=
// int_const` cond stays as an unfolded `ExprOp2`, and `ContractFolding` raised a
// spurious `error[30151]` (static assert condition is not constexpr or const).
//
// `decs_boost`'s `append_iterator` also used a plain `if (typeinfo has_field<_aka>(a))`
// where the dead branch's `a._aka` reference fails to resolve once infer-time folding
// is off — same root cause, separate site.
//
// `extended_checks` runs lint on changed .das files (linux x64 only by gate); this
// file lives in `tests/` so the lint job picks it up.

require daslib/decs_boost
require daslib/linq_boost
require daslib/linq_fold

[decs_template(prefix = "x_")]
struct X {
    a : int
    b : int
    c : int
}

[export]
def target() : int {
    return _fold(from_decs_template(type<X>)._where(_.a + _.b + _.c > 0).count())
}

[export]
def main {
    print("count={target()}\n")
}
