.. _tutorial_random:

==========================================
Random Numbers
==========================================

.. index::
    single: Tutorial; Random Numbers
    single: Tutorial; RNG
    single: Tutorial; random_seed
    single: Tutorial; random_float
    single: Tutorial; Shuffle

This tutorial covers the ``daslib/random`` module — a deterministic
pseudo-random number generator built on a linear congruential generator (LCG).
All functions take a mutable ``int4`` seed state, making sequences
reproducible when given the same starting seed.

.. code-block:: das

    require daslib/random
    require math

Seeding the RNG
===============

Every RNG function takes a ``var int4&`` seed.  Create one from a single
integer with ``random_seed``.  The same seed always produces the same
sequence — useful for reproducible simulations:

.. code-block:: das

    var seed = random_seed(42)

    // Same seed -> same sequence
    var s1 = random_seed(42)
    var s2 = random_seed(42)
    print("{random_int(s1)} == {random_int(s2)}\n")  // true

Integer generation
==================

.. code-block:: das

    var seed = random_seed(1)
    print("{random_int(seed)}\n")       // 0..32767
    print("{random_big_int(seed)}\n")   // 0..1073741823
    print("{random_uint(seed)}\n")      // 0..4294967295

* ``random_int`` — range ``0..LCG_RAND_MAX`` (32767)
* ``random_big_int`` — wider range ``0..32768*32768-1``
* ``random_uint`` — full 32-bit unsigned range

Float generation
================

``random_float`` returns a value in ``[0.0, 1.0)``.  Map to a custom range
with simple arithmetic:

.. code-block:: das

    var seed = random_seed(7)
    let f = random_float(seed)         // 0.0 .. 1.0

    // Map to [10, 20)
    let lo = 10.0
    let hi = 20.0
    let mapped = lo + random_float(seed) * (hi - lo)

Vector generation
=================

.. code-block:: das

    var seed = random_seed(13)
    let ri = random_int4(seed)               // each component 0..32767
    let rf = random_float4(seed)             // each component 0..1
    let uv = random_unit_vector(seed)        // normalized direction
    let sp = random_in_unit_sphere(seed)     // point inside unit sphere
    let dk = random_in_unit_disk(seed)       // point inside unit disk (z=0)

* ``random_unit_vector`` — uniformly distributed direction (length = 1)
* ``random_in_unit_sphere`` — uniformly distributed point inside the unit
  sphere
* ``random_in_unit_disk`` — uniformly distributed point inside the unit disk
  (z component is always 0)

Infinite iterator
=================

``each_random_uint`` produces an infinite stream of random ``uint`` values.
Use ``break`` or ``take`` to limit it:

.. code-block:: das

    var count = 0
    for (val in each_random_uint(42)) {
        print("{val} ")
        count ++
        if (count >= 5) {
            break
        }
    }

Practical examples
==================

Dice roll
---------

.. code-block:: das

    def roll_dice(var seed : int4&) : int {
        return (random_int(seed) % 6) + 1
    }

Random pick from array
----------------------

.. code-block:: das

    def random_pick(arr : array<string>; var seed : int4&) : string {
        let idx = random_int(seed) % length(arr)
        return arr[idx]
    }

Fisher-Yates shuffle
--------------------

.. code-block:: das

    def shuffle(var arr : array<int>; var seed : int4&) {
        var i = length(arr) - 1
        while (i > 0) {
            let j = random_int(seed) % (i + 1)
            let tmp = arr[i]
            arr[i] = arr[j]
            arr[j] = tmp
            i --
        }
    }

Summary
=======

============================  =========================================
Function                      Returns
============================  =========================================
``random_seed(int)``          ``int4`` seed state
``random_int(seed)``          ``int`` 0..32767
``random_big_int(seed)``      ``int`` 0..1073741823
``random_uint(seed)``         ``uint`` full range
``random_float(seed)``        ``float`` 0..1
``random_int4(seed)``         ``int4`` each 0..32767
``random_float4(seed)``       ``float4`` each 0..1
``random_unit_vector(seed)``  ``float3`` length=1
``random_in_unit_sphere(s)``  ``float3`` inside unit sphere
``random_in_unit_disk(s)``    ``float3`` inside unit disk (z=0)
``each_random_uint(seed)``    infinite ``iterator<uint>``
============================  =========================================

.. seealso::

   Full source: :download:`tutorials/language/38_random.das <../../../../tutorials/language/38_random.das>`

   Previous tutorial: :ref:`tutorial_utility_patterns`

   Next tutorial: :ref:`tutorial_dynamic_type_checking`
