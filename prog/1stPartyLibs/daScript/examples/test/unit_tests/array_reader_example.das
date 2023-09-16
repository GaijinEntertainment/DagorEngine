// options log = true

require _array_reader

[export]
def test
    var x = %arr~\{\}\w\x\y\n%%
    //                \   {    \   }    \   w    \   x    \   y    \   n
    var y = [[int[12] 92; 123; 92; 125; 92; 119; 92; 120; 92; 121; 92; 110]]
    for t1,t2 in x,y
        assert(t1==t2)
    return true
