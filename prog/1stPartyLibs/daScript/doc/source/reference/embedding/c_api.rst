.. _embedding_c_api:

.. index::
   single: Embedding; C API
   single: Embedding; daScriptC.h

=========================================
 C API Reference
=========================================

The C API (``daScriptC.h``) provides a pure-C interface for embedding
daslang in applications that cannot use C++.  It covers the full
lifecycle: initialization, compilation, simulation, function calls,
type binding, serialization, and sandboxing.

For step-by-step tutorials with compilable examples, see the
:ref:`C integration tutorials <tutorial_integration_c_hello_world>`.


.. contents::
   :local:
   :depth: 2


When to use the C API
=====================

The C API is appropriate when:

* The host application is written in C (not C++).
* You need a stable ABI boundary (e.g. loading daslang as a shared
  library from another language).
* You want to avoid C++ name mangling in your interop layer.

The C++ API (``daScript.h``) is more ergonomic and exposes more
functionality.  The C API covers the most common embedding scenarios
but does not expose every C++ feature (e.g. class adapters, custom
annotations).


Initialization and shutdown
===========================

.. code-block:: c

   #include "daScript/daScriptC.h"

   int main() {
       das_initialize();
       // ... use daslang ...
       das_shutdown();
       return 0;
   }

``das_initialize()``
   Must be called once before any other C API call.  Registers all
   built-in modules.

``das_shutdown()``
   Frees all internal structures.  Call once when done.


Text output
===========

.. code-block:: c

   das_text_writer * tout = das_text_make_printer();  // stdout
   das_text_writer * buf  = das_text_make_writer();   // string buffer
   das_text_output(tout, "hello\n");
   das_text_release(tout);
   das_text_release(buf);

``das_text_make_printer()``
   Creates a writer that prints to stdout.

``das_text_make_writer()``
   Creates a writer that accumulates output in an internal buffer.

``das_text_release(writer)``
   Frees a text writer.


File access
===========

.. code-block:: c

   das_file_access * fAccess = das_fileaccess_make_default();
   // ... compile scripts ...
   das_fileaccess_release(fAccess);

``das_fileaccess_make_default()``
   Creates a filesystem-backed file access.

``das_fileaccess_make_project(project_file)``
   Creates a file access backed by a ``.das_project`` file for
   sandboxing.  See :ref:`tutorial_integration_c_sandbox`.

``das_fileaccess_introduce_file(access, name, content, owns)``
   Registers a virtual file from a C string.  If ``owns`` is non-zero,
   the content is copied; otherwise the caller must keep it alive.

``das_fileaccess_introduce_file_from_disk(access, name, disk_path)``
   Reads a file from disk and caches it under a virtual path.

``das_fileaccess_introduce_daslib(access)``
   Pre-loads all standard library modules into the cache.

``das_fileaccess_introduce_native_modules(access)``
   Pre-loads all native plugin modules listed in ``external_resolve.inc``.

``das_fileaccess_lock(access)`` / ``das_fileaccess_unlock(access)``
   While locked, only pre-introduced files can be accessed — filesystem
   reads are blocked.  Essential for sandboxing.

``das_get_root(buf, maxbuf)``
   Copies the daslang root path into ``buf``.


Compilation
===========

.. code-block:: c

   das_module_group * lib = das_modulegroup_make();
   das_program * program = das_program_compile(
       "script.das", fAccess, tout, lib);

   if (das_program_err_count(program) > 0) {
       for (int i = 0; i < das_program_err_count(program); i++) {
           das_error * err = das_program_get_error(program, i);
           das_error_output(err, tout);
       }
   }

``das_program_compile(file, access, tout, libgroup)``
   Compiles a ``.das`` file.  Always returns non-NULL; check
   ``das_program_err_count()`` for errors.

``das_program_compile_policies(file, access, tout, libgroup, policies)``
   Same as above but applies custom ``CodeOfPolicies``.

``das_program_release(program)``
   Frees a compiled program.

``das_program_err_count(program)``
   Returns the number of errors (0 = success).

``das_program_context_stack_size(program)``
   Returns the minimum stack size for simulation.

``das_program_get_error(program, index)``
   Returns the i-th error object.

See :ref:`tutorial_integration_c_hello_world`.


Compilation policies
--------------------

