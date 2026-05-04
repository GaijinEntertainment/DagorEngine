.. _tutorial_data_walker:

========================================
Data Walking with DapiDataWalker
========================================

.. index::
    single: Tutorial; DapiDataWalker
    single: Tutorial; Data Walking
    single: Tutorial; JSON Serializer
    single: Tutorial; Runtime Introspection

This tutorial covers ``DapiDataWalker`` — a visitor-pattern class for
inspecting and transforming daslang values at runtime.  You subclass
it, override callbacks for the types you care about, and walk any data
through RTTI type information.

Prerequisites: basic daslang knowledge (structs, arrays, tables,
enumerations, bitfields).

.. code-block:: das

    options gen2
    options rtti

    require daslib/rtti
    require daslib/debugger
    require daslib/strings_boost


Minimal walker — scalar types
==============================

A ``DapiDataWalker`` subclass overrides only the callbacks you need.
All 87 methods default to no-ops, so a minimal walker that prints
integers, floats, strings, and booleans is very small:

.. code-block:: das

    class ScalarPrinter : DapiDataWalker {
        def override Int(var value : int&) : void {
            print("  int: {value}\n")
        }
        def override Float(var value : float&) : void {
            print("  float: {value}\n")
        }
        def override String(var value : string&) : void {
            print("  string: \"{value}\"\n")
        }
        def override Bool(var value : bool&) : void {
            print("  bool: {value}\n")
        }
    }

To walk a value, create the walker, wrap it with ``make_data_walker``,
then call ``walk_data`` with a pointer and ``TypeInfo``:

.. code-block:: das

    var walker = new ScalarPrinter()
    var inscope adapter <- make_data_walker(walker)

    var x = 42
    unsafe {
        adapter |> walk_data(addr(x), typeinfo rtti_typeinfo(x))
    }

``typeinfo rtti_typeinfo(variable)`` is a compile-time intrinsic that
returns the RTTI ``TypeInfo`` for any variable or type expression.


Walking structures
===================

The walker calls ``beforeStructure``/``afterStructure`` around the
whole struct, and ``beforeStructureField``/``afterStructureField``
around each field.  ``StructInfo.name`` gives the struct type name,
``VarInfo.name`` gives the field name:

.. code-block:: das

    struct Vec3 {
        x : float
        y : float
        z : float
    }

    struct Player {
        name : string
        health : int
        pos : Vec3
    }

    class StructPrinter : DapiDataWalker {
        indent : int = 0

        def pad() {
            for (_ in range(indent)) {
                print("  ")
            }
        }

        def override beforeStructure(ps : void?; si : StructInfo) : void {
            print("{si.name} \{\n")
            indent++
        }

        def override afterStructure(ps : void?; si : StructInfo) : void {
            indent--
            self->pad()
            print("\}\n")
        }

        def override beforeStructureField(ps : void?; si : StructInfo;
                pv : void?; vi : VarInfo; last : bool) : void {
            self->pad()
            print("{vi.name} = ")
        }

        // ... plus Int, Float, String overrides
    }

Walking a ``Player`` value produces indented output showing nested
structures:

.. code-block:: text

    Player {
      name = "Alice"
      health = 100
      pos = Vec3 {
        x = 1
        y = 2.5
        z = -3
      }
    }


Arrays and tables
==================

Dynamic arrays trigger ``beforeArrayData``/``afterArrayData`` plus
per-element callbacks.  Tables trigger ``beforeTable``/``afterTable``
with key/value pairs:

.. code-block:: das

    class ContainerPrinter : DapiDataWalker {
        indent : int = 0

        def override beforeArrayData(ps : void?; stride : uint;
                count : uint; ti : TypeInfo) : void {
            self->pad()
            print("array[{int(count)}] = [\n")
            indent++
        }

        def override beforeArrayElement(ps : void?; ti : TypeInfo;
                pe : void?; index : uint; last : bool) : void {
            self->pad()
            print("[{int(index)}] = ")
        }

        def override beforeTable(pa : DapiTable; ti : TypeInfo) : void {
            self->pad()
            print("table[{int(pa.size)}] = \{\n")
            indent++
        }

        // ... afterTable, beforeTableKey, afterTableKey, etc.
    }

Note that ``count``, ``index``, and ``pa.size`` are ``uint`` values
which print as hexadecimal by default — cast to ``int`` for decimal
output.


Tuples and variants
====================

Tuples walk each element by index.  Variants walk only the active
alternative.  ``beforeTupleEntry`` receives the element index,
``beforeVariant`` receives the ``TypeInfo`` of the active case:

.. code-block:: das

    typedef Result = variant<ok : int; err : string>

    class TupleVariantPrinter : DapiDataWalker {
        def override beforeTupleEntry(ps : void?; ti : TypeInfo;
                pv : void?; idx : int; last : bool) : void {
            self->pad()
            print("_{idx} = ")
        }

        def override beforeVariant(ps : void?; ti : TypeInfo) : void {
            self->pad()
            print("variant = ")
        }

        // ... plus scalar overrides for Int, String, etc.
    }

Walking ``Result(ok = 42)`` prints ``variant = 42``.
Walking ``Result(err = "not found")`` prints ``variant = "not found"``.


Enumerations and bitfields
===========================

Enumerations trigger ``WalkEnumeration`` with an ``EnumInfo``
containing value names.  Bitfields trigger ``Bitfield`` with a
``TypeInfo`` containing field names:

