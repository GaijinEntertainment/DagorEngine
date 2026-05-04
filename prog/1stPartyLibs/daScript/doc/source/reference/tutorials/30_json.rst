.. _tutorial_json:

========================
JSON
========================

.. index::
    single: Tutorial; JSON
    single: Tutorial; JSON Parsing
    single: Tutorial; JSON Serialization
    single: Tutorial; json_boost
    single: Tutorial; sprint_json
    single: Tutorial; sscan_json

This tutorial covers ``daslib/json`` and ``daslib/json_boost`` — parsing,
building, writing, and querying JSON data in daslang.

``json`` provides the core parser, writer, and ``JsValue`` variant type.
``json_boost`` adds safe access operators, struct serialization, and
the ``%json~`` reader macro.

Parsing JSON
============

``read_json`` takes a string and returns ``JsonValue?``.
If parsing fails, the error string is set and null is returned::

  var error : string
  var js = read_json("{ \"name\": \"Alice\", \"age\": 30 }", error)
  if (js != null) {
      print("parsed OK\n")
  }

The ``JsonValue`` struct wraps a ``JsValue`` variant with these cases:

- ``_object``  — ``table<string; JsonValue?>``
- ``_array``   — ``array<JsonValue?>``
- ``_string``  — ``string``
- ``_number``  — ``double`` (floating point)
- ``_longint`` — ``int64`` (integer)
- ``_bool``    — ``bool``
- ``_null``    — ``void?``

Building JSON with JV
=====================

``JV()`` constructors wrap native values into ``JsonValue?``::

  var str_val  = JV("hello")   // _string
  var int_val  = JV(42)        // _longint
  var flt_val  = JV(3.14)      // _number
  var bool_val = JV(true)      // _bool
  var null_val = JVNull()      // _null

Multi-value ``JV`` creates an array::

  var arr = JV(1, "two", true)   // [1, "two", true]

Writing JSON
============

``write_json`` converts a ``JsonValue?`` back to a string::

  var js = JV(42)
  print(write_json(js))   // 42

  var empty : JsonValue?
  print(write_json(empty))  // null

Safe access operators
=====================

``json_boost`` provides ``?.``, ``?[]``, and ``??`` for convenient access.
These never crash — they return null or a default value on missing keys::

  var js = read_json("{ \"user\": { \"name\": \"Bob\" } }", error)

  let name = js?.user?.name ?? "unknown"     // "Bob"
  let age  = js?.user?.age ?? 0              // 0 (missing key)

  // Array indexing
  var arr = read_json("[10, 20, 30]", error)
  let first = arr?[0] ?? -1                  // 10

  // Chained access on deeply missing paths returns the default
  let deep = js?.a?.b?.c ?? -1               // -1

  // null pointer is safe
  var nothing : JsonValue?
  let safe = nothing?.foo ?? "safe"          // "safe"

Variant checks with is/as
=========================

``json_boost`` rewrites ``is`` and ``as`` on ``JsonValue?`` to check
the underlying ``JsValue`` variant::

  var js = JV("hello")
  print("{js is _string}\n")     // true
  print("{js as _string}\n")     // hello

  var jn = JV(42)
  print("{jn is _longint}\n")    // true

Struct serialization
====================

``json_boost`` provides generic ``JV()`` and ``from_JV()`` that convert
structs to/from JSON using compile-time reflection::

  struct Player {
      name  : string
      hp    : int
      speed : float
  }

  let p = Player(name = "Hero", hp = 100, speed = 5.5)
  var js = JV(p)
  // {"speed":5.5, "name":"Hero", "hp":100}

  var p2 = from_JV(parsed, type<Player>)
  print("{p2.name}, hp={p2.hp}\n")

Enum serialization
==================

Enums serialize as strings by default::

  enum Weapon { sword; bow; staff }

  var js = JV(Weapon.bow)      // "bow"
  var w  = from_JV(js, type<Weapon>)   // Weapon.bow

