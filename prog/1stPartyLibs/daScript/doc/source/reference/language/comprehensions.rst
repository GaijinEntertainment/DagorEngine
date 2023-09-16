.. _comprehensions:

=============
Comprehension
=============

Comprehensions are concise notation constructs designed to allow sequences to be built with other sequences.

The syntax is inspired by that of a for loop::

    comprehension ::= array_comprehension | iterator_comprehension
    array_comprehension ::= [{ any_comprehension }]
    iterator_comprehension := [[ any_comprehension ]]
    any_comprehension ::= for argument_list in source_list; result { ; where optional_clause }
    argument_list ::= argument | argument_list ',' argument
    source_list ::= iterable_expression | source_list ',' iterable_expression

Comprehension produces either an iterator or a dynamic array, depending on the style of brackets::

    var a1 <- [[for x in range(0,10); x]]   // iterator<int>
    var a2 <- [{for x in range(0,10); x}]   // array<int>

A ``where`` clause acts as a filter::

    var a3 <- [{for x in range(0,10); x; where (x & 1) == 1}]   // only odd numbers

Just like a for loop, comprehension can iterate over multiple sources::

    var a4 <- [{for x,y in range(0,10),a1; x + y; where x==y }] // multiple variables

Iterator comprehension may produce a referenced iterator::

    var a = [[int 1;2;3;4]]
    var b <- [[for x in a; a]]  // iterator<int&> and will point to captured copy of the elements of a

Regular lambda capturing rules are applied for iterator comprehensions (see :ref:`Lambdas <lambdas>`).

Internally array comprehension produces an ``invoke`` of a local block and a for loop; whereas iterator comprehension produces a generator (lambda).
Array comprehensions are typically faster, but iterator comprehensions have less of a memory footprint.
