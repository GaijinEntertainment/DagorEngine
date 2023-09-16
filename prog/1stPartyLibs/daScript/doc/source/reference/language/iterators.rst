.. _iterators:

========
Iterator
========

Iterators are objects which can traverse over a sequence without knowing the details of the sequence's implementation.

The Iterator type is defined as follows::

    iterator_type ::= iterator < type >

    iterator<int>           // iterates over integer
    iterator<const Foo&>    // iterates over Foo by reference

Iterators can be moved, but not copied or cloned.

Iterators can be created via the ``each`` function from a range, static array, or dynamic array.
``each`` functions are unsafe because the iterator does not capture its arguments::

    unsafe
        var it <- each ( [[int 1;2;3;4]] )

The most straightforward way to traverse an iterator is with a ``for`` loop::

    for x in it             // iterates over contents of 'it'
        print("x = {x}\n")

For the reference iterator, the ``for`` loop will provide a reference variable::

    var t = [[int 1;2;3;4]]
    for x in t      // x is int&
        x ++        // increases values inside t

Iterators can be created from lambdas (see :ref:`Lambda <lambdas_iterator>`) or generators (see :ref:`Generator <generators>`).

Calling ``delete`` on an iterator will make it sequence out and free its memory::

    var it <- each_enum(Numbers one)
    delete it                           // safe to delete

    var it <- each_enum(Numbers one)
    for x in it
        print("x = {x}\n")
    delete it                           // its always safe to delete sequenced out iterator

Loops and iteration functions do it automatically.

-----------------
builtin iterators
-----------------

Table keys and values iterators can be obtained via the ``keys`` and ``values`` functions::

    var tab <- {{ "one"=>1; "two"=>2 }}
    for k,v in keys(tab),values(tab)        // keys(tab) is iterator<string>
        print("{k} => {v}\n")               // values(tab) is iterator<int&>

It is possible to iterate over each character of the string via the ``each`` function::

    unsafe
        for ch in each("hello,world!")      // string iterator is iterator<int>
            print("ch = {ch}\n")

It is possible to iterate over each element of an enumeration via the ``each_enum`` function::

    enum Numbers
        one
        two
        ten = 10

    for x in each_enum(Numbers one)         // argument is any value from said enumeration
        print("x = {x}\n")

-------------------------------------
builtin iteration functions
-------------------------------------

The ``empty`` function checks if an iterator is null or already sequenced out::

    unsafe
        var it <- each ( [[int 1;2;3;4]] )
        for x in it
            print("x = {x}\n")
        verify(empty(it))           // iterator is sequenced out

More complicated iteration patterns may require the ``next`` function::

    var x : int
    while next(it,x)        // this is semantically equivalent to the `for x in it`
        print("x = {x}\n")

Next can only operate on copyable types.

-------------------------------------
low level builtin iteration functions
-------------------------------------

``_builtin_iterator_first``, ``_builtin_iterator_next``, and ``_builtin_iterator_close`` address the regular lifecycle of the iterator.
A semantic equivalent of the for loop can be explicitly written using these operations::

    var it <- each(range(0,10))
    var i : int
    var pi : void?
    unsafe
        pi = reinterpret<void?> ( addr(i) )
    if _builtin_iterator_first(it,pi)
        print("i = {i}\n")
        while _builtin_iterator_next(it,pi)
            print("i = {i}\n")
        _builtin_iterator_close(it,pi)

``_builtin_iterator_iterate`` is one function to rule them all. It acts like all 3 functions above.
On a non-empty iterator it starts with 'first',
then proceeds to call `next` until the sequence is exhausted.
Once the iterator is sequenced out, it calls `close`::

    var it <- each(range(0,10))
    var i : int
    var pi : void?
    unsafe
        pi = reinterpret<void?> ( addr(i) )
    while _builtin_iterator_iterate(it,pi)      // this is equivalent to the example above
        print("i = {i}\n")

---------------------------
next implementation details
---------------------------

The function ``next`` is implemented as follows::

    def next ( it:iterator<auto(TT)>; var value : TT& ) : bool
        static_if !typeinfo(can_copy type<TT>)
            concept_assert(false, "requires type which can be copied")
        static_elif typeinfo(is_ref_value type<TT>)
            var pValue : TT - & ?
            unsafe
                if _builtin_iterator_iterate(it, addr(pValue))
                    value = *pValue
                    return true
                else
                    return false
        else
            unsafe
                return _builtin_iterator_iterate(it, addr(value))

It is important to notice that builtin iteration functions accept pointers instead of references.
