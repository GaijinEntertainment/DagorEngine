.. _tutorial_integration_c_ecs:

.. index::
   single: Tutorial; C Integration; ECS
   single: Tutorial; C Integration; Mock ECS
   single: Tutorial; C Integration; Macros
   single: Tutorial; C Integration; TypeInfo

=============================================
 C Integration: Mock ECS
=============================================

This tutorial demonstrates a pattern for integrating daslang with a
**native Entity Component System (ECS)**.  The C side owns component data
as flat arrays (SOA layout).  daslang scripts define "systems" -- small
update functions annotated with ``[es]`` -- and a daslang macro module
handles all the registration plumbing.

Three files work together:

====================  ================================================================
File                  Role
====================  ================================================================
``12_ecs.c``          C host -- mock ECS data, module, game loop
``ecs_macro.das``     Macro module -- ``[es]`` annotation, ``@required`` globals
``12_ecs.das``        User script -- struct, globals, ES functions
====================  ================================================================


The user script
===============

The user defines a **component struct** whose field names match the C-side
array names, declares **host-provided globals** with ``@required``, and
writes ``[es]`` functions with **no arguments** -- the macro injects them:

.. code-block:: das

   options gen2

   require tutorial_c_12
   require ecs_macro

   var @required dt : float

   struct Movement {
       position : float3
       velocity : float3
   }

   [es]
   def update() {
       position += velocity * dt
   }

   [es]
   def apply_gravity() {
       velocity.y -= 9.8 * dt
   }

   [export]
   def test() {
       print("ECS ready\n")
   }

The ``[es]`` macro transforms ``def update()`` into
``def update(var position : float3&; var velocity : float3&)`` and generates
an ``[init]`` function that calls ``ecs_register`` with the function pointer
and the struct's ``TypeInfo``.


The macro module
================

The ``ecs_macro`` module provides the ``[es]`` function annotation:

1. **Find the component struct** in the user's module (one per module, skips
   generated/lambda/generator structs).

2. **Add struct fields as arguments** using ``qmacro_variable`` with a ref
   type -- ``type<$t(fld._type)&>``.

3. **Set** ``[export]`` so the C host can find the function.

4. **Generate an** ``[init]`` function that calls ``ecs_register`` passing the
   function pointer (via ``@@``), the name, and a ``TypeInfo`` pointer
   obtained with ``typeinfo rtti_typeinfo``.

5. **Register** ``@required`` **globals** -- scans for the annotation argument
   on module globals and generates ``ecs_register_global`` calls with
   ``addr()`` of the global variable.

.. code-block:: das

   [function_macro(name="es")]
   class EsMacro : AstFunctionAnnotation {
       def override apply(var func : FunctionPtr; ...) : bool {
           // ... find struct, add fields as var ref args ...
           for (fld in st.fields) {
               func.arguments |> emplace_new <| qmacro_variable(
                   string(fld.name), type<$t(fld._type)&>)
           }
           func.flags |= FunctionFlags.exports
           // Generate [init] registration
           var blk <- setup_call_list("register`es`{funcName}", ...)
           blk.list |> emplace_new <| qmacro(
               ecs_register(
                   unsafe(reinterpret<void?> @@$c(funcName)),
                   $v(funcName),
                   typeinfo rtti_typeinfo(type<$t(stType)>)))
           return true
       }
   }


The C host
==========

**Mock ECS data** -- hardcoded SOA arrays:

.. code-block:: c

   #define NUM_ENTITIES 4
   static float comp_position[NUM_ENTITIES][3] = { ... };
   static float comp_velocity[NUM_ENTITIES][3] = { ... };

**Component registry** maps names to data arrays:

.. code-block:: c

   typedef struct { const char *name; void *data; int elem_size; } ComponentArray;

   register_component("position", comp_position, 3 * sizeof(float));
   register_component("velocity", comp_velocity, 3 * sizeof(float));

**C module** (``tutorial_c_12``) exposes two interop functions:

- ``ecs_register(fn : void?; name : string; ti : TypeInfo const)`` -- stores
  the function pointer and introspects the struct via ``TypeInfo``
- ``ecs_register_global(name : string; ptr : void?; ti : TypeInfo const)``
  -- stores a pointer to the daslang global variable

Type mangling uses ``CH<rtti_core::TypeInfo>`` for a const handled type
parameter.  The module group must include the ``rtti_core`` module (via
``das_module_find``) so the mangled name parser can resolve it.

**Game loop** -- each tick, the C host writes ``@required`` globals and
calls each registered ES function per entity.  Argument names are matched
to component arrays once per ES (not per entity):

.. code-block:: c

   // Resolve components for each argument (once per ES)
   ComponentArray * arg_comp[16];
   for (int a = 0; a < argc; a++)
       arg_comp[a] = find_component(das_func_info_get_arg_name(fi, a));

   // Call per entity
   for (int ent = 0; ent < NUM_ENTITIES; ent++) {
       vec4f args[16];
       for (int a = 0; a < argc; a++) {
           char * base = (char *)arg_comp[a]->data + ent * arg_comp[a]->elem_size;
           args[a] = das_result_ptr(base);
       }
       das_context_eval_with_catch(ctx, fn, args);
   }


Build & run
===========

Build::

   cmake --build build --config Release --target integration_c_12

Run::

   bin/Release/integration_c_12

Expected output::

   [C] ecs_register: 'update' (struct 'Movement', 2 fields: position:float3 velocity:float3)
   [C] ecs_register_global: 'dt' (float, 4 bytes)
   [C] ecs_register: 'apply_gravity' (struct 'Movement', 2 fields: position:float3 velocity:float3)
   ECS ready

   Initial state:
     [0] pos=(0.00, 10.00, 0.00) vel=(1.00, 0.00, 0.00)
     [1] pos=(5.00, 20.00, 0.00) vel=(0.00, 2.00, -1.00)
     [2] pos=(-3.00, 5.00, 2.00) vel=(0.00, 0.00, 0.00)
     [3] pos=(0.00, 0.00, 0.00) vel=(3.00, 1.00, 0.00)

   After 3 ticks (dt=0.0167):
     [0] pos=(0.05, 9.99, 0.00) vel=(1.00, -0.49, 0.00)
     [1] pos=(5.00, 20.09, -0.05) vel=(0.00, 1.51, -1.00)
     [2] pos=(-3.00, 4.99, 2.00) vel=(0.00, -0.49, 0.00)
     [3] pos=(0.15, 0.04, 0.00) vel=(3.00, 0.51, 0.00)


.. seealso::

   Full source:
   :download:`12_ecs.c <../../../../tutorials/integration/c/12_ecs.c>`,
   :download:`ecs_macro.das <../../../../tutorials/integration/c/ecs_macro.das>`,
   :download:`12_ecs.das <../../../../tutorials/integration/c/12_ecs.das>`

   Previous tutorial: :ref:`tutorial_integration_c_type_introspection`

   C API reference: :ref:`embedding_c_api`

   daScriptC.h API header: ``include/daScript/daScriptC.h``