.. code-block:: c

   das_policies * pol = das_policies_make();
   das_policies_set_bool(pol, DAS_POLICY_AOT, 1);
   das_policies_set_bool(pol, DAS_POLICY_NO_UNSAFE, 1);
   das_policies_set_int(pol, DAS_POLICY_STACK, 65536);

   das_program * prog = das_program_compile_policies(
       "script.das", fAccess, tout, lib, pol);
   das_policies_release(pol);

**Boolean policies** (``das_bool_policy``):

.. list-table::
   :header-rows: 1

   * - Flag
     - Effect
   * - ``DAS_POLICY_AOT``
     - Enable AOT compilation linking
   * - ``DAS_POLICY_NO_UNSAFE``
     - Forbid ``unsafe`` blocks
   * - ``DAS_POLICY_NO_GLOBAL_VARIABLES``
     - Forbid module-level ``var`` declarations
   * - ``DAS_POLICY_NO_GLOBAL_HEAP``
     - Forbid heap allocations for globals
   * - ``DAS_POLICY_NO_INIT``
     - Forbid ``[init]`` functions
   * - ``DAS_POLICY_FAIL_ON_NO_AOT``
     - Treat missing AOT as an error
   * - ``DAS_POLICY_THREADLOCK_CONTEXT``
     - Enable context mutex for threading
   * - ``DAS_POLICY_INTERN_STRINGS``
     - Use string interning
   * - ``DAS_POLICY_PERSISTENT_HEAP``
     - Persistent heap (no GC between calls)
   * - ``DAS_POLICY_MULTIPLE_CONTEXTS``
     - Context-safe code generation
   * - ``DAS_POLICY_STRICT_SMART_POINTERS``
     - Strict smart pointer rules
   * - ``DAS_POLICY_RTTI``
     - Generate extended RTTI
   * - ``DAS_POLICY_NO_OPTIMIZATIONS``
     - Disable all optimizations

**Integer policies** (``das_int_policy``):

.. list-table::
   :header-rows: 1

   * - Field
     - Effect
   * - ``DAS_POLICY_STACK``
     - Context stack size in bytes
   * - ``DAS_POLICY_MAX_HEAP_ALLOCATED``
     - Max heap allocated (0 = unlimited)
   * - ``DAS_POLICY_MAX_STRING_HEAP_ALLOCATED``
     - Max string heap allocated (0 = unlimited)
   * - ``DAS_POLICY_HEAP_SIZE_HINT``
     - Initial heap size hint
   * - ``DAS_POLICY_STRING_HEAP_SIZE_HINT``
     - Initial string heap size hint

See :ref:`tutorial_integration_c_sandbox`.


Simulation and context
======================

.. code-block:: c

   das_context * ctx = das_context_make(
       das_program_context_stack_size(program));
   das_program_simulate(program, ctx, tout);

   das_function * fn = das_context_find_function(ctx, "main");
   das_context_eval_with_catch(ctx, fn, NULL);

   das_context_release(ctx);

``das_context_make(stackSize)``
   Creates an execution context.

``das_program_simulate(program, ctx, tout)``
   Links the program into the context.  Returns 1 on success.

``das_context_find_function(ctx, name)``
   Finds an exported function by name.

``das_context_release(ctx)``
   Frees a context.

See :ref:`tutorial_integration_c_calling_functions`.


Function evaluation
===================

Three calling conventions are provided:

Aligned (vec4f)
---------------

Arguments and return values use 16-byte aligned ``vec4f``:

.. code-block:: c

   vec4f args[2];
   args[0] = das_result_int(10);
   args[1] = das_result_int(20);
   vec4f result = das_context_eval_with_catch(ctx, fn, args);
   int sum = das_argument_int(result);

Unaligned (vec4f_unaligned)
---------------------------

No alignment requirement — preferred for plain C:

.. code-block:: c

   vec4f_unaligned args[2], result;
   das_result_int_unaligned(&args[0], 10);
   das_result_int_unaligned(&args[1], 20);
   das_context_eval_with_catch_unaligned(ctx, fn, args, 2, &result);
   int sum = das_argument_int_unaligned(&result);

Complex results (cmres)
-----------------------

For functions returning structures or tuples:

.. code-block:: c

   MyStruct result_buf;
   das_context_eval_with_catch_cmres(ctx, fn, args, &result_buf);

The caller allocates the buffer; the function writes the result into it.


Argument helpers
================

