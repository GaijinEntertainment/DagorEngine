.. _tutorial_integration_cpp_binding_enums:

.. index::
   single: Tutorial; C++ Integration; Binding Enums

============================================
 C++ Integration: Binding Enumerations
============================================

This tutorial shows how to expose C++ ``enum`` and ``enum class`` types
to daslang.  Topics covered:

* ``DAS_BASE_BIND_ENUM`` — the macro approach to enum binding
* ``DAS_BIND_ENUM_CAST`` — cast specialization (when needed)
* ``addEnumeration`` — registering enums in a module
* Manual ``Enumeration`` construction — an alternative to macros
* Using bound enums in ``addExtern`` functions


Prerequisites
=============

* Tutorial 04 completed (:ref:`tutorial_integration_cpp_binding_types`).
* Comfort with ``MAKE_TYPE_FACTORY`` and ``addExtern``.


Defining the C++ enums
========================

Both scoped (``enum class``) and unscoped (``enum``) enums can be bound.
This tutorial uses scoped enums — the modern C++ style:

.. code-block:: cpp

   enum class Direction : int {
       North = 0,
       East  = 1,
       South = 2,
       West  = 3
   };

   enum class Severity : int {
       Debug   = 0,
       Info    = 1,
       Warning = 2,
       Error   = 3
   };


``DAS_BASE_BIND_ENUM``
========================

The primary macro for binding enums.  It creates:

* A class ``Enumeration<DasName>`` that describes the enum's values
* A ``typeFactory<CppEnum>`` so that ``addExtern`` can resolve the type

.. code-block:: cpp

   DAS_BASE_BIND_ENUM(Direction, Direction,
       North,
       East,
       South,
       West
   )

   DAS_BASE_BIND_ENUM(Severity, Severity,
       Debug,
       Info,
       Warning,
       Error
   )

The first argument is the C++ enum type, the second is the daslang
name, and the remaining arguments are the enum values.  The generated
class names follow the pattern ``Enumeration<DasName>`` — e.g.
``EnumerationDirection``.

.. note::

   Place ``DAS_BASE_BIND_ENUM`` macros **before** ``using namespace das``.
   The macros define names inside ``namespace das`` that can collide with
   the global enum names if both namespaces are active.

For unscoped (C-style) enums, use ``DAS_BASE_BIND_ENUM_98`` instead.


``DAS_BIND_ENUM_CAST``
========================

This macro creates a ``cast<>`` specialization that lets enum values
cross the C++ / daslang boundary.  In many cases the SFINAE default in
the engine already handles this, but you may need it for more complex
enum types:

.. code-block:: cpp

   DAS_BIND_ENUM_CAST(Direction)


Registering enums in the module
================================

In the module constructor, call ``addEnumeration`` with the generated
class:

.. code-block:: cpp

   addEnumeration(make_smart<EnumerationDirection>());
   addEnumeration(make_smart<EnumerationSeverity>());


Manual enum construction
=========================

When the macros don't fit (e.g. you need to rename values, skip some, or
the enum lives in a deeply nested namespace), you can construct an
``Enumeration`` object by hand:

.. code-block:: cpp

   auto pEnum = make_smart<Enumeration>("Severity");
   pEnum->cppName  = "Severity";
   pEnum->external = true;
   pEnum->baseType = Type::tInt;
   pEnum->addIEx("Debug",   "Severity::Debug",   0, LineInfo());
   pEnum->addIEx("Info",    "Severity::Info",     1, LineInfo());
   pEnum->addIEx("Warning", "Severity::Warning",  2, LineInfo());
   pEnum->addIEx("Error",   "Severity::Error",    3, LineInfo());
   addEnumeration(pEnum);

You still need ``DAS_BASE_BIND_ENUM`` (or at least ``DAS_BIND_ENUM_CAST``)
for the ``typeFactory<>`` so that ``addExtern`` can match the type.


Binding functions that use enums
=================================

Once the enum type is registered, ``addExtern`` handles enum parameters
and return values automatically — no special treatment is required:

.. code-block:: cpp

   Direction opposite_direction(Direction d) {
       switch (d) {
           case Direction::North: return Direction::South;
           case Direction::South: return Direction::North;
           case Direction::East:  return Direction::West;
           case Direction::West:  return Direction::East;
           default:               return d;
       }
   }

   addExtern<DAS_BIND_FUN(opposite_direction)>(*this, lib, "opposite_direction",
       SideEffects::none, "opposite_direction")
           ->args({"d"});


Using bound enums in daslang
==============================

Enum values are accessed with dot syntax — ``EnumName.Value``:

.. code-block:: das

   require tutorial_05_cpp

   [export]
   def test() {
       let dir = Direction.North
       print("dir = {direction_name(dir)}\n")
       print("opposite = {direction_name(opposite_direction(dir))}\n")

       // Enum comparison
       let east = Direction.East
       let west = Direction.West
       print("opposite(East) == West? {opposite_direction(east) == west}\n")

       // Passing enums to C++ functions
       log_message(Severity.Warning, "disk space low")

       // Boolean result from enum logic
       print("Warning is severe? {is_severe(Severity.Warning)}\n")


Name collision warning
========================

The daslang engine defines some enum names internally (e.g.
``das::LogLevel`` in ``string_writer.h``).  If your C++ enum has the same
name as an engine-internal enum **and** you use ``using namespace das``,
you will get ambiguous symbol errors.  Rename your enum to avoid the
collision.


Building and running
====================

::

   cmake --build build --config Release --target integration_cpp_05
   bin\Release\integration_cpp_05.exe

Expected output::

   === Direction enum ===
   dir = North
   opposite of North = South
   Rotating CW: North East South West
   East == West? false
   opposite(East) == West? true

   === Severity enum ===
   [DEBUG] system starting up
   [INFO] ready to serve
   [WARN] disk space low
   [ERROR] connection lost
   Debug is severe?   false
   Warning is severe? true
   Error is severe?   true


.. seealso::

   Full source:
   :download:`05_binding_enums.cpp <../../../../tutorials/integration/cpp/05_binding_enums.cpp>`,
   :download:`05_binding_enums.das <../../../../tutorials/integration/cpp/05_binding_enums.das>`

   Previous tutorial: :ref:`tutorial_integration_cpp_binding_types`

   Next tutorial: :ref:`tutorial_integration_cpp_interop`
