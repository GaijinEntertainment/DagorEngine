options gen2
options indenting = 4

module _test_template_tuple_composite_mod shared public

require daslib/typemacro_boost public

// ``Bag<T>`` — single-arg, composite field shape (``items : array<T>``).
// Pre-fix, the typemacro generic path failed to match ``$Bag<auto(TT)>``
// against a concrete ``Bag<int>`` because the per-field detection only
// caught direct alias and the bind-extraction was wrong-shaped for any
// wrapper.
[template_tuple(T)]
struct template public Bag {
    count : int = 0
    items : array<T>
}

// ``Pair<K, V>`` — multi-arg, top-level alias fields.
[template_tuple(K, V)]
struct template public Pair {
    key : K
    value : V
}

// ``KVList<K, V>`` — multi-arg, composite field shapes for both args.
[template_tuple(K, V)]
struct template public KVList {
    keys : array<K>
    values : array<V>
}

// Generic-path consumers — these are the call sites that were broken
// pre-fix. Resolution invokes the typemacro's generic path and binds
// the auto-named template arg through the field shape.
def public bag_size(b : $Bag < auto(TT) >) : int {
    return length(b.items)
}

def public pair_first(p : $Pair < auto(KK), auto(VV) >) : KK {
    return p.key
}

def public kvlist_size(l : $KVList < auto(KK), auto(VV) >) : int {
    return length(l.keys)
}
