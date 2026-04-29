.. _variants:

=======
Variant
=======

Variants are nameless types which provide support for values that can be one of a number of named cases,
possibly each with different values and types:

.. code-block:: das

    var t : variant<i_value:uint,f_value:float>

There is a shorthand type alias syntax to define a variant:

.. code-block:: das

    variant U_F {
        i_value : uint
        f_value : float
    }

    typedef U_F = variant<i_value:uint,f_value:float> // exactly the same as the declaration above

Any two variants are the same type if they have the same named cases of the same types in the same order.

Variants hold the ``index`` of the current case, as well as the value for the current case only.

The current case selection can be checked via the ``is`` operator, and accessed via the ``as`` operator:

.. code-block:: das

    assert(t is i_value)
    assert(t as i_value == 0x3f800000)

The entire variant selection can be modified by copying the properly constructed variant of a different case:

.. code-block:: das

    t = U_F(i_value = 0x40000000)    // now case is i_value
    t = U_F(f_value = 1.0)           // now case is f_value

Accessing a variant case of the incorrect type will cause a panic:

.. code-block:: das

    t = U_F(i_value = 0x40000000)
    return t as f_value                 // panic, invalid variant index

Safe navigation is available via the ``?as`` operation:

.. code-block:: das

    return t ?as f_value ?? 1.0         // will return 1.0 if t is not f_value

Cases can also be accessed in an unsafe manner without checking the type:

.. code-block:: das

    unsafe {
        t.i_value = 0x3f800000
        return t.f_value                    // will return memory, occupied by f_value - i.e. 1.0f
    }

The current index can be determined via the ``variant_index`` function:

.. code-block:: das

    var t : U_F
    assert(variant_index(t)==0)

The index value for a specific case can be determined via the ``variant_index`` and ``safe_variant_index`` type traits.
``safe_variant_index`` will return -1 for invalid indices and types, whereas ``variant_index`` will report a compilation error:

.. code-block:: das

    assert(typeinfo variant_index<i_value>(t)==0)
    assert(typeinfo variant_index<f_value>(t)==1)
    assert(typeinfo variant_index<unknown_value>(t)==-1) // compilation error

    assert(typeinfo safe_variant_index<i_value>(t)==0)
    assert(typeinfo safe_variant_index<f_value>(t)==1)
    assert(typeinfo safe_variant_index<unknown_value>(t)==-1)

Current case selection can be modified with the unsafe operation ``safe_variant_index``:

.. code-block:: das

    unsafe {
        set_variant_index(t, typeinfo variant_index<f_value>(t))
    }

-------------------------
Alignment and data layout
-------------------------

Variants contain the 'index' of the current case, followed by a union of individual cases, similar to the following C++ layout:

.. code-block:: das

    struct MyVariantName {
        int32_t __variant_index;
        union {
            type0   case0;
            type1   case1;
            ...
        };
    };

Individual cases start from the same offset.

The variant type is aligned by the alignment of its largest case, but no less than that of an int32.

.. seealso::

    :ref:`Pattern matching <pattern-matching>` for matching and extracting variant cases,
    :ref:`Aliases <aliases>` for the ``typedef`` shorthand variant syntax,
    :ref:`Datatypes <datatypes_and_values>` for a list of built-in types,
    :ref:`Move, copy, and clone <clone_to_move>` for variant clone and move semantics,
    :ref:`Unsafe <unsafe>` for unsafe variant field access and ``?as`` without null coalescing,
    :ref:`Finalizers <finalizers>` for variant finalization.

