.. _options:

=======
Options
=======

.. index::
    single: Options

The ``options`` statement sets compilation options for the current file. Options control
compiler behavior — linting, optimization, logging, memory limits, and language features.

Syntax:

.. code-block:: das

    options name = value
    options name                    // shorthand for name = true
    options a = true, b = false     // multiple options, comma-separated

Option values can be ``bool``, ``int``, or ``string``, depending on the option.

Multiple ``options`` statements are allowed per file. They can appear before or after
``module`` and ``require`` declarations. By convention they are placed at the top of the file:

.. code-block:: das

    options gen2
    options no_unused_block_arguments = false

    module my_module shared public

    require daslib/strings_boost

.. note::
    The host application can restrict which options are allowed in which files via the
    ``isOptionAllowed`` method on the project's file access object.

--------------------
Language and Syntax
--------------------

.. list-table::
   :header-rows: 1
   :widths: 30 10 10 50

   * - Option
     - Type
     - Default
     - Description
   * - ``gen2``
     - bool
     - false
     - Enables generation-2 syntax. Requires ``{ }`` braces for all blocks, ``;`` semicolons,
       and disables legacy make syntax (``[[...]]`` for structs, ``[{...}]`` for arrays).
       Applied immediately during parsing.
   * - ``indenting``
     - int
     - 0
     - Tab size for indentation-sensitive parsing. Valid values are ``0`` (disabled), ``2``, ``4``, ``8``.
       Applied immediately during parsing.
   * - ``always_export_initializer``
     - bool
     - false
     - Always exports initializer functions.
   * - ``infer_time_folding``
     - bool
     - true
     - Enables constant folding during type inference.
   * - ``disable_run``
     - bool
     - false
     - Disables compile-time execution (the ``[run]`` annotation has no effect).

--------------------
Lint Control
--------------------

.. list-table::
   :header-rows: 1
   :widths: 30 10 10 50

   * - Option
     - Type
     - Default
     - Description
   * - ``lint``
     - bool
     - true
     - Enables or disables all lint checks for this module.
   * - ``no_writing_to_nameless``
     - bool
     - true
     - Disallows writing to nameless (intermediate) variables on the stack.
   * - ``always_call_super``
     - bool
     - false
     - Requires ``super()`` to be called in every class constructor.
   * - ``no_unused_function_arguments``
     - bool
     - false
     - Reports unused function arguments as errors.
   * - ``no_unused_block_arguments``
     - bool
     - false
     - Reports unused block arguments as errors.
   * - ``no_deprecated``
     - bool
     - false
     - Reports use of deprecated features as errors.
   * - ``no_aliasing``
     - bool
     - false
     - Reports aliasing as errors. When ``false``, aliasing only disables certain optimizations.
   * - ``strict_smart_pointers``
     - bool
     - true
     - Enables stricter safety checks for smart pointers.
   * - ``strict_properties``
     - bool
     - false
     - Enables strict property semantics. When ``true``, ``a.prop = b`` is not promoted to
       ``a.prop := b``.
   * - ``report_invisible_functions``
     - bool
     - true
     - Reports functions that are not visible from the current module.
   * - ``report_private_functions``
     - bool
     - true
     - Reports functions that are inaccessible due to private module visibility.

--------------------
Optimization
--------------------

.. list-table::
   :header-rows: 1
   :widths: 30 10 10 50

   * - Option
     - Type
     - Default
     - Description
   * - ``optimize``
     - bool
     - true
     - Enables compiler optimizations.
   * - ``no_optimization``
     - bool
     - false
     - Disables compiler optimizations (inverse of ``optimize``).
   * - ``no_optimizations``
     - bool
     - false
     - Disables all optimizations unconditionally. Overrides all other optimization settings.
   * - ``fusion``
     - bool
     - true
     - Enables simulation node fusion for faster execution.
   * - ``remove_unused_symbols``
     - bool
     - true
     - Removes unused symbols from the compiled program. Must be ``false`` for modules
       compiled for AOT, otherwise the AOT tool will not find all necessary symbols.
   * - ``no_fast_call``
     - bool
     - false
     - Disables the fastcall optimization.

--------------------
Memory
--------------------