**Extracting values** from ``vec4f`` (in interop functions):

``das_argument_int(arg)``, ``das_argument_uint(arg)``,
``das_argument_int64(arg)``, ``das_argument_uint64(arg)``,
``das_argument_bool(arg)``, ``das_argument_float(arg)``,
``das_argument_double(arg)``, ``das_argument_string(arg)``,
``das_argument_ptr(arg)``, ``das_argument_function(arg)``,
``das_argument_lambda(arg)``, ``das_argument_block(arg)``

**Packing values** into ``vec4f`` (for returning from interop functions):

``das_result_void()``, ``das_result_int(r)``, ``das_result_uint(r)``,
``das_result_int64(r)``, ``das_result_uint64(r)``,
``das_result_bool(r)``, ``das_result_float(r)``,
``das_result_double(r)``, ``das_result_string(r)``,
``das_result_ptr(r)``, ``das_result_function(r)``,
``das_result_lambda(r)``, ``das_result_block(r)``

Unaligned variants (``_unaligned`` suffix) take a ``vec4f_unaligned *``
argument instead.


Calling lambdas and blocks
==========================

Lambdas and blocks use parallel evaluation functions:

.. code-block:: c

   // Lambda (heap-allocated closure)
   das_context_eval_lambda(ctx, lambda, args);
   das_context_eval_lambda_unaligned(ctx, lambda, args, nargs, &result);

   // Block (stack-allocated closure)
   das_context_eval_block(ctx, block, args);
   das_context_eval_block_unaligned(ctx, block, args, nargs, &result);

Complex-result variants (``_cmres``) are also available.

See :ref:`tutorial_integration_c_callbacks`.


Binding C functions
===================

.. code-block:: c

   vec4f my_add(das_context * ctx, das_node * node, vec4f * args) {
       int a = das_argument_int(args[0]);
       int b = das_argument_int(args[1]);
       return das_result_int(a + b);
   }

   das_module * mod = das_module_create("my_module");
   das_module_bind_interop_function(mod, lib, my_add,
       "add", "my_add", SIDEEFFECTS_none, " Cii>i");

The ``args`` string uses daslang's type mangling format.

For unaligned calling convention:

.. code-block:: c

   void my_add_u(das_context * ctx, das_node * node,
                 vec4f_unaligned * args, vec4f_unaligned * result) {
       int a = das_argument_int_unaligned(&args[0]);
       int b = das_argument_int_unaligned(&args[1]);
       das_result_int_unaligned(result, a + b);
   }

   das_module_bind_interop_function_unaligned(mod, lib, my_add_u,
       "add", "my_add_u", SIDEEFFECTS_none, " Cii>i");

See :ref:`tutorial_integration_c_binding_types`.


Binding types
=============

Structures
----------

.. code-block:: c

   das_structure * st = das_structure_make(lib,
       "MyType", "MyType", sizeof(MyType), alignof(MyType));
   das_structure_add_field(st, mod, lib,
       "x", "x", offsetof(MyType, x), "f");    // float
   das_structure_add_field(st, mod, lib,
       "y", "y", offsetof(MyType, y), "f");    // float
   das_module_bind_structure(mod, st);

Field types use mangled-name format (``"f"`` = float, ``"i"`` = int,
``"s"`` = string, etc.).

Enumerations
------------

.. code-block:: c

   das_enumeration * en = das_enumeration_make(
       "Color", "Color", 0);
   das_enumeration_add_value(en, "red",   "Color::red",   0);
   das_enumeration_add_value(en, "green", "Color::green", 1);
   das_enumeration_add_value(en, "blue",  "Color::blue",  2);
   das_module_bind_enumeration(mod, en);

Type aliases
------------

.. code-block:: c

   das_module_bind_alias(mod, lib, "Color", "i");  // alias Color = int


String allocation
=================

.. code-block:: c

   char * s = das_allocate_string(ctx, "hello");

Allocates a copy on the context's string heap.  The returned pointer
is managed by the context — do not free it manually.


Context variables
=================

After simulation, global variables are accessible by name or index:

.. code-block:: c

   int idx = das_context_find_variable(ctx, "counter");
   if (idx >= 0) {
       int * ptr = (int *)das_context_get_variable(ctx, idx);
       *ptr = 42;  // direct memory write
   }

   int total = das_context_get_total_variables(ctx);
   for (int i = 0; i < total; i++) {
       printf("%s: %d bytes\n",
           das_context_get_variable_name(ctx, i),
           das_context_get_variable_size(ctx, i));
   }

