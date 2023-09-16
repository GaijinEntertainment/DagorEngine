expect 39902:2

def test
    let t = 1
    typeinfo(sizeof type<auto>)           // 39902: typeinfo(sizeof auto) is undefined
    typeinfo(typename type<auto>)         // 39902: typeinfo(typename auto) is undefined

