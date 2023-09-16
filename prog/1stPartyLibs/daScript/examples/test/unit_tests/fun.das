//options log_infer_passes = true
//options log = true
//options log_nodes = true
// options log_cpp = true
// options log_aot = true

require daslib/functional

def is_even(x:int)
    return x % 2 == 0

def twice(x:int)
    return x + x

def add(a,b:int)
    return a + b

[sideeffects]
def verify_even(arr)
    for x,y in arr,range(50)
        assert(x==y*2)

def first(x)
    return x._0

def operator != ( a,b : tuple<int;float> )
    return a._0!=b._0 || a._1!=b._1

def operator != ( a,b : array<int> )
    if length(a) != length(b)
        return true
    for x,y in a,b
        if x != y
            return true
    return false

def is_same(a,b)
    for x,y in a,b
        if x != y
            return false
    return true

def verify_14 ( t )
    for x,y in range(1,4),t
        assert(y[0]==x && y[1]==2)

[export]
def test
    // compehension
    verify_even() <| [[for x in range(100); x; where x % 2==0]]

    // array comprehension
    verify_even() <| [{for x in range(100); x; where x % 2==0}]

    // comprehension iterator with moveable type
    verify_14() <| [[for x in range(1,4); [{int x;2}] ]]

    // comprehension array with moveable type
    verify_14() <| [{for x in range(1,4); [{int x;2}] }]

    (   // filter with function
            range(100)
        |>  each()
        |>  filter(@@is_even)
        |>  verify_even()
    )

    (   // filter with lambda
            range(100)
        |>  each()
        |>  filter( @(x) => x % 2==0 )
        |>  verify_even()
    )

    (   // map
            range(50)
        |>  each()
        |>  map( @@twice )
        |>  verify_even()
    )

    (   // map with lambbda
            range(50)
        |>  each()
        |>  map( @(x : int) => x + x )
        |>  verify_even()
    )

    (   // filter and map with moveable type
            [[for x in range(100); [{int x;2}] ]]
        |>  filter( @(x) => x[0] % 2==0 )
        |>  map ( @(x) : array<int> => <- [{int x[0]; x[0]; x[0] }] )
        |>  map ( @(x) : int => x[0] )
        |>  verify_even()
    )

    (   // reduce non-ref iterator with block
            range(10)
        |>  each()
        |>  reduce( $(x,y) => x + y )
        |>  is_equal(45)
        |>  verify()
    )

    (   // reduce ref iterator with block
            [[ for x in range(10); x ]]
        |>  reduce( $(x,y) => x + y )
        |>  is_equal(45)
        |>  verify()
    )

    (   // reduce non-ref iterator with lambda
            range(10)
        |>  each()
        |>  reduce( @(x,y) => x + y )
        |>  is_equal(45)
        |>  verify()
    )

    (   // reduce non-ref iterator with function
            range(10)
        |>  each()
        |>  reduce( @@add )
        |>  is_equal(45)
        |>  verify()
    )

    (   // reduce single element sequence
            range(45,46)
        |>  each()
        |>  reduce( @@add )
        |>  is_equal(45)
        |>  verify()
    )

    (   // reduce ref type
            range(10)
        |>  each()
        |>  map( @(x : int) => [[auto x, 1.5]] )
        |>  reduce( $(x,y) => [[auto x._0 + y._0, 2.f]] )
        |>  first()
        |>  is_equal(45)
        |>  verify()
    )

    (   // sum
            range(10)
        |>  each()
        |>  sum()
        |>  is_equal(45)
        |>  verify()
    )

    (   // any (pass)
            [[bool true;false]]
        |>  any()
        |>  verify()
    )

    (   // any (fail)
            range(10)
        |>  each()
        |>  map ( @(x) : bool => x > 30 )
        |>  any()
        |>  not()
        |>  verify()
    )

    (   // all (pass)
            range(10)
        |>  each()
        |>  map ( @(x) : bool => x < 100 )
        |>  all()
        |>  verify()
    )

    (   // all (fail)
            range(10)
        |>  each()
        |>  map ( @(x) : bool => x > 0 )
        // |>  to_array()   // if we uncomment this two, we can see intermediate values
        // |>  echo()
        |>  all()
        |>  not()
        |>  verify()
    )

    (   // islice
            range(10)
        |>  each()
        |>  islice(1,4)
        |>  sum()
        |>  is_equal(6)
        |>  verify()
    )

    (   // count
            count(0,10)
        |>  islice(0,10)
        |>  sum()
        |>  is_equal(450)
        |>  verify()
    )

    (   // infinite repeat
            repeat(10)
        |>  islice(0,5)
        |>  to_array()
        |>  length()
        |>  is_equal(5)
        |>  verify()
    )

    (   // limited repeat
            repeat(10,6)
        |>  to_array()
        |>  length()
        |>  is_equal(6)
        |>  verify()
    )

    (   // cycle <by value>
        range(1,4)
    |>  each()
    |>  cycle()
    |>  islice(0,10)
    |>  is_same( [[for x in range(10); (x % 3)+1]] )
    |>  verify()
    )

    (   // cycle <by reference>
        [[for x in range(1,4); [[tuple<int;float> x,2.5]]]]
    |>  cycle()
    |>  islice(0,10)
    |>  is_same([[for x in range(10); [[tuple<int;float> (x%3)+1,2.5]]]])
    |>  verify()
    )

    (   // cycle <by moveable>
        [[for x in range(1,4); [{int x;2}] ]]
    |>  cycle()
    |>  islice(0,10)
    |>  is_same([[for x in range(10); [{int (x%3)+1;2}] ]])
    |>  verify()
    )

    return true