See :ref:`tutorial_integration_c_context_variables`.


Type introspection
==================

After simulation, the C API can inspect the types, struct layouts,
enumerations, and function signatures of a compiled program.  All
returned pointers are borrowed from the context — no allocation or
cleanup is needed.

Querying variable types
-----------------------

.. code-block:: c

   int idx = das_context_find_variable(ctx, "entities");
   das_type_info * ti = das_context_get_variable_type(ctx, idx);

   das_base_type bt = das_type_info_get_type(ti);   // DAS_TYPE_ARRAY
   int sz = das_type_info_get_size(ti);
   const char * desc = das_type_info_get_description(ti);  // "array<Entity>"

``das_context_get_variable_type(ctx, idx)``
   Returns the type info for the global variable at *idx*.

``das_type_info_get_type(info)``
   Returns the base type (``das_base_type`` enum: ``DAS_TYPE_INT``,
   ``DAS_TYPE_FLOAT``, ``DAS_TYPE_STRUCTURE``, ``DAS_TYPE_ARRAY``, etc.).

``das_type_info_get_size(info)`` / ``das_type_info_get_align(info)``
   Returns the size and alignment in bytes.

``das_type_info_get_flags(info)``
   Returns a bitmask of ``DAS_TYPEINFO_FLAG_*`` constants (ref, pod,
   const, smart_ptr, handled, etc.).

``das_type_info_get_description(info)``
   Returns a human-readable type string (e.g. ``"array<int>"``).
   Valid until the next call on the same thread.

``das_type_info_get_mangled_name(info)``
   Returns the mangled name string.  Same lifetime rules.


Navigating composite types
--------------------------

.. code-block:: c

   // array<Entity> -> Entity
   das_type_info * elem = das_type_info_get_first_type(ti);

   // table<string; int> -> key=string, value=int
   das_type_info * key = das_type_info_get_first_type(ti);
   das_type_info * val = das_type_info_get_second_type(ti);

   // tuple<int; float; string> -> 3 sub-types
   int n = das_type_info_get_arg_count(ti);
   das_type_info * t0 = das_type_info_get_arg_type(ti, 0);
   int off0 = das_type_info_get_tuple_field_offset(ti, 0);

``das_type_info_get_first_type(info)``
   For arrays/pointers/iterators: the element type.
   For tables: the key type.

``das_type_info_get_second_type(info)``
   For tables: the value type.

``das_type_info_get_arg_count(info)``
   Number of sub-types (tuples, variants, function types).

``das_type_info_get_arg_type(info, idx)``
   The *idx*-th sub-type.

``das_type_info_get_tuple_field_offset(info, idx)``
   Byte offset of the *idx*-th field in a tuple.

``das_type_info_get_variant_field_offset(info, idx)``
   Byte offset of the *idx*-th field in a variant.

``das_type_info_get_dim_count(info)`` / ``das_type_info_get_dim(info, idx)``
   Fixed-array dimension count and sizes.


Struct layout
-------------

.. code-block:: c

   das_struct_info * si = das_type_info_get_struct(elem);
   int nfields = das_struct_info_get_field_count(si);
   int struct_size = das_struct_info_get_size(si);

   for (int f = 0; f < nfields; f++) {
       const char * name = das_struct_info_get_field_name(si, f);
       int offset = das_struct_info_get_field_offset(si, f);
       das_type_info * ft = das_struct_info_get_field_type(si, f);
       das_base_type bt = das_type_info_get_type(ft);
       int fsz = das_type_info_get_size(ft);
   }

``das_type_info_get_struct(info)``
   Returns the ``das_struct_info`` for a ``DAS_TYPE_STRUCTURE`` type,
   or NULL.

``das_struct_info_get_name(si)`` / ``das_struct_info_get_module(si)``
   Name and module of the structure.

``das_struct_info_get_field_count(si)`` / ``das_struct_info_get_size(si)``
   Number of fields and total size in bytes.

``das_struct_info_get_field_name(si, idx)``
   Name of the *idx*-th field.

``das_struct_info_get_field_offset(si, idx)``
   Byte offset of the *idx*-th field.

