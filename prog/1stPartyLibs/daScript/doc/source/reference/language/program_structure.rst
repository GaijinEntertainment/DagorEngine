.. _program_structure:

=================
Program Structure
=================

.. index::
    single: Program Structure
    single: module
    single: require
    single: options

A daslang source file is a sequence of top-level declarations. This page describes the overall
layout of a file and the key declarations that control how it interacts with the rest of the program.

--------------------
File Layout
--------------------

A typical daslang file follows this layout:

.. code-block:: das

    options gen2                              // compilation options

    module my_module shared public            // module declaration

    require math                              // imports
    require daslib/strings_boost

    struct MyData                             // type declarations
        value : int
        name : string

    enum Color {
        red
        green
        blue
    }

    let MAX_COUNT = 100                       // global constants

    var total : int = 0                       // global variables

    typedef IntArray = array<int>             // type aliases

    def helper(x : int) : int {               // functions
        return x * 2
    }

    [export]                                  // entry point
    def main {
        print("hello\n")
    }

The parser does not enforce a strict ordering among ``options``, ``module``, and ``require``.
However, the ``module`` declaration must appear before any type declarations (structs, enums,
functions, global variables, type aliases). By convention, ``options`` lines come first,
followed by ``module``, then ``require``.

.. note::
    The order above is a convention, not a hard rule. The only enforced constraint is that
    ``module`` precedes type declarations.

--------------------
Module Declaration
--------------------

The ``module`` declaration names the current file's module:

.. code-block:: das

    module my_module

If omitted, the module name defaults to the file name (without extension).

^^^^^^^^^^^^^^^^^^^
Modifiers
^^^^^^^^^^^^^^^^^^^

The ``module`` declaration supports several modifiers:

``shared``
    Promotes the module to a built-in module. Only one instance is created per compilation
    environment, and it is shared across contexts:

    .. code-block:: das

        module my_lib shared

``public`` / ``private``
    Sets the default visibility of all declarations in the module. Functions, structs,
    enums, and globals inherit this default unless they specify their own visibility:

    .. code-block:: das

        module my_lib public          // all declarations are public by default
        module my_lib private         // all declarations are private by default

    If neither is specified, the module uses the environment's default (typically ``public``).

``inscope``
    Makes the module visible to all modules in the project without an explicit ``require``.
    This uses the ``!inscope`` syntax:

    .. code-block:: das

        module my_lib !inscope

Modifiers can be combined:

.. code-block:: das

    module my_lib shared public

--------------------
Require Declaration
--------------------

The ``require`` declaration imports another module:

.. code-block:: das

    require math
    require daslib/ast_boost

Module names can contain ``/`` and ``.`` separators. The project is responsible for
resolving module names into file paths.

^^^^^^^^^^^^^^^^^^^
Re-exporting
^^^^^^^^^^^^^^^^^^^

By default, required modules are private — they are only visible within the current module.
The ``public`` modifier re-exports the module, making it transitively visible to any module
that requires the current one:

.. code-block:: das

    require dastest/testing_boost public

^^^^^^^^^^^^^^^^^^^
Aliasing
^^^^^^^^^^^^^^^^^^^

When two modules share the same name, the ``as`` keyword provides a local alias:

.. code-block:: das

    require event
    require sub/event as sub_event

    def handle {
        sub_event::process()          // qualified call using the alias
    }

(see :ref:`Modules <modules>` for details on module function visibility and the ``_`` / ``__``
module prefixes).

--------------------
Options Declaration
--------------------

The ``options`` declaration sets compiler options for the file:

.. code-block:: das

    options gen2
    options no_unused_block_arguments = false

Multiple options can appear on one line, separated by commas:

.. code-block:: das

    options no_aot = true, rtti = true

A bare option name (without ``= value``) is shorthand for ``= true``:

.. code-block:: das

    options gen2          // equivalent to: options gen2 = true

(see :ref:`Options <options>` for the complete list of recognized options).

-----------------------
Top-Level Declarations
-----------------------

After the header declarations, the rest of the file consists of:

- **Type aliases** — ``typedef``, named ``tuple``, ``variant``, ``bitfield``
  (see :ref:`Type Aliases <aliases>`)
- **Enumerations** — ``enum`` (see :ref:`Constants and Enumerations <constants_and_enumerations>`)
- **Structures and classes** — ``struct``, ``class``
  (see :ref:`Structs <structs>`, :ref:`Classes <classes>`)
- **Global variables** — ``let`` (constant) and ``var`` (mutable)
  (see :ref:`Constants and Enumerations <constants_and_enumerations>`)
- **Functions** — ``def`` (see :ref:`Functions <functions>`)
- **Top-level reader macros** — ``%macro_name~...~``

All of these are peers in the grammar and can appear in any order, interleaved freely.

--------------------
Visibility
--------------------

Each top-level declaration can be marked ``public`` or ``private``:

.. code-block:: das

    def public helper(x : int) : int {     // visible to other modules
        return x * 2
    }

    struct private Internal {               // only visible within this module
        data : int
    }

If no visibility is specified, the declaration inherits the module's default visibility.

Shared global variables use the ``shared`` keyword and are shared across cloned contexts:

.. code-block:: das

    let shared GLOBAL_TABLE : table<string; int>

--------------------
Entry Points
--------------------

A daslang program is compiled and simulated by the host application (a C++ executable).
The host decides which functions to call. Several annotations mark functions with special roles.

^^^^^^^^^^^^^^^^^^^
[export]
^^^^^^^^^^^^^^^^^^^

Marks a function as callable from the host application. The host invokes exported functions
by name through the context API:

.. code-block:: das

    [export]
    def main {
        print("hello world\n")
    }

