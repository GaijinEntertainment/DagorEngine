.. _bitfields:

========
Bitfield
========

Bitfields are nameless types that represent a collection of flags in a single integer:

.. code-block:: das

    var t : bitfield < one,two,three >

There is a shorthand type alias syntax to define a bitfield:

.. code-block:: das

    bitfield bits123 {
        one
        two
        three
    }

    typedef bits123 = bitfield<one; two; three> // exactly the same as the declaration above

Bitfield flags can be 8, 16, 32 or 64 bits in size. By default, bitfields are 32 bits.
To specify a different size, use ``: bitfield : uintXX`` syntax:

.. code-block:: das

    var t8  : bitfield : uint8 <one; two; three>
    var t16 : bitfield : uint16 <one; two; three>
    var t32 : bitfield : uint <one; two; three>
    var t32_by_default : bitfield <one; two; three>
    var t64 : bitfield : uint64 <one; two; three>

A type alias notation can also be used to specify the size:

.. code-block:: das

    bitfield bits123_8bit : uint8 {
        one
        two
        three
    }

    bitfield bits123_16bit : uint16 {
        one
        two
        three
    }

    bitfield bits123_32bit : uint {
        one
        two
        three
    }

    bitfield bits123_64bit : uint64 {
        one
        two
        three
    }


Any two bitfields are the same type, if an underlying integer type is the same:

.. code-block:: das

    var a : bitfield<one; two; three>
    var b : bitfield<one; two>
    b = a

Individual flags can be read as if they were regular bool fields:

.. code-block:: das

    var t : bitfield < one; two; three >
    assert(!t.one)

If an alias is available, a bitfield can be constructed using alias notation:

.. code-block:: das

    assert(t==bits123.three)

Bitfields can be constructed via an integer value. Limited binary logical operators are available:

.. code-block:: das

    var t : bitfield < one; two; three > = bitfield(1<<1) | bitfield(1<<2)
    assert(!t.one && t.two && t.three)
    assert("{t}"=="(two|three)")
    t ^= bitfield(1<<1)

Bitfields support built-in constants:

.. code-block:: das

    bitfield OneTwoThree {
        one
        two
        three
        All = OneTwoThree.one | OneTwoThree.two | OneTwoThree.three
        None = bitfield(0)
    }

.. seealso::

    :ref:`Datatypes <datatypes_and_values>` for a complete list of built-in types,
    :ref:`Aliases <aliases>` for the ``typedef`` shorthand syntax,
    :ref:`Expressions <expressions>` for binary operators used with bitfields.

