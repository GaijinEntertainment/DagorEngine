.. _tutorial_serialization:

==========================================
Serialization (archive)
==========================================

.. index::
    single: Tutorial; Serialization
    single: Tutorial; Archive
    single: Tutorial; Binary Save/Load
    single: Tutorial; mem_archive_save
    single: Tutorial; mem_archive_load

This tutorial covers binary serialization using the ``daslib/archive`` module.
The module provides automatic serialization for all built-in types, structs,
arrays, tables, tuples, and variants.

.. code-block:: das

    require daslib/archive

Primitive serialization
=======================

``mem_archive_save`` serializes any value to an ``array<uint8>``.
``mem_archive_load`` deserializes it back:

.. code-block:: das

    var x = 42
    var data <- mem_archive_save(x)
    print("int data size: {length(data)} bytes\n")    // 4 bytes

    var loaded_x : int
    mem_archive_load(data, loaded_x)
    print("loaded int: {loaded_x}\n")                 // 42

This works for ``int``, ``float``, ``string``, and all other primitive types.

Struct serialization
====================

Structs are serialized field by field automatically:

.. code-block:: das

    struct Player {
        name : string
        hp : int
        x : float
        y : float
    }

    var player = Player(name = "Hero", hp = 100, x = 1.5, y = 2.5)
    var data <- mem_archive_save(player)

    var loaded : Player
    mem_archive_load(data, loaded)
    print("{loaded.name}, hp={loaded.hp}\n")    // Hero, hp=100

Array serialization
===================

Dynamic arrays serialize their length followed by all elements:

.. code-block:: das

    var scores <- [10, 20, 30, 40, 50]
    var data <- mem_archive_save(scores)

    var loaded : array<int>
    mem_archive_load(data, loaded)
    // loaded contains [10, 20, 30, 40, 50]

Table serialization
===================

Tables serialize their length followed by key-value pairs:

.. code-block:: das

    var inventory : table<string; int>
    inventory |> insert("sword", 1)
    inventory |> insert("potion", 5)
    inventory |> insert("arrow", 20)

    var data <- mem_archive_save(inventory)
    var loaded : table<string; int>
    mem_archive_load(data, loaded)

.. note::

   Table iteration order may vary, but all key-value pairs are preserved.

Tuple serialization
===================

.. code-block:: das

    var pair : tuple<name:string; score:int>
    pair.name = "Alice"
    pair.score = 99

    var data <- mem_archive_save(pair)
    var loaded : tuple<name:string; score:int>
    mem_archive_load(data, loaded)
    print("name={loaded.name}, score={loaded.score}\n")

Variant serialization
=====================

Variants serialize the active variant index plus its value.  Requires
``unsafe`` for variant field access:

.. code-block:: das

    variant Value {
        i : int
        f : float
        s : string
    }

    unsafe {
        var v1 = Value(i = 42)
        var data <- mem_archive_save(v1)
        var loaded : Value
        mem_archive_load(data, loaded)
        if (loaded is i) {
            print("loaded: int = {loaded.i}\n")    // 42
        }
    }

Nested structures
=================

Serialization is recursive — structs containing arrays, tables, and other
structs are handled automatically:

.. code-block:: das

    struct Inventory {
        items : array<string>
        counts : table<string; int>
    }

    struct GameState {
        player : Player
        inventory : Inventory
        level : int
    }

    var state : GameState
    state.player = Player(name = "Knight", hp = 80, x = 10.0, y = 20.0)
    state.level = 3
    state.inventory.items |> push("sword")
    state.inventory.counts |> insert("sword", 1)

    var data <- mem_archive_save(state)
    var loaded : GameState
    mem_archive_load(data, loaded)

Manual Archive usage
====================

For full control, create an ``Archive`` and ``MemSerializer`` manually.
This lets you write multiple values into a single byte stream:

.. code-block:: das

    // Writing phase
    var writer = new MemSerializer()
    var warch = Archive(reading = false, stream = writer)
    var name = "save_001"
    var score = 9999
    var tags <- ["rpg", "fantasy"]
    warch |> serialize(name)
    warch |> serialize(score)
    warch |> serialize(tags)
    var data <- writer->extractData()

    // Reading phase
    var reader = new MemSerializer(data)
    var rarch = Archive(reading = true, stream = reader)
    var r_name : string
    var r_score : int
    var r_tags : array<string>
    rarch |> serialize(r_name)
    rarch |> serialize(r_score)
    rarch |> serialize(r_tags)

The same ``serialize`` function works for both reading and writing — the
``Archive.reading`` flag determines the direction.

Custom serialize
================

You can override the serialization of any type by defining a ``serialize``
overload with the exact signature ``def serialize(var arch : Archive; var val : YourType&)``.
This overload is more specific than the generic struct serializer, so it wins —
as long as the ``serialize`` call happens within the module that defines the
overload.

.. note::

   ``mem_archive_save`` / ``mem_archive_load`` resolve ``serialize`` inside
   the archive module, so they won't pick up overloads defined in user code.
   Use the manual ``Archive`` + ``MemSerializer`` pattern instead.

Here a ``Color`` struct stores channels as floats but serializes as 3 compact
bytes:

.. code-block:: das

    struct Color {
        r : float = 0.0
        g : float = 0.0
        b : float = 0.0
    }

    def serialize(var arch : Archive; var c : Color&) {
        if (arch.reading) {
            var rb, gb, bb : uint8
            arch |> serialize_raw(rb)
            arch |> serialize_raw(gb)
            arch |> serialize_raw(bb)
            c.r = float(rb) / 255.0
            c.g = float(gb) / 255.0
            c.b = float(bb) / 255.0
        } else {
            var rb = uint8(clamp(c.r * 255.0, 0.0, 255.0))
            var gb = uint8(clamp(c.g * 255.0, 0.0, 255.0))
            var bb = uint8(clamp(c.b * 255.0, 0.0, 255.0))
            arch |> serialize_raw(rb)
            arch |> serialize_raw(gb)
            arch |> serialize_raw(bb)
        }
    }

Using it with the manual archive pattern:

.. code-block:: das

    var c = Color(r = 1.0, g = 0.5, b = 0.0)
    var writer = new MemSerializer()
    var warch = Archive(reading = false, stream = writer)
    warch |> serialize(c)
    var data <- writer->extractData()
    print("size: {length(data)} bytes\n")     // 3 (not 12)

    var reader = new MemSerializer(data)
    var rarch = Archive(reading = true, stream = reader)
    var loaded = Color()
    rarch |> serialize(loaded)

The custom overload is also picked up by the generic array serializer when the
call-site is in the same module:

.. code-block:: das

    var colors <- [Color(r=1.0, g=0.0, b=0.0), Color(r=0.0, g=1.0, b=0.0)]
    var writer = new MemSerializer()
    var warch = Archive(reading = false, stream = writer)
    warch |> serialize(colors)      // 4 + 2×3 = 10 bytes

Summary
=======

==================================  ================================================
Function                            Description
==================================  ================================================
``mem_archive_save(value)``         Serialize any value to ``array<uint8>``
``mem_archive_load(data, value)``   Deserialize from ``array<uint8>`` into value
``serialize(archive, value)``       Read or write depending on ``archive.reading``
``serialize_raw(archive, value)``   Raw byte read/write (no type dispatch)
``Archive``                         Struct combining stream + direction
``MemSerializer``                   In-memory byte stream (reading or writing)
==================================  ================================================

.. seealso::

   Full source: :download:`tutorials/language/41_serialization.das <../../../../tutorials/language/41_serialization.das>`

   Previous tutorial: :ref:`tutorial_coroutines`

   Next tutorial: :ref:`tutorial_testing_tools`
