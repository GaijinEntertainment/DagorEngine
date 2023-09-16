require _typeinfo_macro_example

[export]
def test
    let T : auto(QQ) = "QQ"
    var tAlias = typeinfo(alias_name T)
    tAlias += "1"
    assert(tAlias=="QQ1")
    return true