.. code-block:: das

    class EnumBitfieldPrinter : DapiDataWalker {
        def override WalkEnumeration(var value : int&; ei : EnumInfo) : void {
            for (i in range(ei.count)) {
                unsafe {
                    if (int(ei.fields[i].value) == value) {
                        print("  enum {ei.name}.{ei.fields[i].name} ({value})\n")
                        return
                    }
                }
            }
            print("  enum {ei.name} = {value}\n")
        }

        def override Bitfield(var value : uint&; ti : TypeInfo) : void {
            print("  bitfield = {value} [")
            var first = true
            for (i in range(ti.argCount)) {
                if ((value & (1u << uint(i))) != 0u) {
                    if (!first) {
                        print(", ")
                    }
                    first = false
                    unsafe {
                        print("{ti.argNames[i]}")
                    }
                }
            }
            print("]\n")
        }
    }


JSON serializer — putting it all together
===========================================

A practical walker that serializes any daslang value to JSON.
This combines structure, array, table, tuple, and scalar callbacks
into one coherent class.  The walker writes to a ``StringBuilderWriter``
for efficiency — no intermediate string concatenation:

.. code-block:: das

    class JsonWalker : DapiDataWalker {
        @do_not_delete writer : StringBuilderWriter?
        indent : int = 0
        needComma : array<bool>

        // --- structures ---
        def override beforeStructure(ps : void?; si : StructInfo) : void {
            *writer |> write("\{")
            indent++
            pushComma()
        }

        def override beforeStructureField(ps : void?; si : StructInfo;
                pv : void?; vi : VarInfo; last : bool) : void {
            comma()
            nl()
            *writer |> write("\"")
            *writer |> write(vi.name)
            *writer |> write("\": ")
        }

        // --- arrays ---
        def override beforeArrayData(ps : void?; stride : uint;
                count : uint; ti : TypeInfo) : void {
            *writer |> write("[")
            indent++
            pushComma()
        }

        // ... tables, tuples, scalars
    }

The ``to_json`` helper wraps the walk in ``build_string``:

.. code-block:: das

    def to_json(var value; tinfo : TypeInfo) : string {
        var walker = new JsonWalker()
        var inscope adapter <- make_data_walker(walker)
        let res = build_string() $(var writer) {
            unsafe {
                walker.writer = addr(writer)
                adapter |> walk_data(addr(value), tinfo)
            }
        }
        unsafe { delete walker }
        return res
    }

Walking an ``Inventory`` struct produces well-formatted JSON:

.. code-block:: json

    {
      "owner": "Alice",
      "gold": 250,
      "items": [
        {
          "name": "Sword",
          "weight": 3.5
        },
        {
          "name": "Shield",
          "weight": 5.2
        },
        {
          "name": "Potion",
          "weight": 0.5
        }
      ],
      "flags": {
        "is_merchant": true,
        "is_hostile": false
      }
    }


Filtering with canVisit
=========================

All ``canVisit*`` methods return ``true`` by default.  Override them
to return ``false`` and the walker skips that subtree entirely:

.. code-block:: das

    class FilteringWalker : DapiDataWalker {
        skipStructName : string

        def override canVisitStructure(ps : void?; si : StructInfo) : bool {
            if (si.name == skipStructName) {
                print("<skipped {si.name}>\n")
                return false
            }
            return true
        }

        // ... beforeStructure, afterStructure, field and scalar overrides
    }

Setting ``skipStructName = "Secret"`` skips the ``Secret`` struct
entirely, including all its fields:

.. code-block:: text

    PublicRecord {
      title = "Performance Review"
      secret = <skipped Secret>
      score = 95
    }


Mutating data in-place
========================

Scalar callbacks receive ``var value : T&`` — a mutable reference.
This means the walker can modify data during traversal:

.. code-block:: das

    class FloatClamper : DapiDataWalker {
        lo : float = 0.0
        hi : float = 1.0

        def override Float(var value : float&) : void {
            if (value < lo) {
                value = lo
            }
            if (value > hi) {
                value = hi
            }
        }
    }

Walking a ``Particle`` with out-of-range floats clamps them to
``[0..1]``:

.. code-block:: text

    Before clamping: x=-0.5 y=0.3 z=1.7 alpha=0.8
    After clamping:  x=0 y=0.3 z=1 alpha=0.8


Quick reference
================

=================================================  ====================================================
``DapiDataWalker``                                 Base class — subclass and override callbacks
``make_data_walker(walker)``                       Wrap class instance into ``smart_ptr<DataWalker>``
``walk_data(adapter, ptr, typeinfo)``              Walk data at ``ptr`` using RTTI ``TypeInfo``
``typeinfo rtti_typeinfo(var)``                    Get ``TypeInfo`` for any variable (compile-time)
``unsafe { addr(var) }``                           Get ``void?`` pointer to a variable
``canVisit*(...)``                                 Return ``false`` to skip a subtree
``beforeX(...) / afterX(...)``                     Container enter/exit callbacks
``Int, Float, String, Bool, ...``                  Scalar value callbacks (mutable references)
``WalkEnumeration(value, EnumInfo)``               Enumeration callback with value metadata
``Bitfield(value, TypeInfo)``                      Bitfield callback with field name metadata
=================================================  ====================================================


.. seealso::

   Full source: :download:`tutorials/language/47_data_walker.das <../../../../tutorials/language/47_data_walker.das>`

   :ref:`stdlib_debugapi` — ``DapiDataWalker`` class reference and
   ``make_data_walker`` / ``walk_data`` functions.

   :ref:`stdlib_rtti` — RTTI types (``TypeInfo``, ``StructInfo``,
   ``VarInfo``, ``EnumInfo``).

   Previous tutorial: :ref:`tutorial_apply_in_context`

   Next tutorial: :ref:`tutorial_apply`
