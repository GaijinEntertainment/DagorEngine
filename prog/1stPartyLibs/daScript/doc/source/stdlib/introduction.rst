.. _stdlib_introduction:

============
Introduction
============

The Daslang standard library is a collection of modules that extend the language
with commonly needed functionality — math, string manipulation, file I/O,
serialization, regular expressions, AST manipulation, and more.

Some modules are implemented in C++ and are available by default (``builtin``, ``math``,
``strings``, ``fio``, ``rtti``, ``ast``, ``network``, ``jobque``, ``uriparser``).
The rest are written in daslang itself and live in the ``daslib/`` directory;
import them with ``require daslib/<module_name>``.

Core runtime
------------

* :doc:`builtin <generated/builtin>` — built-in runtime functions, operators, containers, smart pointers, and system infrastructure
* :doc:`math <generated/math>` — vector and scalar math, trigonometry, noise, matrix and quaternion operations
* :doc:`math_bits <generated/math_bits>` — bit-level float/int/double reinterpretation helpers
* :doc:`math_boost <generated/math_boost>` — angle conversions, projection matrices, plane math, color packing
* :doc:`fio <generated/fio>` — file input/output, directory manipulation, process spawning
* :doc:`random <generated/random>` — LCG-based random number generators and distributions
* :doc:`network <generated/network>` — low-level TCP socket server
* :doc:`uriparser <generated/uriparser>` — URI parsing, normalization, and file-name conversion (based on UriParser)
* :doc:`uriparser_boost <generated/uriparser_boost>` — URI component accessors and query-string helpers

Strings
-------

* :doc:`strings <generated/strings>` — core string manipulation: search, slice, conversion, builder, character groups
* :doc:`strings_boost <generated/strings_boost>` — split/join, replace, Levenshtein distance, formatting helpers
* :doc:`utf8_utils <generated/utf8_utils>` — UTF-8 encoding and decoding utilities
* :doc:`temp_strings <generated/temp_strings>` — temporary string construction that avoids heap allocation
* :doc:`stringify <generated/stringify>` — ``%stringify~`` reader macro for embedding long strings at compile time
* :doc:`base64 <generated/base64>` — Base64 encoding and decoding

Regular expressions
-------------------

* :doc:`regex <generated/regex>` — regular expression matching and searching
* :doc:`regex_boost <generated/regex_boost>` — ``%regex~`` reader macro for compile-time regex construction

Reflection and AST
------------------

* :doc:`rtti <generated/rtti>` — runtime type information: type queries, module/function/variable iteration, compilation and simulation
* :doc:`ast <generated/ast>` — compile-time AST access: expression/function/structure creation, visitor pattern, macro infrastructure
* :doc:`ast_boost <generated/ast_boost>` — AST convenience helpers: queries, annotation manipulation, expression generation, visitor utilities
* :doc:`ast_block_to_loop <generated/ast_block_to_loop>` — AST transform that converts block-based iteration to explicit loops (used by DECS)
* :doc:`ast_used <generated/ast_used>` — collect all types used by a set of functions (for code generation)
* :doc:`ast_match <generated/ast_match>` — AST pattern matching via reverse reification: match expressions against structural patterns
* :doc:`quote <generated/quote>` — AST quasiquotation for constructing syntax trees from inline code
* :doc:`type_traits <generated/type_traits>` — compile-time type introspection and manipulation macros
* :doc:`typemacro_boost <generated/typemacro_boost>` — type macro and template structure support infrastructure
* :doc:`dynamic_cast_rtti <generated/dynamic_cast_rtti>` — runtime dynamic casting between class hierarchies

Functional and algorithms
-------------------------

* :doc:`functional <generated/functional>` — higher-order functions: ``filter``, ``map``, ``reduce``, ``any``, ``all``, ``flatten``, ``sorted``
* :doc:`algorithm <generated/algorithm>` — binary search, topological sort, set operations on tables, array utilities
* :doc:`sort_boost <generated/sort_boost>` — custom comparator support for the built-in ``sort``
* :doc:`linq <generated/linq>` — LINQ-style query operations: select, where, order, group, join, aggregate, set operations
* :doc:`linq_boost <generated/linq_boost>` — macro support for LINQ query syntax

Data structures
---------------

* :doc:`flat_hash_table <generated/flat_hash_table>` — open-addressing flat hash table template
* :doc:`cuckoo_hash_table <generated/cuckoo_hash_table>` — cuckoo hash table with O(1) worst-case lookup
* :doc:`bool_array <generated/bool_array>` — packed boolean array (``BoolArray``) backed by ``uint[]`` storage
* :doc:`soa <generated/soa>` — Structure of Arrays transformation for cache-friendly data layout

Serialization and data
----------------------

* :doc:`archive <generated/archive>` — general-purpose binary serialization framework with ``Serializer`` / ``Archive`` pattern
* :doc:`json <generated/json>` — JSON parser and writer (``JsValue`` variant, ``read_json``, ``write_json``)
* :doc:`json_boost <generated/json_boost>` — automatic struct ↔ JSON conversion via reflection

