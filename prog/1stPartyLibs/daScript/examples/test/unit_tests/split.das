require daslib/strings_boost

[sideeffects]
def expect0(a)
    assert(length(a)==1)
    assert(a[0]=="")

[sideeffects]
def expect1(a)
    assert(length(a)==1)
    assert(a[0]=="one")

[sideeffects]
def expect2(a)
    assert(length(a)==2)
    assert(a[0]=="one")
    assert(a[1]=="two")

[sideeffects]
def expect3(a)
    assert(length(a)==3)
    assert(a[0]=="one")
    assert(a[1]=="two")
    assert(a[2]=="three")

[sideeffects]
def expect3o(a)
    assert(length(a)==3)
    assert(a[0]=="o")
    assert(a[1]=="n")
    assert(a[2]=="e")

[export]
def test
    split("","") <| $ ( a )
        expect0(a)
    split("one",",") <| $ ( a )
        expect1(a)
    split("one,two",",") <| $ ( a )
        expect2(a)
    split("one, two, three",", ") <| $ ( a )
        expect3(a)
    split("one","") <| $ ( a )
        expect3o(a)
    expect0(split("",""))
    expect0(split("","."))
    expect1(split("one",","))
    expect2(split("one,two",","))
    expect3(split("one-->two-->three","-->"))
    expect3o(split("one",""))

    // empty
    let es <- split("","/")
    assert(length(es)==1)
    let ese <- split("","")
    assert(length(ese)==1)
    // e0
    let e0 <- split("/","/")
    assert(length(e0)==2)
    assert(e0[0]=="" && e0[1]=="")
    // e1
    let e1 <- split("/1","/")
    assert(length(e1)==2)
    assert(e1[0]=="" && e1[1]=="1")
    // e2
    let e2 <- split("1/","/")
    assert(length(e2)==2)
    assert(e2[0]=="1" && e2[1]=="")
    // e3
    let e3 <- split("1/2/3","/")
    assert(length(e3)==3)
    assert(e3[0]=="1" && e3[1]=="2" && e3[2]=="3")
    return true