There is nothing special about the name ``main`` — it is purely a convention. The host chooses
which exported function(s) to call and in what order.

^^^^^^^^^^^^^^^^^^^
[init]
^^^^^^^^^^^^^^^^^^^

Marks a function to run automatically during context initialization. ``[init]`` functions
cannot have arguments and cannot return a value:

.. code-block:: das

    [init]
    def setup {
        print("initializing\n")
    }

Multiple ``[init]`` functions execute in declaration order. Ordering can be controlled with
attributes:

.. code-block:: das

    [init(tag="db")]
    def init_database {
        pass
    }

    [init(after="db")]
    def init_cache {
        pass
    }

    [init(before="db")]
    def init_logging {
        pass
    }

The option ``no_init`` disables all ``[init]`` functions.

^^^^^^^^^^^^^^^^^^^
[finalize]
^^^^^^^^^^^^^^^^^^^

Marks a function to run automatically during context shutdown. Same constraints as ``[init]`` —
no arguments, no return value:

.. code-block:: das

    [finalize]
    def cleanup {
        print("shutting down\n")
    }

--------------------
Expect Declaration
--------------------

The ``expect`` declaration is used in test files to declare expected compilation errors.
When present, the compiler treats the listed errors as intentional — the file compiles
"successfully" only if exactly those errors (and no others) are produced.

This is primarily used in negative test suites to verify that the compiler correctly
rejects invalid code:

.. code-block:: das

    expect 40214:3              // expect error 40214 exactly 3 times
    expect 30304, 30101         // expect each error once (count defaults to 1)

The syntax is:

.. code-block:: das

    expect <error_code> [: <count>] [, <error_code> [: <count>] ...]

Multiple ``expect`` declarations can appear in the same file. Error codes are numeric
identifiers organized by compilation phase:

+------------------+--------------------------------------------+
| Range            | Category                                   |
+==================+============================================+
| ``10001–10011``  | Lexer errors (mismatched brackets, etc.)   |
+------------------+--------------------------------------------+
| ``20000–20001``  | Parser errors (syntax errors)              |
+------------------+--------------------------------------------+
| ``30101–30128``  | Semantic: invalid type/annotation/name     |
+------------------+--------------------------------------------+
| ``30201–30213``  | Semantic: already declared / too many args |
+------------------+--------------------------------------------+
| ``30301–30311``  | Semantic: not found (type, func, var, etc.)|
+------------------+--------------------------------------------+
| ``30401–30403``  | Semantic: can't initialize                 |
+------------------+--------------------------------------------+
| ``30501–30509``  | Semantic: can't dereference/copy/move      |
+------------------+--------------------------------------------+
| ``30601–30602``  | Semantic: condition errors                 |
+------------------+--------------------------------------------+
| ``31300``        | Unsafe operation outside ``unsafe`` block  |
+------------------+--------------------------------------------+
| ``39901–39903``  | Semantic: missing value/typeinfo           |
+------------------+--------------------------------------------+
| ``40101–40214``  | Lint-time errors and warnings              |
+------------------+--------------------------------------------+

For example, a test that verifies the compiler rejects copying an array:

.. code-block:: das

    expect 30507    // cant_copy

    [export]
    def main {
        var a <- [1, 2, 3]
        var b = a           // error: can't copy array
    }

--------------------
Program vs. Module
--------------------

The same file format is used for both programs and modules. The distinction is how the file
is used:

**Program (entry point)**
    The top-level file compiled by the host application. Typically contains ``[export]`` functions.
    May omit the ``module`` declaration.

**Module (library)**
    A file imported via ``require`` by other files. Typically has a ``module`` declaration
    and provides types, functions, and globals for reuse.

A file can serve both roles simultaneously.

--------------------
Execution Lifecycle
--------------------

1. The host compiles a source file into a ``Program``
2. The program is simulated into a ``Context``
3. The context is initialized:

   - Global variables are initialized in declaration order, per module
   - ``[init]`` functions run in declaration order (or topologically, if ordering attributes are used)

4. The host calls ``[export]`` functions as needed
5. The context is shut down:

   - ``[finalize]`` functions run

--------------------
Complete Example
--------------------

The following example shows a complete program with all structural elements:

.. code-block:: das

    options gen2

    require math
    require daslib/strings_boost

    struct Particle {
        pos : float3
        vel : float3
        life : float
    }

    enum State {
        alive
        dead
    }

    let GRAVITY = float3(0.0, -9.8, 0.0)

    var particles : array<Particle>

    def update_particle(var p : Particle; dt : float) : State {
        p.vel += GRAVITY * dt
        p.pos += p.vel * dt
        p.life -= dt
        if (p.life > 0.0) {
            return State.alive
        }
        return State.dead
    }

    [init]
    def setup {
        for (i in range(100)) {
            particles |> push(Particle(pos=float3(0), vel=float3(0, 10.0, 0), life=5.0))
        }
    }

    [export]
    def main {
        let dt = 0.016
        for (p in particles) {
            update_particle(p, dt)
        }
        print("particles: {length(particles)}\n")
    }

    [finalize]
    def cleanup {
        unsafe {
            delete particles
        }
    }

Expected output:

.. code-block:: text

    particles: 100

.. seealso::

    :ref:`Modules <modules>` for module declaration and ``require`` semantics,
    :ref:`Options <options>` for compiler and runtime options,
    :ref:`Annotations <annotations>` for ``[init]``, ``[finalize]``, and ``[export]``,
    :ref:`Contexts <contexts>` for the execution context lifecycle,
    :ref:`Constants and enumerations <constants_and_enumerations>` for global declarations.
