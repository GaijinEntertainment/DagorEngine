options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

module algorithm shared public

require daslib/array_boost
require daslib/contracts

def unique ( var a:array<auto(TT)> )
    //! Returns array of the elements of a with duplicates removed.
    var pidx = -1
    var b : array<TT-const-&>
    for e,eidx in a,range(length(a))
        if pidx==-1 || a[pidx]!=e
            pidx = eidx
            b |> push_clone(e)
    return <- b

def sort_unique ( var a : array<auto(TT)> )
    //! Returns array of the elements of a, sorted and with duplicates removed.
    //! The elements of a are sorted in ascending order.
    //! The resulted array has only unqiue elements.
    if empty(a)
        return
    sort(a)
    var b <- unique(a)
    delete a
    a <- b

def reverse ( var a:array<auto> )
    //! Returns array of the elements of a in reverse order.
    let l = length(a)
    let half = l/2
    let lm1 = l-1
    for i in range(half)
        swap(a[i], a[lm1-i])

def combine ( a,b:array<auto(TT)> )
    //! Returns array of the elements of a and then b.
    var c : array<TT>
    reserve(c, length(a) + length(b))
    for t in a
        c |> push(t)
    for t in b
        c |> push(t)
    return <- c

def lower_bound ( a:array<auto(TT)>; f,l:int; val : TT const-& )
    //! Returns an iterator pointing to the first element in the range [first, last) that is not less than (i.e. greater or equal to) value, or last if no such element is found.
    assert(f>=0 && f<=l,"lower bound first out of range")
    assert(l>=f && l<=length(a),"lower bound last out of range")
    var count = l - f
    var first = f
    while count > 0
        let step = count / 2
        let it = first + step
        if a[it] < val
            first += step + 1
            count -= step + 1
        else
            count = step
    return first

def lower_bound ( a:array<auto(TT)>; val : TT const-& )
    return lower_bound(a,0,length(a),val)

def lower_bound ( a:array<auto(TT)>; f, l : int; value:TT const-&; less : block<(a, b:TT const-&):bool> )
    assert(f>=0 && f<=l,"lower bound first out of range")
    assert(l>=f && l<=length(a),"lower bound last out of range")
    var count = l - f
    var first = f
    while count > 0
        let step = count / 2
        let it = first + step
        if invoke(less,a[it],value)
            first += step + 1
            count -= step + 1
        else
            count = step
    return first

def lower_bound ( a:array<auto(TT)>; value:TT const-&; less : block<(a, b:TT const-&):bool> )
    return lower_bound(a, 0, length(a), value, less )

def binary_search ( a:array<auto(TT)>; val : TT const-& )
    let first = lower_bound(a, val)
    return (first!=length(a)) && (!(val<a[first]))

def binary_search ( a:array<auto(TT)>; f, last : int; val : TT const-& )
    //! Returns true if an val appears within the range [f, last).
    //! Array a must be sorted.
    let first = lower_bound(a, f, last, val)
    return (first!=last) && (!(val<a[first]))

def binary_search ( a:array<auto(TT)>; val : TT const-&; less : block<(a, b:TT const-&):bool> )
    let first = lower_bound(a, val, less)
    let last = length(a)
    return (first!=last) && (!invoke(less,val,a[first]))

def binary_search ( a:array<auto(TT)>; f, last : int; val : TT const-&; less : block<(a, b:TT const-&):bool> )
    let first = lower_bound(a, f, last, val, less)
    return (first!=last) && (!invoke(less,val,a[first]))

// now for all the other types

[expect_any_array(a)]
def reverse ( var a )
    unsafe
        reverse(temp_array(a))

[expect_any_array(a)]
def combine ( a,b )
    unsafe
        return combine(temp_array(a),temp_array(b))

[expect_any_array(a)]
def lower_bound ( a; f,l:int; val )
    static_if typeinfo(stripped_typename a[0]) != typeinfo(stripped_typename val)
        concept_assert(false, "value type {typeinfo(stripped_typename val)} should be the same as array {typeinfo(stripped_typename a)} element type {typeinfo(stripped_typename a[0])}")
        return -1
    else
        unsafe
            return lower_bound(temp_array(a), f,l, val)

[expect_any_array(a)]
def lower_bound ( a; val )
    static_if typeinfo(stripped_typename a[0]) != typeinfo(stripped_typename val)
        concept_assert(false, "value type {typeinfo(stripped_typename val)} should be the same as array {typeinfo(stripped_typename a)} element type {typeinfo(stripped_typename a[0])}")
        return -1
    else
        unsafe
            return lower_bound(temp_array(a), val)

[expect_any_array(a)]
def lower_bound ( a; f, l : int; val:auto(TT); less : block<(a, b:TT const-&):bool> )
    static_if typeinfo(stripped_typename a[0]) != typeinfo(stripped_typename val)
        concept_assert(false, "value type {typeinfo(stripped_typename val)} should be the same as array {typeinfo(stripped_typename a)} element type {typeinfo(stripped_typename a[0])}")
        return -1
    else
        unsafe
            return lower_bound(temp_array(a), f, l, val, less)

