.. _tables:


=====
Table
=====

.. index::
    single: Tables

Tables are associative containers implemented as a set of key/value pairs::

    var tab: table<string; int>
    tab["10"] = 10
    tab["20"] = 20
    tab["some"] = 10
    tab["some"] = 20  // replaces the value for 'some' key


There are several relevant builtin functions: ``clear``, ``key_exists``, ``find``, and ``erase``.
For safety, ``find`` doesn't return anything. Instead, it works with block as last argument. It can be used with the rbpipe operator::

    var tab: table<string; int>
    tab["some"] = 10
    find(tab,"some") <| $(var pValue: int? const)
        if pValue != null
            assert(deref(pValue) == 10)

If it was not done this way, ``find`` would have to return a pointer to its value, which would continue to point 'somewhere' even if data was deleted.
Consider this hypothetical ``find`` in the following example::

    var tab: table<string; int>
    tab["some"] = 10
    var v: int? = find(tab,"some")
    assert(v)      // not null!
    tab |> clear()
    deref(v) = 10  // where we will write this 10? UB and segfault!

So, if you just want to check for the existence of a key in the table, use ``key_exists(table, key)``.

Tables (as well as arrays, structs, and handled types) are passed to functions by reference only.

Tables cannot be assigned, only cloned or moved. ::

  def clone_table(var a, b: table<string, int>)
    a := b      // a is not deep copy of b
    clone(a, b) // same as above
    a = b       // error

  def move_table(var a, b: table<string, int>)
    a <- b  //a is no points to same data as b, and b is empty.

Table keys can be not only strings, but any other 'workhorse' type as well.

Tables can be constructed inline::

	let tab <- {{ "one"=>1; "two"=>2 }}

This is syntax sugar for::

	let tab : table<string;int> <- to_table_move([[tuple<string;int>[2] "one"=>1; "two"=>2]])



