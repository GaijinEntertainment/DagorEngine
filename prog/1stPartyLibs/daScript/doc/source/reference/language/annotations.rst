.. _annotations:

===========
Annotations
===========

.. index::
    single: Annotations

Annotations are metadata decorators attached to functions, structures, classes, enumerations,
and variables. They control compiler behavior — export, initialization, safety, optimization,
and macro registration.

An annotation is written in square brackets before the declaration it applies to:

.. code-block:: das

    [export]
    def main {
        print("hello\n")
    }

Multiple annotations can be combined with commas:

.. code-block:: das

    [export, no_aot]
    def main {
        print("hello\n")
    }

Some annotations accept arguments:

.. code-block:: das

    [init(tag="db")]
    def init_database {
        pass
    }

    [unused_argument(x, y)]
    def foo(x, y : int) {
        pass
    }

---------------------
Function Annotations
---------------------

^^^^^^^^^^^^^^^^^^^^^
Lifecycle
^^^^^^^^^^^^^^^^^^^^^

``[export]``
    Marks a function as callable from the host application. The host invokes exported functions
    by name through the context API:

    .. code-block:: das

        [export]
        def main {
            print("hello\n")
        }

``[init]``
    Marks a function to run automatically during context initialization. The function must
    take no arguments and return ``void``:

    .. code-block:: das

        [init]
        def setup {
            pass
        }

    Ordering can be controlled with attributes:

    - ``tag`` — defines a named initialization pass
    - ``before`` — runs before the named pass
    - ``after`` — runs after the named pass

    All ordering attributes imply late initialization:

    .. code-block:: das

        [init(tag="db")]
        def init_database {
            pass
        }

        [init(after="db")]
        def init_cache {
            pass
        }

``[finalize]``
    Marks a function to run automatically during context shutdown. Same constraints as
    ``[init]`` — no arguments, no return value:

    .. code-block:: das

        [finalize]
        def cleanup {
            pass
        }

    Supports a ``late`` attribute for ordering.

``[run]``
    Marks a function to run at compile time:

    .. code-block:: das

        [run]
        def compile_time_check {
            print("compiling...\n")
        }

    Disabled by the ``disable_run`` option (see :ref:`Options <options>`).

(see :ref:`Program Structure <program_structure>` for the full initialization lifecycle).

^^^^^^^^^^^^^^^^^^^^^
Visibility
^^^^^^^^^^^^^^^^^^^^^

``[private]``
    Makes a function private to the current module. Equivalent to the ``private`` keyword
    before ``def``.

^^^^^^^^^^^^^^^^^^^^^
Safety
^^^^^^^^^^^^^^^^^^^^^

``[unsafe_deref]``
    Marks a function as allowing unsafe dereferences inside its body:

    .. code-block:: das

        [unsafe_deref]
        def read_ptr(p : int?) {
            return *p
        }

``[unsafe_operation]``
    Marks a function as an unsafe operation. Calling it requires an ``unsafe`` block:

    .. code-block:: das

        [unsafe_operation]
        def dangerous_thing {
            pass
        }

        unsafe {
            dangerous_thing()
        }

``[unsafe_outside_of_for]``
    Marks a function as unsafe when called outside of a ``for`` loop body.

^^^^^^^^^^^^^^^^^^^^^
Lint Control
^^^^^^^^^^^^^^^^^^^^^

``[unused_argument]``
    Suppresses "unused argument" warnings for specific arguments:

    .. code-block:: das

        [unused_argument(x)]
        def handler(x : int) {
            pass
        }

    Multiple arguments can be listed: ``[unused_argument(x, y)]``.

``[nodiscard]``
    Warns if the return value of the function is discarded:

    .. code-block:: das

        [nodiscard]
        def compute : int {
            return 42
        }

        compute()       // warning: return value discarded

``[deprecated]``
    Marks a function as deprecated. Produces a compile-time warning when called:

    .. code-block:: das

        [deprecated(message="use new_func instead")]
        def old_func {
            pass
        }

``[no_lint]``
    Disables all lint checks for this function.

``[sideeffects]``
    Declares that the function has side effects, even if the compiler cannot detect any.
    Prevents the optimizer from removing calls to this function.

^^^^^^^^^^^^^^^^^^^^^^
Generics and Contracts
^^^^^^^^^^^^^^^^^^^^^^

``[generic]``
    Marks a function as generic (a template that is instantiated for each unique set of
    argument types):

    .. code-block:: das

        [generic]
        def add(a, b : auto) {
            return a + b
        }

    (see :ref:`Generic Programming <generic_programming>`).

