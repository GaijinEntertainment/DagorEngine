options gen2
options auto_inline_functions = true
require _inline_shadow_helper
require math

// Regression test (was error[30838]: can't locate variable '__inl1_l_transform' at
// `transform[3] = center` before the splice declined on can_shadow capture).
//
// Minimal shape, reduced from dagor active_matter scripts/game/es/migration.das
// (lauch_migration): my_region has an expression body, so its call site takes the pure-graft
// splice. Its block literal declares `transform`, which the graft renames to __inl1_l_transform.
// The caller's callback -- substituted into that body -- has its own outer `transform` and ALSO
// contains a later can_shadow block redeclaring the name. With that inner block present, the
// caller's own reference `transform[3] = center` is renamed into the callee's __inl1_l_ namespace
// and infer then fails to locate it. Without the can_shadow inner block the same shape correctly
// reports 30701 at the callee instead. Only the INNER [shadowable] matters:
//   callee/outer/inner = any/any/shadowable -> 30838; any/any/plain -> 30701.
// Renaming the inner block argument (e.g. transform2) also makes it compile, which is the
// workaround dagor cannot use: [es] query components must keep their component names, and
// das_es.cpp marks every query block argument can_shadow.

[never_inline] def each_region(blk : block<(var tm : float3x4) : void>) {
    var t : float3x4
    invoke(blk, t)
}

def my_region(cb : block<(center : float3) : void>) {
    each_region() $(var transform : float3x4) {
        cb |> invoke(transform[3])
    }
}

[export] def main {
    each_region() $(var transform : float3x4) {
        my_region() $(center : float3) {
            transform[3] = center
            each_region() $ [shadowable] (var transform : float3x4) {
                print("{transform[3]}\n")
            }
        }
    }
}