Reader macro
============

The ``%json~`` reader macro embeds JSON directly in daslang code.
It parses at compile time and creates a ``JsonValue?`` at runtime::

  var settings = %json~
  {
      "resolution": [1920, 1080],
      "fullscreen": true,
      "title": "My Game"
  }
  %%

  let title = settings?.title ?? "untitled"   // "My Game"

Writer settings
===============

Global settings control ``write_json`` behavior:

- ``set_no_trailing_zeros(true)`` — omit trailing zeros from round doubles (``1.0`` → ``1``)
- ``set_no_empty_arrays(true)`` — skip empty array fields in objects

::

  let old = set_no_trailing_zeros(true)
  print(write_json(JV(1.0lf)))  // 1
  set_no_trailing_zeros(old)

Collections
===========

``JV`` and ``from_JV`` work with ``array<T>`` and ``table<string;T>``::

  var arr <- array<int>(10, 20, 30)
  var js = JV(arr)           // [10, 20, 30]
  var back = from_JV(js, type<array<int>>)

  var tab <- { "x" => 1, "y" => 2 }
  var jt = JV(tab)           // {"x":1, "y":2}

Vector types (``float2/3/4``, ``int2/3/4``) serialize as objects
with ``x``, ``y``, ``z``, ``w`` keys::

  var js = JV(float3(1.0, 2.0, 3.0))
  // {"x":1, "y":2, "z":3}

Tuples serialize with ``_0``, ``_1``, ... keys.
Variants include a ``$variant`` field.

Broken JSON repair
==================

``try_fixing_broken_json`` attempts to fix common issues from LLM output:

- String concatenation: ``"hello" + "world"`` → ``"helloworld"``
- Trailing commas: ``[1, 2, ]`` → ``[1, 2]``
- Nested quotes: ``"she said "hi""`` → ``"she said \"hi\""``

::

  let bad = "{ \"msg\": \"hello\", }"
  let fixed = try_fixing_broken_json(bad)
  var js = read_json(fixed, error)

sprint_json
===========

``sprint_json`` is a builtin function that serializes any daslang value
directly to a JSON string — no ``JsonValue?`` intermediate. It handles
structs, classes, variants, tuples, tables, arrays, enums, pointers,
and all basic types::

  struct Record {
      id     : int
      tag    : string
      data   : Payload
      values : array<int>
      meta   : table<string; int>
      coords : tuple<int; float>
      ptr    : void?
  }

  var r <- Record(uninitialized
      id = 1, tag = "test",
      data = Payload(uninitialized code = 42),
      values = [1, 2, 3],
      meta <- { "x" => 10 },
      coords = (7, 3.14),
      ptr = null
  )
  let compact = sprint_json(r, false)
  // {"id":1,"tag":"test","data":{"code":42},"values":[1,2,3],...}

  let pretty = sprint_json(r, true)
  // human-readable with indentation

The second argument controls human-readable formatting. Works with
simple values too::

  sprint_json(42, false)          // 42
  sprint_json("hello", false)     // "hello"
  sprint_json([10, 20, 30], false) // [10,20,30]

Field annotations
=================

Struct field annotations control how ``sprint_json`` and ``sscan_json`` handle fields.
These require ``options rtti`` to be enabled.

- ``@optional`` — skip the field if it has a default or empty value
- ``@embed`` — embed a string field as raw JSON (no extra quotes)
- ``@unescape`` — don't escape special characters in the string
- ``@enum_as_int`` — serialize an enum as its integer value, not a string
- ``@rename="key"`` — use ``key`` as the JSON field name instead of the daslang field name

