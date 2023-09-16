// options log = true

require _hint_macros

[tag_function (good,bad)]
def ttt
    return true

[export]
def test
    var hints <- [[HintLookup
        tab <- {{
            hash("one") => "one";
            hash("two") => "two"
        }}
    ]]
    let h1 = get_hint(hints, "one", "no hint")      // static name, fast lookup
    let h2 = get_hint(hints, "three", "no hint")    // static name, fast lookup
    var s3 = "tw"
    s3 += "o"
    let h3 = get_hint(hints, s3, "no hint")         // this one with dynamic name
    assert(h1=="one")
    assert(h2=="no hint")
    assert(h3=="two")
    assert(g_non_hints==1)  // only one with dynamic name
    return true
