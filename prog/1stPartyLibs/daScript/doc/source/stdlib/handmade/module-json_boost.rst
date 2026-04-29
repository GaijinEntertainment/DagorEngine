The JSON_BOOST module extends JSON support with operator overloads for convenient
field access (``?[]``), null-coalescing (``??``), and automatic struct-to-JSON
conversion macros (``from_JsValue``, ``to_JsValue``).

See also :doc:`json` for core JSON parsing and writing.
See :ref:`tutorial_json` for a hands-on tutorial.

All functions and symbols are in "json_boost" module, use require to get access to it.

.. code-block:: das

    require daslib/json_boost

Example:

.. code-block:: das

    require daslib/json_boost

        [export]
        def main() {
            let data = "\{ \"name\": \"Alice\", \"age\": 30 \}"
            var error = ""
            var js <- read_json(data, error)
            if (error == "") {
                let name = js?.name ?? "?"
                print("name = {name}\n")
                let age = js?.age ?? -1
                print("age = {age}\n")
            }
            unsafe {
                delete js
            }
        }
        // output:
        // name = Alice
        // age = 30

Field annotations
-----------------

Struct fields can carry annotations that control how ``JV`` / ``from_JV`` and
the builtin ``sprint_json`` serialize and deserialize them.  Annotations are
parsed by :ref:`parse_json_annotation <function-json_boost_parse_json_annotation_string_array_ls_tuple_ls_name_c_string;data_c_variant_ls_tBool_c_bool;tInt_c_int;tUInt_c_uint;tInt64_c_int64;tUInt64_c_uint64;tFloat_c_float;tDouble_c_double;tString_c_string;nothing_c_any_gr__gr__gr_>`
into a :ref:`JsonFieldState <struct-json_boost-JsonFieldState>` and stored in a
``static_let`` cache so each field is parsed only once.

``sprint_json`` requires ``options rtti`` for annotations to take effect at
runtime.

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Annotation
     - Effect
   * - ``@optional``
     - Skip the field when its value is default / empty (``0``, ``false``,
       empty string, empty array, empty table, null pointer).
   * - ``@rename="json_key"``
     - Use *json_key* instead of the daslang field name in JSON output and
       when looking up keys during ``from_JV`` deserialization.  The annotation
       value must be a string (``@rename="name"``).  A bare ``@rename`` with
       no string value is silently ignored.
   * - ``@embed``
     - Treat a ``string`` field as raw JSON — embed it without extra quoting.
       During ``JV`` conversion the string is parsed with ``read_json`` and
       the resulting sub-tree is inserted directly.
   * - ``@unescape``
     - Write the string field without escaping special characters
       (backslashes, quotes, etc.).
   * - ``@enum_as_int``
     - Serialize an enum field as its integer value instead of the
       enumeration name string.

Example with ``sprint_json``:

.. code-block:: das

    options rtti

    struct Config {
        name : string
        @optional debug : bool          // omitted when false
        @rename="type" _type : string   // JSON key is "type"
        @embed raw : string             // embedded as raw JSON
        @unescape path : string         // no escaping of backslashes
        @enum_as_int level : Priority   // integer, not string
    }

    let json_str = sprint_json(cfg, false)

See :ref:`tutorial_json` for runnable examples of every annotation.
