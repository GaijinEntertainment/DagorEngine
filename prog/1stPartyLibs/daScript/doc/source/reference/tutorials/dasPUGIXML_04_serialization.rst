.. _tutorial_dasPUGIXML_serialization:

===========================================
PUGIXML-04 — Struct Serialization
===========================================

.. index::
    single: Tutorial; XML Serialization
    single: Tutorial; dasPUGIXML
    single: Tutorial; to_XML
    single: Tutorial; from_XML

This tutorial covers automatic struct ↔ XML serialization using
``to_XML``, ``from_XML``, and the low-level ``XML()`` builder in
``pugixml/PUGIXML_boost``.

Basic roundtrip
===============

``to_XML`` serializes any struct to an XML string.  ``from_XML``
parses an XML string back into a struct:

.. code-block:: das

   struct Player {
       name : string
       hp : int
       speed : float
       alive : bool
   }

   let p = Player(name = "Hero", hp = 100, speed = 5.5, alive = true)
   let xml_str = to_XML(p)
   var restored = from_XML(xml_str, type<Player>)
   print("name: {restored.name}, hp: {restored.hp}\n")
   // name: Hero, hp: 100

Custom root element
===================

By default the root element is ``<root>``.  Pass a second argument to
override:

.. code-block:: das

   let xml_str = to_XML(p, "player")
   // <player> ... </player> instead of <root> ... </root>

Nested structs
==============

Nested structs serialize as child elements and round-trip correctly:

.. code-block:: das

   struct Address {
       street : string
       city : string
       zip : string
   }

   struct Person {
       name : string
       age : int
       address : Address
   }

   let p = Person(
       name = "Alice", age = 30,
       address = Address(street = "123 Main St", city = "Springfield", zip = "62704")
   )
   let xml_str = to_XML(p, "person")
   var restored = from_XML(xml_str, type<Person>)
   print("restored: {restored.name}, {restored.address.city}\n")
   // restored: Alice, Springfield

Enums
=====

Enums serialize as their name by default.  Use ``@enum_as_int`` on a
field to serialize as the integer value instead:

.. code-block:: das

   enum Weapon {
       sword
       bow
       staff
   }

   struct Warrior {
       name : string
       weapon : Weapon             // serializes as "sword"
       @enum_as_int backup : Weapon  // serializes as "1"
   }

   let w = Warrior(name = "Knight", weapon = Weapon.sword, backup = Weapon.bow)
   let xml_str = to_XML(w, "warrior")
   var restored = from_XML(xml_str, type<Warrior>)
   print("weapon: {restored.weapon}, backup: {restored.backup}\n")
   // weapon: sword, backup: bow

Bitfields
=========

Bitfields serialize as their unsigned integer value:

.. code-block:: das

   bitfield Permissions {
       read
       write
       execute
   }

   let perms = Permissions.read | Permissions.execute
   let xml_str = to_XML(perms, "permissions")
   var restored = from_XML(xml_str, type<Permissions>)
   let has_read = uint(restored & Permissions.read) != 0u
   print("read: {has_read}\n")
   // read: true

Arrays and tables
=================

``array<T>`` elements become ``<item>`` children.  ``table<K;V>``
entries become ``<entry>`` children with ``<_key>`` and ``<_val>``:

.. code-block:: das

   struct Inventory {
       items : array<string>
       counts : table<string; int>
   }

   var inv <- Inventory(
       items <- ["sword", "shield", "potion"],
       counts <- { "sword" => 1, "shield" => 2, "potion" => 5 }
   )
   let xml_str = to_XML(inv, "inventory")
   var restored <- from_XML(xml_str, type<Inventory>)
   print("items: {length(restored.items)}\n")
   // items: 3

Tuples and variants
===================

Tuples serialize with ``_0``, ``_1``, … field names.  Variants store
the active index in a ``_variant`` attribute:

.. code-block:: das

   variant Shape {
       circle : float
       rectangle : float2
   }

   // Tuple roundtrip
   let t = (42, "hello", 3.14)
   let t_xml = to_XML(t, "tuple")
   var t_back = from_XML(t_xml, type<tuple<int; string; float>>)
   print("restored: ({t_back._0}, {t_back._1}, {t_back._2})\n")
   // restored: (42, hello, 3.14)

   // Variant roundtrip
   let s = Shape(circle = 5.0)
   let s_xml = to_XML(s, "shape")
   var s_back = from_XML(s_xml, type<Shape>)
   print("restored circle: {s_back as circle}\n")
   // restored circle: 5

Fixed-size arrays (dim)
========================

Fixed-size arrays serialize as sequential ``<item>`` children:

.. code-block:: das

   struct Matrix2x2 {
       data : int[4]
   }

   var m = Matrix2x2(data = fixed_array(1, 0, 0, 1))
   let xml_str = to_XML(m, "matrix")
   var restored = from_XML(xml_str, type<Matrix2x2>)
   print("restored: [{restored.data[0]}, {restored.data[1]}, {restored.data[2]}, {restored.data[3]}]\n")
   // restored: [1, 0, 0, 1]

Vector types
============

``float2/3/4`` and ``int2/3/4`` serialize with ``x``, ``y``, ``z``,
``w`` child elements:

.. code-block:: das

   let pos = float3(1.0, 2.5, -3.0)
   let xml_str = to_XML(pos, "position")
   var restored = from_XML(xml_str, type<float3>)
   print("restored: ({restored.x}, {restored.y}, {restored.z})\n")
   // restored: (1, 2.5, -3)

The ``@rename`` annotation
==========================

Use ``@rename = "xml_name"`` to change the XML element name for a
field without changing the daslang field name:

.. code-block:: das

   struct Config {
       @rename = "type" _type : string
       @enum_as_int level : Priority
       name : string
   }

The low-level ``XML()`` builder
===============================

``XML()`` serializes a struct's fields into an existing ``xml_node``.
``from_XML(node, type<T>)`` deserializes from a node.  This gives you
control over the document structure — e.g. embedding multiple records:

.. code-block:: das

   with_doc() <| $(doc) {
       doc |> tag("records") <| $(var records) {
           let p1 = Player(name = "Alice", hp = 100, speed = 5.0, alive = true)
           let p2 = Player(name = "Bob", hp = 80, speed = 3.5, alive = false)

           records |> tag("player") <| $(var node) {
               XML(node, p1)
           }
           records |> tag("player") <| $(var node) {
               XML(node, p2)
           }
       }

       // Read them back
       let records = doc.document_element
       records |> for_each_child("player") <| $(node) {
           var p = from_XML(node, type<Player>)
           print("  {p.name}: hp={p.hp}\n")
       }
   }

Complete roundtrip
==================

A full game-state roundtrip with nested struct, array, and vector:

.. code-block:: das

   struct GameState {
       level : int
       score : int
       player : Player
       weapons : array<string>
       position : float3
   }

   var state <- GameState(
       level = 3, score = 42000,
       player = Player(name = "Hero", hp = 95, speed = 6.0, alive = true),
       weapons <- ["sword", "bow", "fireball"],
       position = float3(10.5, 0.0, -3.2)
   )
   let xml_str = to_XML(state, "save")
   var loaded <- from_XML(xml_str, type<GameState>)
   print("Level: {loaded.level}, Score: {loaded.score}\n")
   print("Player: {loaded.player.name}, HP: {loaded.player.hp}\n")
   print("Weapons: {length(loaded.weapons)}\n")

.. seealso::

   Full source: :download:`tutorials/dasPUGIXML/04_serialization.das <../../../../tutorials/dasPUGIXML/04_serialization.das>`

   Previous tutorial: :ref:`tutorial_dasPUGIXML_xpath`
