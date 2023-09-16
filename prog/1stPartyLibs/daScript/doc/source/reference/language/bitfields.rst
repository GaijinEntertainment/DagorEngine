.. _bitfields:

========
Bitfield
========

Bitfields are a nameless types which represent a collection of up to 32-bit flags in a single integer::

    var t : bitfield < one; two; three >

There is a shorthand type alias syntax to define a bitfield::

    bitfield bits123
        one
        two
        three

    typedef
        bits123 = bitfield<one; two; three> // exactly the same as the declaration above

Any two bitfields are the same type and represent 32-bit integer::

    var a : bitfield<one; two; three>
    var b : bitfield<one; two>
    b = a

Individual flags can be read as if they were regular bool fields::

    var t : bitfield < one; two; three >
    assert(!t.one)

If alias is available, bitfield can be constructed via alias notation::

    assert(t==bits123 three)

Bitfields can be constructed via an integer value. Limited binary logical operators are available::

    var t : bitfield < one; two; three > = bitfield(1<<1) | bitfield(1<<2)
    assert(!t.one && t.two && t.three)
    assert("{t}"=="(two|three)")
    t ^= bitfield(1<<1)