Jobs and concurrency
--------------------

* :doc:`jobque <generated/jobque>` — job queue primitives: channels, job status, lock boxes, atomics
* :doc:`jobque_boost <generated/jobque_boost>` — ``new_job`` / ``new_thread`` helpers, channel iteration
* :doc:`apply_in_context <generated/apply_in_context>` — cross-context function evaluation helpers
* :doc:`async_boost <generated/async_boost>` — async/await coroutine macros using job queues

Macros and metaprogramming
--------------------------

* :doc:`templates <generated/templates>` — ``decltype`` macro and ``[template]`` function annotation
* :doc:`templates_boost <generated/templates_boost>` — template application helpers: variable/type replacement, hygienic names
* :doc:`macro_boost <generated/macro_boost>` — miscellaneous macro manipulation utilities
* :doc:`contracts <generated/contracts>` — function argument contract annotations (``[expect_any_array]``, ``[expect_any_table]``, etc.)
* :doc:`apply <generated/apply>` — ``apply`` reflection pattern for struct field iteration
* :doc:`enum_trait <generated/enum_trait>` — compile-time enumeration trait queries
* :doc:`constexpr <generated/constexpr>` — constant expression detection and substitution macro
* :doc:`bitfield_boost <generated/bitfield_boost>` — operator overloads for bitfield types
* :doc:`bitfield_trait <generated/bitfield_trait>` — reflection utilities for bitfield names and values
* :doc:`consume <generated/consume>` — ``consume`` pattern for move-ownership argument passing
* :doc:`generic_return <generated/generic_return>` — ``[generic_return]`` annotation for return type instantiation
* :doc:`remove_call_args <generated/remove_call_args>` — AST transformation to remove specified call arguments
* :doc:`class_boost <generated/class_boost>` — macros for extending class functionality and method binding

Control flow macros
-------------------

* :doc:`defer <generated/defer>` — ``defer`` and ``defer_delete`` — execute code at scope exit
* :doc:`if_not_null <generated/if_not_null>` — ``if_not_null`` safe-access macro
* :doc:`safe_addr <generated/safe_addr>` — ``safe_addr`` and ``temp_ptr`` — safe temporary pointer macros
* :doc:`static_let <generated/static_let>` — ``static_let`` — variables initialized once and persisted across calls
* :doc:`lpipe <generated/lpipe>` — left-pipe operator macro (``<|``)
* :doc:`is_local <generated/is_local>` — ``is_local_expr`` / ``is_scope_expr`` AST query helpers
* :doc:`assert_once <generated/assert_once>` — ``assert_once`` — assertion that fires only on first failure
* :doc:`unroll <generated/unroll>` — compile-time loop unrolling macro
* :doc:`instance_function <generated/instance_function>` — ``[instance_function]`` annotation for struct method binding
* :doc:`array_boost <generated/array_boost>` — ``temp_array``, ``array_view``, and ``empty`` helpers

Pattern matching
----------------

* :doc:`match <generated/match>` — ``match`` macro for structural pattern matching on variants and values

Entity component system
-----------------------

* :doc:`decs <generated/decs>` — DECS (Daslang Entity Component System): archetypes, components, queries, staged updates
* :doc:`decs_boost <generated/decs_boost>` — DECS macro support for query syntax
* :doc:`decs_state <generated/decs_state>` — DECS state machine support for entity lifecycle

OOP and interfaces
------------------

* :doc:`coroutines <generated/coroutines>` — coroutine runner (``cr_run``, ``cr_run_all``) and generator macros (``yield_from``)
* :doc:`interfaces <generated/interfaces>` — ``[interface]`` and ``[implements]`` annotations for interface-based polymorphism
* :doc:`cpp_bind <generated/cpp_bind>` — C++ class adapter binding code generator

Testing and debugging
---------------------

* :doc:`faker <generated/faker>` — random test data generator: strings, numbers, dates
* :doc:`fuzzer <generated/fuzzer>` — function fuzzing framework
* :doc:`profiler <generated/profiler>` — instrumenting CPU profiler for function-level timing
* :doc:`profiler_boost <generated/profiler_boost>` — profiler cross-context helpers and high-level macros
* :doc:`debug_eval <generated/debug_eval>` — runtime expression evaluation for interactive debugging
* :doc:`dap <generated/dap>` — Debug Adapter Protocol (DAP) data structures for debugger integration

Code quality and tooling
------------------------

* :doc:`lint <generated/lint>` — static analysis pass for common code issues
* :doc:`validate_code <generated/validate_code>` — AST validation annotations for custom code checks
* :doc:`refactor <generated/refactor>` — automated code refactoring transformations

Developer tools
---------------

* :doc:`das_source_formatter <generated/das_source_formatter>` — daslang source code formatter
* :doc:`das_source_formatter_fio <generated/das_source_formatter_fio>` — file-based source code formatting
* :doc:`rst <generated/rst>` — RST documentation generator used to produce this reference
