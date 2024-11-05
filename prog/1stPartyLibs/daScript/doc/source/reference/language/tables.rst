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


There are several relevant builtin functions: ``clear``, ``key_exists``, ``get``, and ``erase``.
``get`` works with block as last argument, and returns if value has been found. It can be used with the rbpipe operator::

    var tab <- {{ "one"=>1; "two"=>2 }}
    let found = get(tab,"one") <| $(val)
		assert(val==1)
	assert(found)

There is non constant version available as well::

    [export]
    def main
        var tab <- {{ "one"=>1; "two"=>2 }}
        let found = get(tab,"one") <| $(var val)
            val = 123
        let t = tab["one"]
        assert(t==123)

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

Alternative syntax is::

    let tab <- table("one"=>1, "two"=>2)
    let tab <- table<string; int>("one"=>1, "two"=>2)

Table which holds no associative data can also be declared::

    var tab : table<int>
    tab |> insert(1)        // this is how we insert a key into such table

Table can be iterated over with the ``for`` loop::

    var tab <- {{ "one"=>1; "two"=>2 }}
    for key, value in keys(tab), values(tab)
        print("key: {key}, value: {value}\n")

Table which holds no associative data only has keys.
