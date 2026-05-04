function foo() {}
let gg = foo()

function _bar() {
    let { cfg = null, hasOwned = false } = gg.value

    if (cfg == null || hasOwned)
        return  // Early exit if cfg is null OR hasOwned is true

    // After return: cfg != null && !hasOwned
    let { _c, _g } = cfg  // No warning - cfg is non-null after the guard
    return
}
