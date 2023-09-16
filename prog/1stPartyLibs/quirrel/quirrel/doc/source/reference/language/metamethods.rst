.. _metamethods:

-----------
Metamethods
-----------

Metamethods are a mechanism that allows the customization of certain aspects of the
language semantics. Those methods are normal functions placed in a class declaration;
It is possible to change many aspects of a table/class instance behavior by just defining
a metamethod.

For example when we use relational operators other than '==' on 2 instances, the VM will
check if the class has a method in his parent called '_cmp'; if so it will call it to determine
the relation between the objects.::

    class Comparable {
        constructor(n)
        {
            name = n;
        }
        function _cmp(other)
        {
            if(name<other.name) return -1;
            if(name>other.name) return 1;
            return 0;
        }
        name = null;
    }

    let a = Comparable("Alberto");
    let b = Comparable("Wouter");

    if(a>b)
        print("a>b")
    else
        print("b<=a");

^^^^^
_set
^^^^^

::

    _set(idx,val)

invoked when the index idx is not present in the object or in its delegate chain.
``_set`` must 'throw null' to notify that a key wasn't found but the there were not runtime errors (clean failure).
This allows the program to differentiate between a runtime error and a 'index not found'.

^^^^^
_get
^^^^^

::

    _get(idx)

invoked when the index idx is not present in the object or in its delegate chain.
_get must 'throw null' to notify that a key wasn't found but the there were not runtime errors (clean failure).
This allows the program to differentiate between a runtime error and a 'index not found'.

^^^^^^^^^
_newslot
^^^^^^^^^

::

    _newslot(key,value)

invoked when a script tries to add a new slot in a table.

if the slot already exists in the target table the method will not be invoked also if the
"new slot" operator is used.

^^^^^^^^^
_delslot
^^^^^^^^^

::

    _delslot(key)

invoked when a script deletes a slot from a table.
if the method is invoked quirrel will not try to delete the slot himself

^^^^^^^^
_add
^^^^^^^^

::

    _add(other)

the + operator

returns this + other

^^^^^^^^^^^^^^^^^^^^^^^^
_sub
^^^^^^^^^^^^^^^^^^^^^^^^

::

    _sub(other)

the - operator (like _add)

^^^^^^^^^^^^^^^^^^^^^^^^
_mul
^^^^^^^^^^^^^^^^^^^^^^^^

::

    _mul(other)

the ``*`` operator (like _add)

^^^^^^^^^^^^^^^^^^^^^^^^
_div
^^^^^^^^^^^^^^^^^^^^^^^^

::

    _div(other)

the ``/`` operator (like _add)

^^^^^^^^^^^^^^^^^^^^^^^^
_modulo
^^^^^^^^^^^^^^^^^^^^^^^^

::

    _modulo(other)

the ``%`` operator (like _add)

^^^^^^^^^
_unm
^^^^^^^^^

::

    _unm()

the unary minus operator

^^^^^^^^^^^^^^^^^^^^^^^^
_typeof
^^^^^^^^^^^^^^^^^^^^^^^^

::

    _typeof()

invoked by the typeof operator on tables, userdata, and class instances.

Returns the type of ``this`` as string

^^^^^^^^^^^^^^^^^^^^^^^^
_cmp
^^^^^^^^^^^^^^^^^^^^^^^^

::

    _cmp(other)

invoked to emulate the ``< > <= >=`` and ``<=>`` operators

returns an integer as follow:

+-----------+----------------------------+
| returns   | relationship               |
+===========+============================+
|  > 0      | if ``this`` > ``other``    |
+-----------+----------------------------+
|  0        | if ``this`` == ``other``   |
+-----------+----------------------------+
|  < 0      | if ``this`` < ``other``    |
+-----------+----------------------------+

^^^^^^^^^^^^^^^^^^^^^^^^
_call
^^^^^^^^^^^^^^^^^^^^^^^^

::

    _call(other)

invoked when a table, userdata, or class instance is called

^^^^^^^^^^^^^^^^^^^^^^^^
_cloned
^^^^^^^^^^^^^^^^^^^^^^^^

::

    _cloned(original)

invoked when a table or class instance is cloned(in the cloned table)

^^^^^^^^^^^^^^^^^^^^^^^^
_nexti
^^^^^^^^^^^^^^^^^^^^^^^^

::

    _nexti(previdx)

invoked when a userdata or class instance is iterated by a foreach loop.

If previdx==null it means that it is the first iteration.
The function has to return the index of the 'next' value.

^^^^^^^^^^^^^^^^^^^^^^^^
_tostring
^^^^^^^^^^^^^^^^^^^^^^^^

::

    _tostring()

Invoked when during string concatenation or when the ``print`` function prints a table, instance, or userdata.
The method is also invoked by the sq_tostring() API.

Must return a string representation of the object.
