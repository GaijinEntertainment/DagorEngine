.. _tuples:

=====
Tuple
=====

Tuples are a concise syntax to create nameless data structures::

    tuple ::= tuple < element_list >
    element_list ::= nameless_element_list | named_element_list
    nameless_element_list ::= type | nameless_element_list ';' type
    named_element_list := name : type | named_element_list ';' name : type

Two tuple declarations are the same if they have the same number of types, and their respective types are the same::

    var a : tuple<int; float>
    var b : tuple<i:int; f:float>
    a = b

Tuple elements can be accessed via nameless fields, i.e. _ followed by the 0 base field index::

    a._0 = 1
    a._1 = 2.0

Named tuple elements can be accessed by name as well as via nameless field::

    b.i = 1         // same as _0
    b.f = 2.0       // same as _1
    b._1 = 2.0      // _1 is also available

Tuples follow the same alignment rules as structures (see :ref:`Structures <structs_alignment>`).

Tuple alias types can be constructed the same way as structures. For example::

    tuple Foo
        a : int
        b : float

It's the same as::

    typedef Foo = tuple<a:int;b:float>

Tuples can be constructed using the tuple constructor, for example::

    var a = [[auto 1,2.0,"3"]]
    var b = [[tuple<int;float;string> 1, 2.0, "3"]]

Alternative syntax is::

    var a = tuple(1,2.0,"3")
    var b = tuple<int;float;string>(1, 2.0, "3")

both auto a full type specification can be used to construct a tuple.
Array of tuples can be constructed using similar syntax, with a ; as a separator::

    var a = [[auto 1, 2.0, "3"; 4, 5.0, "6"]]

There is a shortcut syntax for constructing tuples, where the tuple is returned::

    return 1, 2.0, "3"  // same as return [[auto 1, 2.0, "3"]]

Tuples can be expanded upon the variable declaration, for example::

    var [[a, b, c]] = [[auto 1, 2.0, "3"]]

In this case only one variable is created, as well as for 'assume' expressions. I.e::

    var a`b`c = [[auto 1, 2.0, "3"]]
    assume a  = a`b`c._0
    assume b  = a`b`c._1
    assume c  = a`b`c._2



