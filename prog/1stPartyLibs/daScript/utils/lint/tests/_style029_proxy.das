options gen2

module _style029_proxy

// Proxy: re-exports the leaf publicly. Requiring this module but using only the
// leaf's symbols is the transitive-only pattern STYLE029 flags.
require _style029_leaf public

def style029_proxy_value() : int {
    return 1
}