.. list-table::
   :header-rows: 1
   :widths: 30 10 10 50

   * - Option
     - Type
     - Default
     - Description
   * - ``stack``
     - int
     - 16384
     - Stack size in bytes. Set to ``0`` for a unique stack per context.
   * - ``heap_size_hint``
     - int
     - 65536
     - Initial heap size hint in bytes.
   * - ``string_heap_size_hint``
     - int
     - 65536
     - Initial string heap size hint in bytes.
   * - ``heap_size_limit``
     - int
     - 0
     - Maximum heap allocation in bytes. ``0`` means unlimited.
   * - ``string_heap_size_limit``
     - int
     - 0
     - Maximum string heap allocation in bytes. ``0`` means unlimited.
   * - ``persistent_heap``
     - bool
     - false
     - Uses a persistent (non-releasing) heap. Old allocations are never freed — useful
       with garbage collection.
   * - ``gc``
     - bool
     - false
     - Enables garbage collection for the context.
   * - ``intern_strings``
     - bool
     - false
     - Enables string interning lookup for the regular string heap.
   * - ``very_safe_context``
     - bool
     - false
     - Context does not release old memory from array or table growth (leaves it to the GC).

--------------------
AOT
--------------------

.. list-table::
   :header-rows: 1
   :widths: 30 10 10 50

   * - Option
     - Type
     - Default
     - Description
   * - ``no_aot``
     - bool
     - false
     - Disables AOT (ahead-of-time) compilation for this module.
   * - ``aot_prologue``
     - bool
     - false
     - Generates AOT prologue code (enables AOT export even when not strictly required).
   * - ``aot_lib``
     - bool
     - false
     - Compiles module in AOT library mode.
   * - ``standalone_context``
     - bool
     - false
     - Generates a standalone context class in AOT mode.
   * - ``only_fast_aot``
     - bool
     - false
     - Only allows fast AOT functions.
   * - ``aot_order_side_effects``
     - bool
     - false
     - Orders AOT side effects for deterministic evaluation.

----------------------
Safety and Strictness
----------------------

.. list-table::
   :header-rows: 1
   :widths: 30 10 10 50

   * - Option
     - Type
     - Default
     - Description
   * - ``no_init``
     - bool
     - false
     - Disallows ``[init]`` functions in any form.
   * - ``no_global_variables``
     - bool
     - false
     - Disallows module-level global variables (shared variables are still allowed).
   * - ``no_global_variables_at_all``
     - bool
     - false
     - Disallows all global variables, including shared ones.
   * - ``no_global_heap``
     - bool
     - false
     - Disallows global heap allocation.
   * - ``no_unsafe_uninitialized_structures``
     - bool
     - true
     - Disallows ``unsafe``-ly uninitialized structures.
   * - ``unsafe_table_lookup``
     - bool
     - false
     - Makes table lookup (``tab[key]``) an unsafe operation. When ``false`` (default),
       table lookups are safe but the compiler still detects dangerous patterns where
       the same table is indexed multiple times in a single expression
       (``table_lookup_collision`` lint error).
   * - ``relaxed_pointer_const``
     - bool
     - false
     - Relaxes const correctness on pointers.
   * - ``relaxed_assign``
     - bool
     - true
     - Allows the ``=`` operator to be silently promoted to ``<-`` when the right-hand side
       is a temporary value or function call result and the type cannot be copied.
       See :ref:`Move, Copy, and Clone <move_copy_clone>`.
   * - ``strict_unsafe_delete``
     - bool
     - false
     - Makes ``delete`` of types that contain ``unsafe`` delete an unsafe operation.
   * - ``no_local_class_members``
     - bool
     - true
     - Struct and class members cannot be class types.

--------------------
Multiple Contexts
--------------------

.. list-table::
   :header-rows: 1
   :widths: 30 10 10 50

   * - Option
     - Type
     - Default
     - Description
   * - ``multiple_contexts``
     - bool
     - false
     - Indicates that the code supports multiple context safety.
   * - ``solid_context``
     - bool
     - false
     - All variable and function lookup is context-dependent (via index). Slightly faster,
       but prohibits AOT and patches.

------------------------
Debugging and Profiling
------------------------

