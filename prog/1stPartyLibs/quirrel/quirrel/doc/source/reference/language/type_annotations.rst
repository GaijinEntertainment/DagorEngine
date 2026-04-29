.. _type_annotations:


Type Annotations in Quirrel
===========================

Quirrel extends the Squirrel language with optional static type annotations that are enforced at runtime. Type checks occur during function calls, return statements, assignments, and destructuring operations, providing safety without requiring a separate compilation step.

Basic Type Syntax
-----------------

Primitive Types
~~~~~~~~~~~~~~~

Quirrel supports the following built-in types:

- ``int`` - Signed integer
- ``float`` - Floating-point number
- ``number`` - Union of ``int | float`` (numeric values)
- ``bool`` - Boolean values (``true`` / ``false``)
- ``string`` - Immutable string values
- ``null`` - Null value type

Complex Types
~~~~~~~~~~~~~

- ``table`` - Associative array / object
- ``array`` - Ordered sequence container
- ``function`` - Callable function or closure
- ``userdata`` - Opaque C/C++ data reference
- ``generator`` - Coroutine/generator object
- ``userpointer`` - Raw pointer value
- ``thread`` - Execution thread
- ``instance`` - Class instance
- ``class`` - Class definition object
- ``weakref`` - Weak reference to an object
- ``any`` - Accepts any type (opt-out of type checking)

Type Unions
~~~~~~~~~~~

Combine multiple types using the pipe operator (``|``):

.. code-block:: quirrel

    local value: int|float = 42
    local maybeString: string|null = null
    local numeric: number = 3.14  // equivalent to int|float

Parentheses may be used for clarity in complex unions:

.. code-block:: quirrel

    local x: (int | null) = null
    local y: (string | table | null) = {}

Function Type Annotations
-------------------------

Parameters and Return Types
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Annotate parameters and return values with type specifiers:

.. code-block:: quirrel

    function add(x: int, y: int): int {
        return x + y
    }

    function toString(value: any): string {
        return value.tostring()
    }

Nullable Parameters with Defaults
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Combine unions with default values for optional parameters:

.. code-block:: quirrel

    function greet(name: string|null = null): string {
        return name ? "Hello " + name : "Hello guest"
    }

Vararg Type Annotations
~~~~~~~~~~~~~~~~~~~~~~~

Specify the expected type for variable arguments using ``...``:

.. code-block:: quirrel

    function sum(first: number, ...: number): number {
        local total = first
        foreach (v in vargv) total += v
        return total
    }

Lambda/Anonymous Functions
~~~~~~~~~~~~~~~~~~~~~~~~~~

Type annotations apply to lambdas using the ``@`` syntax:

.. code-block:: quirrel

    let multiply = @(x: int, y: int): int x * y

Variable Type Annotations
-------------------------

Local and Let Bindings
~~~~~~~~~~~~~~~~~~~~~~

Annotate variables at declaration site:

.. code-block:: quirrel

    local count: int = 0
    let pi: float = 3.14159
    local config: table|array = {timeout = 30}

Type checks occur on assignment:

.. code-block:: quirrel

    local id: int = "not a number"  // Runtime type error

Destructuring with Types
------------------------

Object Destructuring
~~~~~~~~~~~~~~~~~~~~

Annotate individual properties during destructuring:

.. code-block:: quirrel

    local { name: string, age: int|null = 25 } = userData

    // With explicit type unions
    local { x: int, y: string|int|null } = point

Array Destructuring
~~~~~~~~~~~~~~~~~~~

Specify types for array elements:

.. code-block:: quirrel

    local [first: int, second: string] = [42, "hello"]

    // Mixed annotated/unannotated elements
    local [head: int, value1, value2] = [1, 2, 3]

    // Complex unions in arrays
    local [value: (int | null), flag: bool] = [null, true]

Nested Destructuring in Parameters
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Functions can destructure parameters with full type annotations:

.. code-block:: quirrel

    function createUser([name: string, age: int], {active: bool = true}) {
        // ...
    }

    function process(
        [handler: function|null = null, config: table],
        {timeout: int|float = 5.0},
        ...
    ) {
        // ...
    }

Runtime Type Checking Behavior
------------------------------

Quirrel performs **dynamic type validation** at these points:

1. **Function calls** - Parameters are checked against declared types before execution
2. **Return statements** - Return values are validated against the function's return type
3. **Assignments** - Values are checked when assigned to typed variables
4. **Destructuring** - Extracted values are validated against property/element types

All checks occur at runtime with immediate exceptions on mismatch:

.. code-block:: quirrel

    function strict(x: int): int { return x }

    strict("wrong")  // Throws: type mismatch in parameter 'x'


Best Practices
--------------

- Type checks add minor runtime overhead; avoid in ultra-hot loops if unneeded
- Use ``number`` instead of ``int|float`` for numeric parameters accepting both types
- Use ``any`` sparingly—only when truly necessary for dynamic behavior
- Combine defaults with nullable types for ergonomic optional parameters:

  .. code-block:: quirrel

      function fetch(url: string, timeout: float|null = 30.0) { ... }