``[expect_ref]``
    Specialization contract: requires named arguments to be references:

    .. code-block:: das

        [expect_ref(arr)]
        def process(var arr : auto) {
            pass
        }

``[expect_dim]``
    Specialization contract: requires named arguments to be fixed-size arrays.

``[expect_any_vector]``
    Specialization contract: requires named arguments to be vector template types.

``[local_only]``
    Verifies that specific arguments are passed as local constructors. The argument value
    indicates the expected state — ``true`` means the argument must be a literal constructor,
    ``false`` means it must not be:

    .. code-block:: das

        [local_only(data=true)]
        def process(data : Foo) {
            pass
        }

Additional contract annotations are available in ``daslib/contracts``
(see `Contract Annotations (daslib)`_ below).

^^^^^^^^^^^^^^^^^^^^^
Optimization and AOT
^^^^^^^^^^^^^^^^^^^^^

``[no_aot]``
    Disables AOT (ahead-of-time) compilation for this function.

``[no_jit]``
    Disables JIT compilation for this function.

``[jit]``
    Requests JIT compilation for this function.

``[hybrid]``
    Marks a function as an AOT hybrid — it can call interpreted code from AOT context.

``[alias_cmres]``
    Allows aliasing of the copy-on-move return value.

``[never_alias_cmres]``
    Prevents aliasing of the copy-on-move return value.

``[pinvoke]``
    Marks a function for platform invoke (external function call).

``[type_function]``
    Registers the function as usable in type expressions.

^^^^^^^^^^^^^^^^^^^^^
Macros
^^^^^^^^^^^^^^^^^^^^^

``[_macro]``
    Marks a function as macro initialization code (runs during macro compilation pass).

``[macro_function]``
    Marks a function as existing only in the macro context — excluded from the regular
    program.

``[macro]``
    Defined in ``daslib/ast_boost``. Like ``[_macro]`` but wraps the function body in a
    module-ready check. Requires ``require daslib/ast_boost``:

    .. code-block:: das

        require daslib/ast_boost

        [macro]
        def setup_macros {
            print("registering macros\n")
        }

``[tag_function]``
    Defined in ``daslib/ast_boost``. Tags a function with string tags for retrieval via
    ``for_each_tag_function``:

    .. code-block:: das

        [tag_function(my_tag)]
        def tagged_func {
            pass
        }

^^^^^^^^^^^^^^^^^^^^^
Markers and Hints
^^^^^^^^^^^^^^^^^^^^^

``[hint]``
    A dummy annotation that carries key-value arguments for optimization hints. Does not
    change behavior by itself.

``[marker]``
    A generic function marker annotation. Does not change behavior.

-------------------------------
Structure and Class Annotations
-------------------------------

``[cpp_layout]``
    Uses C++ memory layout (matching C++ struct alignment rules):

    .. code-block:: das

        [cpp_layout]
        struct CppInterop {
            x : int
            y : float
        }

    Pass ``pod=false`` to allow non-POD layouts: ``[cpp_layout(pod=false)]``.

``[safe_when_uninitialized]``
    Marks the struct as safe even when fields are uninitialized (zero-filled memory is
    a valid state):

    .. code-block:: das

        [safe_when_uninitialized]
        struct Vec2 {
            x : float
            y : float
        }

``[persistent]``
    Makes a structure persistent (survives context reset). All fields must be POD unless
    ``non_pod=true`` is specified:

    .. code-block:: das

        [persistent]
        struct Config {
            value : int
        }

``[no_default_initializer]``
    Suppresses generation of the default constructor for this structure.

``[macro_interface]``
    Marks a structure as a macro interface (for the macro system).

``[comment]``
    A dummy annotation for attaching comment metadata to a structure.

``[tag_structure]``
    Defined in ``daslib/ast_boost``. Tags a structure with string tags for later retrieval.

------------------------------------------
Macro Registration Annotations (daslib)
------------------------------------------

These annotations are defined in ``daslib/ast_boost`` and applied to ``class`` declarations that
inherit from the appropriate AST base class. They auto-register the class as a macro during
module compilation.