``das_struct_info_get_field_type(si, idx)``
   Type info for the *idx*-th field.  Can be used recursively
   (e.g. a nested struct field).

``das_struct_info_get_flags(si)``
   Bitmask: ``DAS_STRUCTINFO_FLAG_CLASS``, ``DAS_STRUCTINFO_FLAG_LAMBDA``, etc.


Enum values
-----------

.. code-block:: c

   das_enum_info * ei = das_type_info_get_enum(ftype);
   int nvals = das_enum_info_get_count(ei);
   for (int v = 0; v < nvals; v++) {
       const char * vname = das_enum_info_get_value_name(ei, v);
       int64_t val = das_enum_info_get_value(ei, v);
   }

``das_type_info_get_enum(info)``
   Returns the ``das_enum_info`` for an enumeration type, or NULL.

``das_enum_info_get_name(ei)`` / ``das_enum_info_get_module(ei)``
   Name and module.

``das_enum_info_get_count(ei)``
   Number of values.

``das_enum_info_get_value_name(ei, idx)`` / ``das_enum_info_get_value(ei, idx)``
   Name and integer value of the *idx*-th entry.


Function signatures
-------------------

.. code-block:: c

   das_func_info * fi = das_function_get_info(fn);
   int nargs = das_func_info_get_arg_count(fi);
   for (int a = 0; a < nargs; a++) {
       const char * name = das_func_info_get_arg_name(fi, a);
       das_type_info * atype = das_func_info_get_arg_type(fi, a);
   }
   das_type_info * ret = das_func_info_get_result(fi);

``das_function_get_info(func)``
   Returns the debug metadata for a function handle.

``das_func_info_get_name(fi)`` / ``das_func_info_get_cpp_name(fi)``
   Function name and C++ mangled name.

``das_func_info_get_arg_count(fi)``
   Number of arguments.

``das_func_info_get_arg_name(fi, idx)`` / ``das_func_info_get_arg_type(fi, idx)``
   Name and type of the *idx*-th argument.

``das_func_info_get_result(fi)``
   Return type.

``das_func_info_get_flags(fi)``
   Bitmask: ``DAS_FUNCINFO_FLAG_INIT``, ``DAS_FUNCINFO_FLAG_BUILTIN``,
   ``DAS_FUNCINFO_FLAG_PRIVATE``, ``DAS_FUNCINFO_FLAG_SHUTDOWN``.

``das_context_get_total_functions(ctx)`` / ``das_context_get_function(ctx, idx)``
   Enumerate all functions in a simulated context.

See :ref:`tutorial_integration_c_type_introspection`.


Serialization
=============

.. code-block:: c

   // Serialize
   const void * data;
   int64_t size;
   das_serialized_data * blob = das_program_serialize(
       program, &data, &size);

   // Save data/size to file...

   // Deserialize (skips parsing and type inference)
   das_program * restored = das_program_deserialize(data, size);

   // Clean up
   das_serialized_data_release(blob);

See :ref:`tutorial_integration_c_serialization`.


AOT checking
============

.. code-block:: c

   das_function * fn = das_context_find_function(ctx, "test");
   if (das_function_is_aot(fn)) {
       printf("Running as native AOT code\n");
   }

See :ref:`tutorial_integration_c_aot`.


Module groups
=============

.. code-block:: c

   das_module_group * lib = das_modulegroup_make();
   das_module * mod = das_module_create("my_module");
   // ... bind functions, types, enums ...
   das_modulegroup_add_module(lib, mod);

   das_program * prog = das_program_compile(
       "script.das", fAccess, tout, lib);

   das_modulegroup_release(lib);


Side effects
============

Side-effect constants for ``das_module_bind_interop_function``:

``SIDEEFFECTS_none``
   Pure function.

``SIDEEFFECTS_unsafe``
   Function is unsafe.

``SIDEEFFECTS_modifyExternal``
   Modifies external state (stdout, files).

``SIDEEFFECTS_accessExternal``
   Reads external state.

``SIDEEFFECTS_modifyArgument``
   Mutates a reference argument.

``SIDEEFFECTS_modifyArgumentAndExternal``
   Both modifyArgument and modifyExternal.

``SIDEEFFECTS_accessGlobal``
   Reads shared global state.

``SIDEEFFECTS_invoke``
   Calls daslang callbacks.

``SIDEEFFECTS_worstDefault``
   Safe fallback — assumes all side effects.