.. list-table::
   :header-rows: 1
   :widths: 30 10 10 50

   * - Option
     - Type
     - Default
     - Description
   * - ``debugger``
     - bool
     - false
     - Enables debugger support. Disables fastcall and adds a context mutex.
   * - ``profiler``
     - bool
     - false
     - Enables profiler support. Disables fastcall.
   * - ``debug_infer_flag``
     - bool
     - false
     - Enables debugging of macro "not_inferred" issues.
   * - ``threadlock_context``
     - bool
     - false
     - Adds a mutex to the context for thread-safe access.
   * - ``log_compile_time``
     - bool
     - false
     - Prints compile time at the end of compilation.
   * - ``log_total_compile_time``
     - bool
     - false
     - Prints detailed compile time breakdown at the end of compilation.

--------------------
RTTI
--------------------

.. list-table::
   :header-rows: 1
   :widths: 30 10 10 50

   * - Option
     - Type
     - Default
     - Description
   * - ``rtti``
     - bool
     - false
     - Creates extended RTTI (runtime type information). Required for reflection, ``typeinfo``,
       and the ``rtti`` module.

--------------------
Logging
--------------------

The following options control diagnostic output during compilation. All are ``bool`` type,
default ``false``.

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Option
     - Description
   * - ``log``
     - Logs the program AST after compilation.
   * - ``log_optimization_passes``
     - Logs each optimization pass.
   * - ``log_optimization``
     - Logs optimization results.
   * - ``log_stack``
     - Logs stack allocation.
   * - ``log_init``
     - Logs initialization.
   * - ``log_symbol_use``
     - Logs symbol usage information.
   * - ``log_var_scope``
     - Logs variable scoping.
   * - ``log_nodes``
     - Logs simulation nodes.
   * - ``log_nodes_aot_hash``
     - Logs AOT hash values for simulation nodes.
   * - ``log_mem``
     - Logs memory usage after simulation.
   * - ``log_debug_mem``
     - Logs debug memory information.
   * - ``log_cpp``
     - Logs C++ representation.
   * - ``log_aot``
     - Logs AOT output.
   * - ``log_infer_passes``
     - Logs type inference passes.
   * - ``log_require``
     - Logs module resolution.
   * - ``log_generics``
     - Logs generic function instantiation.
   * - ``log_mn_hash``
     - Logs mangled name hashes.
   * - ``log_gmn_hash``
     - Logs global mangled name hashes.
   * - ``log_ad_hash``
     - Logs annotation data hashes.
   * - ``log_aliasing``
     - Logs aliasing analysis.
   * - ``log_inscope_pod``
     - Logs inscope analysis for POD-like types.

--------------------
Print Control
--------------------

The following options affect how the AST is printed (for ``log`` output). All are ``bool`` type,
default ``false``.

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Option
     - Description
   * - ``print_ref``
     - Prints reference information in AST output.
   * - ``print_var_access``
     - Prints variable access flags in AST output.
   * - ``print_c_style``
     - Uses C-style formatting for AST output.
   * - ``print_use``
     - Prints symbol usage flags.

--------------------
Miscellaneous
--------------------

.. list-table::
   :header-rows: 1
   :widths: 30 10 10 50

   * - Option
     - Type
     - Default
     - Description
   * - ``max_infer_passes``
     - int
     - 50
     - Maximum number of type inference passes before the compiler gives up.
   * - ``scoped_stack_allocator``
     - bool
     - true
     - Reuses stack memory after variables go out of scope.
   * - ``force_inscope_pod``
     - bool
     - false
     - Forces inscope lifetime semantics for POD-like types.
   * - ``keep_alive``
     - bool
     - false
     - Produces keep-alive nodes in simulation.
   * - ``serialize_main_module``
     - bool
     - true
     - Serializes the main module instead of recompiling it each time.

--------------------------
Module-Registered Options
--------------------------

Any module can register custom options via its ``getOptionType`` method.
These are validated during lint alongside the built-in options.
If a module defines its own option, any file that requires that module can use it.

If you set an option that is not recognized by either the built-in list, the policy list, or any
loaded module, the compiler reports:

.. code-block:: text

    invalid option 'unknown_name'

If the option exists but you provide the wrong value type, the compiler reports:

.. code-block:: text

    invalid option type for 'name', unexpected 'float', expecting 'bool'

.. seealso::

    :ref:`Contexts <contexts>` for memory and stack allocation options,
    :ref:`Annotations <annotations>` for annotation-based equivalents,
    :ref:`Unsafe <unsafe>` for ``unsafe_table_lookup`` option,
    :ref:`Program structure <program_structure>` for overall file layout and ``options`` placement.

