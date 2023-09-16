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