All accept an optional ``name`` argument. If omitted, the class name is used.

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Annotation
     - Base class
     - Purpose
   * - ``[function_macro]``
     - ``AstFunctionAnnotation``
     - Custom function decorator
   * - ``[block_macro]``
     - ``AstBlockAnnotation``
     - Custom block decorator
   * - ``[structure_macro]``
     - ``AstStructureAnnotation``
     - Custom struct/class decorator
   * - ``[enumeration_macro]``
     - ``AstEnumerationAnnotation``
     - Custom enum decorator
   * - ``[contract]``
     - ``AstFunctionAnnotation``
     - Specialization constraint
   * - ``[reader_macro]``
     - ``AstReaderMacro``
     - Custom literal/expression reader
   * - ``[comment_reader]``
     - ``AstCommentReader``
     - Custom comment reader
   * - ``[call_macro]``
     - ``AstCallMacro``
     - Intercepts function-like calls
   * - ``[typeinfo_macro]``
     - ``AstTypeInfoMacro``
     - Custom ``typeinfo(...)`` handler
   * - ``[variant_macro]``
     - ``AstVariantMacro``
     - Custom variant type processing
   * - ``[for_loop_macro]``
     - ``AstForLoopMacro``
     - Custom for-loop behavior
   * - ``[capture_macro]``
     - ``AstCaptureMacro``
     - Custom closure capture handling
   * - ``[type_macro]``
     - ``AstTypeMacro``
     - Custom type expression processing
   * - ``[simulate_macro]``
     - ``AstSimulateMacro``
     - Custom simulation node generation
   * - ``[infer_macro]``
     - ``AstInferMacro``
     - Runs during type inference
   * - ``[dirty_infer_macro]``
     - ``AstDirtyInferMacro``
     - Runs during dirty inference passes
   * - ``[optimization_macro]``
     - ``AstOptimizationMacro``
     - Runs during optimization
   * - ``[lint_macro]``
     - ``AstLintMacro``
     - Runs during linting
   * - ``[global_lint_macro]``
     - ``AstGlobalLintMacro``
     - Runs after all modules are compiled

Example:

.. code-block:: das

    require daslib/ast_boost

    [function_macro(name="my_decorator")]
    class MyDecorator : AstFunctionAnnotation {
        def override apply(var func : FunctionPtr; var group : ModuleGroup;
                           args : AnnotationArgumentList; var errors : das_string) : bool {
            print("decorating {func.name}\n")
            return true
        }
    }

(see :ref:`Macros <macros>` for details on writing macros).

------------------------------------------
Contract Annotations (daslib)
------------------------------------------

These annotations are defined in ``daslib/contracts`` and used as specialization constraints
on generic function arguments. Each accepts one or more argument names to constrain.

Requires ``require daslib/contracts``.

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Annotation
     - Constraint
   * - ``[expect_any_array(arg)]``
     - Argument must be a dynamic array
   * - ``[expect_any_enum(arg)]``
     - Argument must be an enum
   * - ``[expect_any_bitfield(arg)]``
     - Argument must be a bitfield
   * - ``[expect_any_vector_type(arg)]``
     - Argument must be a vector template type
   * - ``[expect_any_struct(arg)]``
     - Argument must be a struct
   * - ``[expect_any_numeric(arg)]``
     - Argument must be a numeric type
   * - ``[expect_any_workhorse(arg)]``
     - Argument must be a "workhorse" type (int, float, etc.)
   * - ``[expect_any_tuple(arg)]``
     - Argument must be a tuple
   * - ``[expect_any_variant(arg)]``
     - Argument must be a variant
   * - ``[expect_any_function(arg)]``
     - Argument must be a function type
   * - ``[expect_any_lambda(arg)]``
     - Argument must be a lambda
   * - ``[expect_ref(arg)]``
     - Argument must be a reference
   * - ``[expect_pointer(arg)]``
     - Argument must be a pointer
   * - ``[expect_class(arg)]``
     - Argument must be a class pointer
   * - ``[expect_value_handle(arg)]``
     - Argument must be a value handle type

Example:

.. code-block:: das

    require daslib/contracts

    [expect_any_array(arr)]
    def first_element(arr : auto) {
        return arr[0]
    }

------------------------------------------
Annotation Syntax Details
------------------------------------------

Annotations can be combined with logical operators for contract composition:

.. code-block:: das

    [expect_ref(a) && expect_dim(b)]
    def process(var a : auto; b : auto) {
        pass
    }

Negation is also supported:

.. code-block:: das

    [!expect_ref(a)]
    def no_ref(a : auto) {
        pass
    }

Annotations on struct/class fields appear before the field name in the ``@`` metadata syntax:

.. code-block:: das

    class Foo {
        @big
        @min = 13
        @max = 42
        value : int
    }

These ``@`` decorators attach metadata to the field. They are accessible via ``typeinfo`` and
at compile time in macros.

