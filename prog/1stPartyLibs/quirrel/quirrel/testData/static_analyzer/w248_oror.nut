//-file:declared-never-used

function foo() {}
let gg = foo()


function bar() {

    let { cfg = null, hasOwned = false } = gg.value

    if (cfg == null || hasOwned)
        return

    // !(cfg == null || hasOwned) -> cfg != null && hasOwned = true

    let { c, g } = cfg
    return
}