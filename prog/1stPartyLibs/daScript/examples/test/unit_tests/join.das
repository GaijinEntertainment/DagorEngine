require daslib/strings_boost

[export]
def test
    // dim
    verify ( join([[int[] 1]],",")=="1" )
    verify ( join([[int 1;2;3;4]],",")=="1,2,3,4" )
    // array
    var a : array<int>
    verify ( join(a," ")=="" )
    push(a,1)
    verify ( join(a,".")=="1" )
    push(a,2)
    verify ( join(a,".")=="1.2" )
    // range
    verify ( join(range(1,4),"-")=="1-2-3" )
    // string
    verify ( join("hello"," ")=="h e l l o" )
    // iterator
    let rit <- each(range(1,4))
    verify ( join(rit,"*")=="1*2*3" )
    return true
