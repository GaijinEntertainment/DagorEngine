.. _tuples:

=====
Tuple
=====

Tuples are a concise syntax to create anonymous data structures.
A tuple type is declared with the ``tuple`` keyword followed by a list of element types
(optionally named) in angle brackets:

.. code-block:: das

    tuple<int; float>               // unnamed elements
    tuple<i:int; f:float>           // named elements

Two tuple declarations are the same if they have the same number of types, and their respective types are the same:

.. code-block:: das

    var a : tuple<int, float>
    var b : tuple<i:int, f:float>
    a = b

Tuple elements can be accessed via nameless fields, i.e. _ followed by the 0 base field index:

.. code-block:: das

    a._0 = 1
    a._1 = 2.0

Named tuple elements can be accessed by name as well as via nameless field:

.. code-block:: das

    b.i = 1         // same as _0
    b.f = 2.0       // same as _1
    b._1 = 2.0      // _1 is also available

Tuples follow the same alignment rules as structures (see :ref:`Structures <structs_alignment>`).

Tuple alias types can be constructed the same way as structures. For example:

.. code-block:: das

    tuple Foo {
        a : int
        b : float
    }

It's the same as:

.. code-block:: das

    typedef Foo = tuple<a:int,b:float>

Tuples can be constructed using the tuple constructor, for example:

.. code-block:: das

    var a = (1,2.0,"3")
    var b = tuple(1, 2.0, "3")

The ``=>`` operator creates a 2-element tuple from its left and right operands:

.. code-block:: das

    var c = "one" => 1   // same as tuple<string,int>("one", 1)

This works in any expression context, not just table literals.
Table literals like ``{ "one"=>1, "two"=>2 }`` use ``=>`` to form key-value tuples
that are then inserted into the table (see :ref:`Tables <tables>`).

Tuple elements can be assigned names via tuple constructor:

.. code-block:: das

    var a = tuple<a:int,b:float,c:string>(a=1, b=2.0, c="3")

Both ``auto`` and a full type specification can be used to construct a tuple.
Array of tuples can be constructed using similar syntax, with a comma as a separator:

.. code-block:: das

    let H : array<Tup> <- array tuple<Tup>((a = 1, b = 2., c = "3"), (a = 4, b = 5., c = "6"))

Tuples can be expanded upon the variable declaration, for example:

.. code-block:: das

    var (a, b, c) = (1, 2.0, "3")

In this case only one variable is created, as well as for 'assume' expressions. I.e:

.. code-block:: das

    var a`b`c = (1, 2.0, "3")
    assume a  = a`b`c._0
    assume b  = a`b`c._1
    assume c  = a`b`c._2

Iterators and containers can be expanded in the for-loop in a similar way:

.. code-block:: das

    var H <- [(1, 2.0, "3"), (4, 5.0, "6")]
    for ( (a, b, c) in H ) {
        assert(a == 1)
        assert(b == 2.0)
        assert(c == "3")
    }

.. seealso::

    :ref:`Datatypes <datatypes_and_values>` for a list of built-in types,
    :ref:`Pattern matching <pattern-matching>` for matching and destructuring tuples,
    :ref:`Finalizers <finalizers>` for tuple finalization,
    :ref:`Move, copy, and clone <clone_to_move>` for tuple copy and move rules,
    :ref:`Aliases <aliases>` for the ``typedef`` shorthand tuple syntax.

