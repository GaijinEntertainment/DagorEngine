options gen2

module _inline_generic_helper public

// fixture for the cross-origin generic-instance fence (optimization_auto_inline_functions):
// a generic defined HERE and instantiated from the test module produces an instance whose
// fromGeneric->module differs from the compiling module. heuristic sites inside such an
// instance must stay calls: a splice re-infers the WHOLE instance under the instantiating
// module's visibility, where the private tiny_scale_helper does not resolve - so without
// the fence, splicing small_boost below breaks compilation of the test module outright
// (the sqlite_boost _sql_create_table_sql failure, in miniature)

[never_inline]
def private tiny_scale_helper(x : int) : int => x + 3

def small_boost(x : int) : int => x + 1

def public gscale(a : auto(TT); k : TT) : TT {
    return tiny_scale_helper(small_boost(a)) * k
}
