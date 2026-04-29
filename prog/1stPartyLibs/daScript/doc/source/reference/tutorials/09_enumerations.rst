.. _tutorial_enumerations:

============================
Enumerations and Bitfields
============================

.. index::
    single: Tutorial; Enumerations
    single: Tutorial; Bitfields

This tutorial covers enum declarations, dot-syntax access, underlying types,
iterating enums, string conversion, and bitfield flag operations.

Declaring an enum
=================

Enum values are auto-numbered starting from 0::

  enum Color {
      Red         // 0
      Green       // 1
      Blue        // 2
      Yellow      // 3
  }

You can assign explicit values::

  enum HttpStatus {
      OK = 200
      NotFound = 404
      ServerError = 500
  }

And specify the underlying storage type::

  enum Direction : uint8 {
      North
      South
      East
      West
  }

Using enums
===========

Access values with dot syntax — ``EnumName.Value``::

  let favorite = Color.Blue
  print("{favorite}\n")          // Blue

  if (favorite == Color.Blue) {
      print("it's blue!\n")
  }

Cast to the underlying integer with ``int()``::

  print("{int(Color.Blue)}\n")   // 2

Iterating over enum values
==========================

Use ``each()`` from ``daslib/enum_trait`` to iterate over all values of an
enum.  Pass any value of the enum type::

  require daslib/enum_trait

  for (c in each(Color.Red)) {
      print("{c} ")
  }
  // Red Green Blue Yellow

Get the number of values with ``typeinfo``::

  let n = typeinfo enum_length(type<Color>)

String conversion
=================

``string(enumValue)`` returns the value name.  ``to_enum`` converts a string
back::

  string(Color.Green)                               // "Green"
  to_enum(type<Color>, "Blue")                      // Color.Blue
  to_enum(type<Color>, "Purple", Color.Red)          // Color.Red (fallback)

Bitfields
=========

Bitfields pack boolean flags into a single integer.  Declare them like enums
but with ``bitfield``::

  bitfield FilePermissions {
      read
      write
      execute
      hidden
  }

Set and test flags with dot syntax::

  var perms : FilePermissions
  perms.read = true
  perms.write = true

  if (perms.read) {
      print("has read\n")
  }

You can also use ``|=`` with ``EnumName.Value`` to set flags::

  var rwx : FilePermissions
  rwx |= FilePermissions.read
  rwx |= FilePermissions.write
  rwx |= FilePermissions.execute
  print("{rwx}\n")               // (read|write|execute)

Clear with dot syntax or toggle with ``^=``::

  var flags : FilePermissions = rwx
  flags.write = false               // clear
  flags ^= FilePermissions.execute  // toggle

bitfield_boost
--------------

``require daslib/bitfield_boost`` adds index-based access for dynamic bit
manipulation::

  flags[0]          // read bit 0 (bool)
  flags[1] = true   // set bit 1
  each(flags)       // iterate over all bits as bool values

This is useful when you need to manipulate bits by index rather than by name.

Running the tutorial
====================

::

  daslang.exe tutorials/language/09_enumerations.das

Full source: :download:`tutorials/language/09_enumerations.das <../../../../tutorials/language/09_enumerations.das>`

See also
========

* **Next:** :ref:`tutorial_tables` — tables (dictionaries)
* :ref:`Constants and Enumerations <enumerations>` — full enum reference
* :ref:`Bitfields <bitfields>` — bitfield reference
