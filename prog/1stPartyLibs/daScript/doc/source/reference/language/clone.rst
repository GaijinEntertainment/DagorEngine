.. _clone:

=====
Clone
=====

Clone is designed to create a deep copy of the data.
Cloning is invoked via the clone operator ``:=``::

    a := b

Cloning can be also invoked via the clone initializer in a variable declaration::

    var x := y

This in turn expands into ``clone_to_move``::

    var x <- clone_to_move(y)

(see :ref:`clone_to_move <clone_to_move>`).

----------------------------------------
Cloning rules and implementation details
----------------------------------------

Cloning obeys several rules.

Certain types like blocks, lambdas, and iterators can't be cloned at all.

However, if a custom clone function exists, it is immediately called regardless of the type's cloneability::

    struct Foo
        a : int

    def clone ( var x : Foo; y : Foo )
        x.a = y.a
        print("cloned\n")

    var l = [[Foo a=1]]
    var cl : Foo
    cl := l                 // invokes clone(cl,l)


Cloning is typically allowed between regular and temporary types (see :ref:`Temporary types <temporary>`).

POD types are copied instead of cloned::

    var a,b : int
    var c,d : int[10]
    a := b
    c := d

This expands to::

    a = b
    c = d

Handled types provide their own clone functionality via ``canClone``, ``simulateClone``,
and appropriate ``das_clone`` C++ infrastructure (see :ref:`Handles <handles>`).

For static arrays, the ``clone_dim`` generic is called,
and for dynamic arrays, the ``clone`` generic is called.
Those in turn clone each of the array elements::

    struct Foo
        a : array<int>
        b : int

    var a, b : array<Foo>
    b := a
    var c, d : Foo[10]
    c := d

This expands to::

    def builtin`clone ( var a:array<Foo aka TT> explicit; b:array<Foo> const )
        resize(a,length(b))
        for aV,bV in a,b
            aV := bV

    def builtin`clone_dim ( var a:Foo[10] explicit; b:Foo const[10] implicit explicit )
        for aV,bV in a,b
            aV := bV

For tables, the ``clone`` generic is called, which in turn clones its values::

    var a, b : table<string;Foo>
    b := a

This expands to::

    def builtin`clone ( var a:table<string aka KT;Foo aka VT> explicit; b:table<string;Foo> const )
        clear(a)
        for k,v in keys(b),values(b)
            a[k] := v

For structures, the default ``clone`` function is generated, in which each element is cloned::

    struct Foo
        a : array<int>
        b : int

This expands to::

    def clone ( var a:Foo explicit; b:Foo const )
        a.a := b.a
        a.b = b.b   // note copy instead of clone

For tuples, each individual element is cloned::

    var a, b : tuple<int;array<int>;string>
    b := a

This expands to::

    def clone ( var dest:tuple<int;array<int>;string> -const; src:tuple<int;array<int>;string> const -const )
        dest._0 = src._0
        dest._1 := src._1
        dest._2 = src._2

For variants, only the currently active element is cloned::

    var a, b : variant<i:int;a:array<int>;s:string>
    b := a

This expands to::

    def clone ( var dest:variant<i:int;a:array<int>;s:string> -const; src:variant<i:int;a:array<int>;s:string> const -const )
        if src is i
            set_variant_index(dest,0)
            dest.i = src.i
        elif src is a
            set_variant_index(dest,1)
            dest.a := src.a
        elif src is s
            set_variant_index(dest,2)
            dest.s = src.s

.. _clone_to_move:

----------------------------
clone_to_move implementation
----------------------------

``clone_to_move`` is implemented via regular generics as part of the builtin module::

    def clone_to_move(clone_src:auto(TT)) : TT -const
        var clone_dest : TT
        clone_dest := clone_src
        return <- clone_dest

Note that for non-cloneable types, Daslang will not promote ``:=`` initialize into ``clone_to_move``.
