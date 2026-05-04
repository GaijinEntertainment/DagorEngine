.. _aliases:

============
Type Aliases
============

.. index::
    single: Type Aliases
    single: typedef
    single: alias

Type aliases allow you to give a new name to an existing type. This improves code readability and
makes it easier to work with complex type declarations. Aliases are fully interchangeable with the
types they refer to — they do not create new types.

-------------------
typedef Declaration
-------------------

A type alias is declared with the ``typedef`` keyword:

.. code-block:: das

    typedef Vec3 = float3
    typedef StringArray = array<string>
    typedef Callback = function<(x:int; y:int):bool>

Once defined, the alias can be used anywhere a type is expected:

.. code-block:: das

    var position : Vec3
    var names : StringArray
    var cb : Callback

Aliases can refer to complex types, including tables, arrays, tuples, variants, blocks, and lambdas:

.. code-block:: das

    typedef IntPair = tuple<int; int>
    typedef Registry = table<string; array<int>>
    typedef Transform = block<(pos:float3; vel:float3):float3>

-------------------
Publicity
-------------------

Type aliases can be ``public`` or ``private``:

.. code-block:: das

    typedef public  Vec3 = float3       // visible to other modules
    typedef private Internal = int      // only visible within this module

If no publicity is specified, the alias inherits the module's default publicity
(i.e., in public modules aliases are public, and in private modules they are private).

-------------------------------
Shorthand Type Alias Syntax
-------------------------------

Several composite types support a shorthand declaration syntax that creates a named type alias.

^^^^^^^^^^^^^^^^^
Tuple Aliases
^^^^^^^^^^^^^^^^^

Instead of writing a ``typedef`` for a tuple, you can use the ``tuple`` keyword directly:

.. code-block:: das

    tuple Vertex {
        position : float3
        normal   : float3
        uv       : float2
    }

This is equivalent to:

.. code-block:: das

    typedef Vertex = tuple<position:float3; normal:float3; uv:float2>

(see :ref:`Tuples <tuples>`).

^^^^^^^^^^^^^^^^^^^
Variant Aliases
^^^^^^^^^^^^^^^^^^^

Variants support a similar shorthand:

.. code-block:: das

    variant Value {
        i : int
        f : float
        s : string
    }

This is equivalent to:

.. code-block:: das

    typedef Value = variant<i:int; f:float; s:string>

(see :ref:`Variants <variants>`).

^^^^^^^^^^^^^^^^^^^
Bitfield Aliases
^^^^^^^^^^^^^^^^^^^

Bitfields also support the shorthand syntax:

.. code-block:: das

    bitfield Permissions {
        read
        write
        execute
    }

This is equivalent to:

.. code-block:: das

    typedef Permissions = bitfield<read; write; execute>

Bitfield aliases can specify a storage type:

.. code-block:: das

    bitfield SmallFlags : uint8 {
        active
        visible
    }

(see :ref:`Bitfields <bitfields>`).

--------------------------
Local Type Aliases
--------------------------

Type aliases can be declared inside function bodies:

.. code-block:: das

    def process {
        typedef Entry = tuple<string; int>
        var data : Entry
        data = ("hello", 42)
    }

Local type aliases are scoped to the enclosing block and are not visible outside it.

Type aliases can also be declared inside structure or class bodies:

.. code-block:: das

    struct Container {
        typedef Element = int
        data : array<Element>
    }

-------------------------------------
Generic Type Aliases (auto)
-------------------------------------

In generic functions, the ``auto(Name)`` syntax creates inferred type aliases.
These are resolved during function instantiation based on the actual argument types:

.. code-block:: das

    def first_element(a : array<auto(ElementType)>) : ElementType {
        return a[0]
    }

    let x = first_element([1, 2, 3])    // ElementType is inferred as int, returns int

Named auto aliases can be reused across multiple arguments to enforce type relationships:

.. code-block:: das

    def add_to_array(var arr : array<auto(T)>; value : T) {
        arr |> push(value)
    }

(see :ref:`Generic Programming <generic_programming>`).

---------------------------------------------
Type Aliases with typedecl
---------------------------------------------

The ``typedecl`` expression can be used inside generic functions to create types based on the
type of an expression:

.. code-block:: das

    def make_table(key : auto(K)) {
        var result : table<typedecl(key); string>
        return <- result
    }

Here the key type of the table is inferred from the type of the ``key`` argument.

(see :ref:`Generic Programming <generic_programming>`).

.. seealso::

    :ref:`Datatypes <datatypes_and_values>` for the built-in types that can be aliased,
    :ref:`Tuples <tuples>` for tuple alias syntax,
    :ref:`Variants <variants>` for variant alias syntax,
    :ref:`Bitfields <bitfields>` for bitfield alias syntax.
