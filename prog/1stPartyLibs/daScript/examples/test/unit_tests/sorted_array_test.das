require daslib/algorithm

[export]
def test
    let a <- [{for x in range(10); x; where x!=4}]
    for i in range(-2,12)
        let lb = a |> lower_bound(i)
        let bs = a |> binary_search(i)
        if i<0
            assert(lb==0)
            assert(bs==false)
        elif i>=10
            assert(lb==9)
            assert(bs==false)
        elif i==4
            assert(lb==4)
            assert(bs==false)
        elif i<=3
            assert(lb==i)
            assert(bs==true)
        else
            assert(lb==i-1)
            assert(bs==true)
    return true
