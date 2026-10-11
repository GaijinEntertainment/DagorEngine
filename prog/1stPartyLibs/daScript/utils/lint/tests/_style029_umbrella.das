options gen2

module _style029_umbrella

// Umbrella: re-exports the leaf publicly and carries a hook-only module as a
// side-effect require. Requiring it for the leaf's symbols alone is load-bearing -
// `require _style029_leaf` instead would drop the hook.
require _style029_leaf public
require _style030_finalize_only   // the hook rides only through this umbrella

def style029_umbrella_value() : int {
    return 2
}
