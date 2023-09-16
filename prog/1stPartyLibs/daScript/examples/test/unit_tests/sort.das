def test_sort ( var arr : auto(TT) )
    sort ( arr )
    let len = length(arr)
    for i in range(1,len)
        assert ( arr[i-1] < arr[i] )
    sort ( arr, $(a,b) => !(a < b) )
    for i in range(1,len)
        assert ( !(arr[i-1] < arr[i]) )

def test_sort_comp ( var arr : auto(TT) )
    let len = length(arr)
    sort ( arr ) <| $ (a, b)
        return a.x < b.x
    for i in range(1, len)
        assert ( arr[i-1].x < arr[i].x )

struct Foo
    x, y : int

def operator < ( a, b : Foo )
    return a.x+a.y < b.x+b.y

def test_vector_sort
    var arr : array<float2>
    push(arr, float2(4, 0))
    push(arr, float2(3, 1))
    push(arr, float2(5, 2))
    test_sort_comp ( arr )

[export]
def test
    // numeric
    test_sort ( [[int 3; 2; 4; 1]] )
    test_sort ( [{int 3; 2; 4; 1}] )
    test_sort ( [[uint 3u; 2u; 4u; 1u]] )
    test_sort ( [{uint 3u; 2u; 4u; 1u}] )
    test_sort ( [[int64 3l; 2l; 4l; 1l]] )
    test_sort ( [{int64 3l; 2l; 4l; 1l}] )
    test_sort ( [[uint64 3ul; 2ul; 4ul; 1ul]] )
    test_sort ( [{uint64 3ul; 2ul; 4ul; 1ul}] )
    test_sort ( [[float 3.0; 2.0; 4.0; 1.0]] )
    test_sort ( [{float 3.0; 2.0; 4.0; 1.0}] )
    test_sort ( [[double 3.0lf; 2.0lf; 4.0lf; 1.0lf]] )
    test_sort ( [{double 3.0lf; 2.0lf; 4.0lf; 1.0lf}] )
    // string
    test_sort ( [[string "3.0lf"; "2.0lf"; "4.0lf"; "1.0lf"]] )
    test_sort ( [{string "3.0lf"; "2.0lf"; "4.0lf"; "1.0lf"}] )
    // custom type
    test_sort ( [[Foo x=1,y=2; x=1,y=1; x=2,y=2; x=0,y=1]] )    // Foo[]
    test_sort ( [{Foo x=1,y=2; x=1,y=1; x=2,y=2; x=0,y=1}] )    // array<Foo>
    // vector
    test_vector_sort()
    return true