[expect_any_array(a)]
def lower_bound ( a; val:auto(TT); less : block<(a, b:TT const-&):bool> )
    static_if typeinfo(stripped_typename a[0]) != typeinfo(stripped_typename val)
        concept_assert(false, "value type {typeinfo(stripped_typename val)} should be the same as array {typeinfo(stripped_typename a)} element type {typeinfo(stripped_typename a[0])}")
        return -1
    else
        unsafe
            return lower_bound(temp_array(a), val, less)

[expect_any_array(a)]
def binary_search ( a; val )
    static_if typeinfo(stripped_typename a[0]) != typeinfo(stripped_typename val)
        concept_assert(false, "value type {typeinfo(stripped_typename val)} should be the same as array {typeinfo(stripped_typename a)} element type {typeinfo(stripped_typename a[0])}")
        return -1
    else
        unsafe
            return binary_search(temp_array(a), val)

[expect_any_array(a)]
def binary_search ( a; f, last : int; val )
    static_if typeinfo(stripped_typename a[0]) != typeinfo(stripped_typename val)
        concept_assert(false, "value type {typeinfo(stripped_typename val)} should be the same as array {typeinfo(stripped_typename a)} element type {typeinfo(stripped_typename a[0])}")
        return -1
    else
        unsafe
            return binary_search(temp_array(a), f, last, val)

[expect_any_array(a)]
def binary_search ( a; val : auto(TT); less : block<(a, b:TT const-&):bool> )
    static_if typeinfo(stripped_typename a[0]) != typeinfo(stripped_typename val)
        concept_assert(false, "value type {typeinfo(stripped_typename val)} should be the same as array {typeinfo(stripped_typename a)} element type {typeinfo(stripped_typename a[0])}")
        return -1
    else
        unsafe
            return binary_search(temp_array(a), val, less)

[expect_any_array(a)]
def binary_search ( a; f, last : int; val : auto(TT); less : block<(a, b:TT const-&):bool> )
    static_if typeinfo(stripped_typename a[0]) != typeinfo(stripped_typename val)
        concept_assert(false, "value type {typeinfo(stripped_typename val)} should be the same as array {typeinfo(stripped_typename a)} element type {typeinfo(stripped_typename a[0])}")
        return -1
    else
        unsafe
            return binary_search(temp_array(a), f, last, val, less)

[expect_any_array(arr)]
def erase_all ( var arr; value )
    //! Erase all elements equal to value from arr
    static_if typeinfo(stripped_typename arr[0]) != typeinfo(stripped_typename value)
        concept_assert(false, "value type {typeinfo(stripped_typename value)} should be the same as array {typeinfo(stripped_typename arr)} element type {typeinfo(stripped_typename arr[0])}")
        return -1
    else
        var i = length(arr) - 1
        while i >= 0
            if arr[i] == value
                erase(arr, i)
            i --

def topological_sort ( nodes : array<auto(Node)> )
    //! Topological sort of a graph.
    //! Each node has an id, and set (table with no values) of dependencies.
    //! Dependency `before` represents a link from a node, which should appear in the sorted list before the node.
    //! Returns a sorted list of nodes.
    static_if !typeinfo(has_field <before> type<Node>) || !typeinfo(is_table nodes[0].before) || !typeinfo(has_field <id> type<Node>)
        concept_assert(false, "Node type {typeinfo(stripped_typename type<Node>)} should have fields 'before:table<id>' and 'id'")
    else
        var sorted : array<Node>
        let lnodes = length(nodes)
        if lnodes != 0
            sorted |> reserve(lnodes)
            var unsorted : array<Node> := nodes
            while length(unsorted) > 0
                var node <- unsorted[0]
                unsorted |> erase(0)
                if length(node.before) == 0
                    for other in unsorted
                        other.before |> erase(node.id)
                    sorted |> emplace(node)
                else
                    unsorted |>emplace(node)
            if length(sorted) != lnodes
                panic("cyclic dependency detected")
            delete unsorted
        return <- sorted

def intersection ( a,b : table<auto(TT)> ) : table<TT>
    //! Returns the intersection of two sets
    var result : table<TT>
    if length(a) < length(b)
        for k in keys(a)
            if b |> key_exists(k)
                result |> insert(k)
    else
        for k in keys(b)
            if a |> key_exists(k)
                result |> insert(k)
    return <- result

def union ( a,b : table<auto(TT)> ) : table<TT>
    //! Returns the union of two sets
    var result : table<TT>
    for k in keys(a)
        result |> insert(k)
    for k in keys(b)
        result |> insert(k)
    return <- result

def difference ( a,b : table<auto(TT)> ) : table<TT>
    //! Returns the difference of two sets
    var result : table<TT>
    for k in keys(a)
        if ! b |> key_exists(k)
            result |> insert(k)
    return <- result

def identical ( a,b : table<auto(TT)> ) : bool
    //! Returns true if the two sets are identical
    if length(a) != length(b)
        return false
    for k in keys(a)
        if ! b |> key_exists(k)
            return false
    return true