::

  struct AnnotatedConfig {
      name : string
      @optional debug : bool          // omitted when false
      @optional tags : array<string>  // omitted when empty
      @embed raw_data : string        // embedded as raw JSON
      @unescape raw_path : string     // no escaping of special chars
      pri : Priority                  // serialized as string
      @enum_as_int level : Priority   // serialized as integer
      @rename="type" _type : string   // appears as "type" in JSON
  }

  var c <- AnnotatedConfig(uninitialized
      name = "app", debug = false,
      raw_data = "[1,2,3]",
      raw_path = "C:\\Users\\test",
      pri = Priority.high,
      level = Priority.medium,
      _type = "widget"
  )
  let json_str = sprint_json(c, false)
  // {"name":"app","raw_data":[1,2,3],"raw_path":"C:\Users\test","pri":"high","level":1,"type":"widget"}

In this example: ``debug`` and ``tags`` are omitted (``@optional``),
``raw_data`` is embedded as ``[1,2,3]`` not ``"[1,2,3]"`` (``@embed``),
``raw_path`` keeps backslashes unescaped (``@unescape``), ``level``
is ``1`` instead of ``"medium"`` (``@enum_as_int``), and ``_type``
appears as ``"type"`` in the output (``@rename``).

@rename annotation
==================

Use ``@rename="json_key"`` when the JSON key is a daslang reserved word
or doesn't follow daslang naming conventions. The field keeps a safe name
in code (e.g. ``_type``) but serializes as the desired key. ``@rename``
works with ``sprint_json``, ``sscan_json``, ``JV``, and ``from_JV``::

  struct ApiResponse {
      @rename="type" _type : string
      @rename="class" _class : int
      value : float
  }

  var resp = ApiResponse(_type = "widget", _class = 3, value = 1.5)
  sprint_json(resp, false)
  // {"type":"widget","class":3,"value":1.5}

  // from_JV maps renamed keys back to struct fields
  var js = read_json("{\"type\":\"button\",\"class\":5,\"value\":2.0}", error)
  var result = from_JV(js, type<ApiResponse>)
  // result._type == "button", result._class == 5

sscan_json
==========

``sscan_json`` is the reverse of ``sprint_json`` — it parses a JSON string
directly into a struct using RTTI, without going through ``JsonValue?``.
It supports all the same types: structs, pointers, arrays, tables, tuples,
variants, enums, bitfields, vector types, and all scalar types.
It also respects ``@rename`` field annotations::

  struct Config {
      host : string
      port : int
      @rename _type : string
      tags : array<string>
  }

  var cfg : Config
  let ok = sscan_json("{\"host\":\"localhost\",\"port\":8080,\"type\":\"server\",\"tags\":[\"prod\",\"v2\"]}", cfg)
  // ok == true, cfg.host == "localhost", cfg.port == 8080
  // cfg._type == "server", cfg.tags == ["prod", "v2"]

``sscan_json`` returns ``true`` on success and ``false`` on malformed JSON.
Unknown keys are silently ignored, and missing fields keep their default values.

Round-trip with ``sprint_json``::

  var src = Player(name = "Hero", hp = 100, speed = 5.5)
  let json = sprint_json(src, false)
  var dst : Player
  sscan_json(json, dst)
  // dst.name == "Hero", dst.hp == 100

Class serialization
===================

Both ``JV``/``from_JV`` and ``sprint_json`` work with classes.
Classes serialize their fields just like structs::

  class Animal {
      species : string
      legs : int
  }

  var a = new Animal(species = "cat", legs = 4)
  let json_str = sprint_json(*a, false)
  // {"species":"cat","legs":4}

  var js = JV(*a)
  print(write_json(js))
  // {"legs":4,"species":"cat"}

.. seealso::

   Full source: :download:`tutorials/language/30_json.das <../../../../tutorials/language/30_json.das>`

   Next tutorial: :ref:`Regular expressions <tutorial_regex>`.

   :ref:`Functional programming tutorial <tutorial_functional>` (previous tutorial).

   :doc:`/stdlib/generated/json` — core JSON module reference.

   :doc:`/stdlib/generated/json_boost` — JSON boost module reference.
