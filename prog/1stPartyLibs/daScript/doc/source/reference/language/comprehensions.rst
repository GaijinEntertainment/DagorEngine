.. _comprehensions:

=============
Comprehension
=============

Comprehensions are concise notation constructs designed to allow sequences to be built from other sequences.

The syntax is inspired by that of a ``for`` loop.
Comprehensions produce either a dynamic array, an iterator, or a table, depending on the
style of brackets used:

* ``[ for(...); expr ]`` — produces a dynamic array
* ``[iterator [ for(...); expr ]]`` — produces an iterator
* ``{ for(...); key_expr => value_expr }`` — produces a table
* ``{ for(...); key_expr }`` — produces a key-only table (set)

An optional ``where`` clause can filter elements.

Examples:

.. code-block:: das

    var a1 <- [iterator for(x in range(0,10)); x]   // iterator<int>
    var a2 <- [for(x in range(0,10)); x]            // array<int>
    var at1 <- {for(x in range(0,10)); x}           // table<int>
    var at2 <- {for(x in range(0,10)); x=>"{x}"}    // table<int;string>

A ``where`` clause acts as a filter:

.. code-block:: das

    var a3 <- [for(x in range(0,10)); x; where (x & 1) == 1]   // only odd numbers

Just like a for loop, comprehension can iterate over multiple sources:

.. code-block:: das

    var a4 <- [for(x,y in range(0,10),a1); x + y; where x==y] // multiple variables

Iterator comprehension may produce a referenced iterator:

.. code-block:: das

    var a = [1,2,3,4]
    var b <- [iterator for(x in a); a]  // iterator<int&> and will point to captured copy of the elements of a

Regular lambda capturing rules apply to iterator comprehensions (see :ref:`Lambdas <lambdas>`).

Internally array comprehension produces an ``invoke`` of a local block and a for loop; whereas iterator comprehension produces a generator (lambda).
Array comprehensions are typically faster, but iterator comprehensions have less of a memory footprint.

.. seealso::

    :ref:`Arrays <arrays>` for the array type produced by array comprehensions,
    :ref:`Tables <tables>` for the table type produced by table comprehensions,
    :ref:`Iterators <iterators>` for the iterator type produced by iterator comprehensions,
    :ref:`Generators <generators>` for the generator mechanism underlying iterator comprehensions.
