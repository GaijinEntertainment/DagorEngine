.. _embedding_modules:

.. index::
   single: Embedding; Modules
   single: Embedding; C++ Bindings
   single: Embedding; Functions
   single: Embedding; Types
   single: Embedding; Enumerations
   single: Embedding; Cast

=========================================
 C++ API Reference
=========================================

This page is the comprehensive reference for binding C++ code to daslang.
For step-by-step tutorials with compilable examples, see the
:ref:`C++ integration tutorials <tutorial_integration_cpp_hello_world>`.


.. contents::
   :local:
   :depth: 2


Creating a module
=================

Derive from ``Module``, register bindings in the constructor, and use
``REGISTER_MODULE``:

.. code-block:: cpp

   class Module_MyMod : public Module {
   public:
       Module_MyMod() : Module("my_module_name") {
           ModuleLibrary lib(this);
           lib.addBuiltInModule();
           // addAnnotation, addExtern, addEnumeration, addConstant ...
       }
   };
   REGISTER_MODULE(Module_MyMod);

The host uses ``NEED_MODULE(Module_MyMod)`` before ``Module::Initialize()``.
Scripts access it with ``require my_module_name``.

.. note::

   ``NEED_MODULE`` contains an ``extern`` declaration that binds to the
   enclosing namespace.  If your initialization code lives inside a C++
   namespace, use the namespace-safe pair ``DECLARE_MODULE`` (file scope)
   and ``PULL_MODULE`` (inside the namespace) instead.  For external
   modules, CMake also generates ``external_declare.inc`` and
   ``external_pull.inc`` alongside the traditional ``external_need.inc``.
   See :ref:`tutorial_integration_cpp_namespace_integration`.

See :ref:`tutorial_integration_cpp_custom_modules` for a complete example.


ModuleAotType
-------------

Three AOT compilation modes for modules:

``no_aot``
   Module functions are never AOT-compiled.  Used for dynamic or
   debug-only modules.

``hybrid``
   Functions use full ABI (slower calls, but no semantic hash
   dependency).  Required when the module may be compiled separately
   from the script.

``cpp``
   Full AOT — function calls use semantic hashes for dispatch.  Default
   and fastest mode.


Embedding daslang source in modules
---------------------------------------

Use the XDD mechanism (``CMAKE_TEXT_XXD`` CMake macro) to embed ``.das``
files as C++ byte arrays, then compile them with ``compileBuiltinModule``:

.. code-block:: cpp

   #include "my_module.das.inc"

   compileBuiltinModule("my_module.das",
       my_module_das, sizeof(my_module_das));

See :ref:`tutorial_integration_cpp_class_adapters` for a full example.


Builtin module constants
--------------------------

.. code-block:: cpp

   addConstant(*this, "MY_CONST", 42);

The type is inferred automatically from the C++ value.


.. _handles:

Binding C++ types
=================

To expose a C++ type to daslang, two things are needed:

1. ``MAKE_TYPE_FACTORY(DasName, CppType)`` at file scope — creates
   ``typeFactory<CppType>`` and ``typeName<CppType>``
2. A ``TypeAnnotation`` subclass — describes fields, properties, and
   behavior

``ManagedStructureAnnotation``
------------------------------

The most common helper for binding C++ structs (POD or non-POD):

.. code-block:: cpp

   struct Object {
       float3   pos;
       float3   vel;
       float    mass;
       float    length() const { return sqrt(pos.x*pos.x + pos.y*pos.y); }
   };

   MAKE_TYPE_FACTORY(Object, Object)

   struct ObjectAnnotation
       : ManagedStructureAnnotation<Object, true /* canNew */> {
       ObjectAnnotation(ModuleLibrary & ml)
           : ManagedStructureAnnotation("Object", ml) {
           addField<DAS_BIND_MANAGED_FIELD(pos)>("pos", "pos");
           addField<DAS_BIND_MANAGED_FIELD(vel)>("vel", "vel");
           addField<DAS_BIND_MANAGED_FIELD(mass)>("mass", "mass");
           addProperty<DAS_BIND_MANAGED_PROP(length)>(
               "length", "length");
       }
   };

   // In module constructor:
   addAnnotation(make_smart<ObjectAnnotation>(lib));

Key points:

* **Field binding order matters** — if type B contains type A, register
  A's annotation first.
* **Properties** look like fields in daslang (``obj.length``) but call
  C++ methods.
* ``addFieldEx`` provides custom offsets and type overrides.
* ``addPropertyExtConst`` handles const/non-const overloads.
* Auto-implements ``walk`` for data inspection.

Handled types are **reference types** in daslang — mutable local
variables (``var``) require ``unsafe`` blocks.  Provide factory functions
(``make_xxx()``) returning by value so scripts can use ``let x = make_xxx()``
without ``unsafe``.

See :ref:`tutorial_integration_cpp_binding_types` for a complete example.


``DummyTypeAnnotation``
-----------------------

Exposes an opaque type with no accessible fields:

.. code-block:: cpp

   addAnnotation(make_smart<DummyTypeAnnotation>(
       "OpaqueHandle", "OpaqueHandle", sizeof(OpaqueHandle),
       alignof(OpaqueHandle)));

Use case: passing handles through daslang without exposing internals.


``ManagedVectorAnnotation``
---------------------------

Exposes ``std::vector<T>`` with automatic ``push``, ``pop``, ``clear``,
``resize``, ``length``, and iteration support:

.. code-block:: cpp

   addAnnotation(make_smart<ManagedVectorAnnotation<int32_t>>(
       "IntVector", lib));


``ManagedValueAnnotation``
--------------------------

For POD types passed by value (must fit in 128 bits).  Requires
``cast<T>`` specialization.


``TypeAnnotation`` virtual methods
-------------------------------------

For full control, subclass ``TypeAnnotation`` directly.  Key methods:

**Capability queries:**
``canAot``, ``canCopy``, ``canMove``, ``canClone``, ``isPod``,
``isRawPod``, ``isRefType``, ``isLocal``, ``canNew``, ``canDelete``,
``canDeletePtr``, ``needDelete``, ``isIndexable``, ``isIterable``,
``isShareable``, ``isSmart``, ``canSubstitute``

**Size and alignment:**
``getSizeOf``, ``getAlignOf``

**Field access:**
``makeFieldType``, ``makeSafeFieldType``

**Indexing and iteration:**
``makeIndexType``, ``makeIteratorType``

**Simulation:**
``simulateDelete``, ``simulateDeletePtr``, ``simulateCopy``,
``simulateClone``, ``simulateRef2Value``, ``simulateGetNew``,
``simulateGetAt``, ``simulateGetAtR2V``, ``simulateGetIterator``

**AOT visitors:**
``aotPreVisitGetField``, ``aotVisitGetField``,
``aotPreVisitGetFieldPtr``, ``aotVisitGetFieldPtr``


.. _cast:

Cast infrastructure
===================

``cast<T>`` converts between ``vec4f`` (daslang's universal register)
and C++ types:

.. code-block:: cpp

   // Value types (≤128 bits)
   vec4f v = cast<int32_t>::from(42);
   int32_t i = cast<int32_t>::to(v);

   // Reference types (pointer packed into vec4f)
   vec4f v = cast<MyObj &>::from(obj);
   MyObj & ref = cast<MyObj &>::to(v);

``MAKE_TYPE_FACTORY(DasName, CppType)`` creates the ``typeFactory`` and
``typeName`` specializations needed by the binding system:

.. code-block:: cpp

   MAKE_TYPE_FACTORY(Point3, float3)

This creates ``typeFactory<float3>::make()`` (returns ``TypeDeclPtr``)
and ``typeName<float3>()`` (returns ``"Point3"``).

**Type aliases** — to expose ``Point3`` as an alias for ``float3``,
provide a custom ``typeFactory<Point3>`` specialization that returns
the existing ``float3`` type declaration.


Binding C++ functions
=====================

``addExtern`` + ``DAS_BIND_FUN``
---------------------------------

.. code-block:: cpp

   int32_t add(int32_t a, int32_t b) { return a + b; }

   addExtern<DAS_BIND_FUN(add)>(*this, lib, "add",
       SideEffects::none, "add")
           ->args({"a", "b"});

**Return by value (> 128 bits)** — use ``SimNode_ExtFuncCallAndCopyOrMove``:

.. code-block:: cpp

   addExtern<DAS_BIND_FUN(make_matrix),
             SimNode_ExtFuncCallAndCopyOrMove>(
       *this, lib, "make_matrix",
       SideEffects::none, "make_matrix");

**Return by reference** — use ``SimNode_ExtFuncCallRef``:

.. code-block:: cpp

   addExtern<DAS_BIND_FUN(get_ref),
             SimNode_ExtFuncCallRef>(
       *this, lib, "get_ref",
       SideEffects::modifyExternal, "get_ref");

See :ref:`tutorial_integration_cpp_binding_functions` for a complete example.


Binding C++ methods
---------------------

daslang has no member functions — methods are free functions where the
first argument is ``self``.  Pipe syntax (``obj |> method()``) provides
method-call ergonomics:

.. code-block:: cpp

   using method_get = DAS_CALL_MEMBER(Counter::get);

   addExtern<DAS_CALL_METHOD(method_get)>(*this, lib, "get",
       SideEffects::none,
       DAS_CALL_MEMBER_CPP(Counter::get))
           ->args({"self"});

* Non-const methods: ``SideEffects::modifyArgument``
* Const methods: ``SideEffects::none``

See :ref:`tutorial_integration_cpp_methods` for details.


Binding operators
-------------------

Register functions with the operator symbol as the daslang name:

.. code-block:: cpp

   addExtern<DAS_BIND_FUN(vec3_add),
             SimNode_ExtFuncCallAndCopyOrMove>(
       *this, lib, "+",
       SideEffects::none, "vec3_add")
           ->args({"a", "b"});

Available operator names: ``+``, ``-``, ``*``, ``/``, ``%``, ``<<``,
``>>``, ``<``, ``>``, ``<=``, ``>=``, ``&``, ``|``, ``^``.

``addEquNeq<T>(*this, lib)`` binds both ``==`` and ``!=``.

See :ref:`tutorial_integration_cpp_operators_and_properties`.


``addInterop`` — low-level binding
-------------------------------------

``addInterop`` provides raw access to simulation-level arguments
(``vec4f *``) and call metadata (``SimNode_CallBase *``).  When a
template parameter is ``vec4f``, it means "any daslang type":

.. code-block:: cpp

   vec4f my_interop(Context & ctx, SimNode_CallBase * call,
                    vec4f * args) {
       TypeInfo * ti = call->types[0];  // type of first arg
       // inspect ti->type, ti->structType, etc.
       return v_zero();
   }

   addInterop<my_interop, void, vec4f>(*this, lib, "my_func",
       SideEffects::none, "my_interop");

Key capabilities (vs ``addExtern``):

* Access to ``call->types[]`` — per-argument ``TypeInfo``
* Access to ``call->debugInfo`` — source location of call site
* ``vec4f`` argument type = "any" — accepts any daslang type

**TypeInfo union warning:** ``TypeInfo`` has a union —
``structType``, ``enumType``, and ``annotation_or_name`` share memory.
Which member is valid depends on ``ti->type``:

* ``tStructure`` → ``ti->structType``
* ``tEnumeration`` / ``tEnumeration8`` / ``tEnumeration16`` → ``ti->enumType``
* ``tHandle`` → ``ti->getAnnotation()``

See :ref:`tutorial_integration_cpp_interop`.


Binding C++ enumerations
========================

.. code-block:: cpp

   // MUST come BEFORE `using namespace das`
   DAS_BASE_BIND_ENUM(CppEnum, DasName, Value1, Value2, Value3)

   using namespace das;

   // In module constructor:
   addEnumeration(make_smart<EnumerationDasName>());

* ``DAS_BASE_BIND_ENUM`` creates ``EnumerationDasName`` class +
  ``typeFactory<CppEnum>``
* ``DAS_BASE_BIND_ENUM_98`` — for unscoped (C-style) enums
* Place enum macros **before** ``using namespace das`` to avoid
  name collisions

See :ref:`tutorial_integration_cpp_binding_enums`.


.. _modules_function_sideeffects:

Function side effects
=====================

Every bound function declares its side effects:

``SideEffects::none``
   Pure function — no external state changes.

``SideEffects::unsafe``
   Function is unsafe.

``SideEffects::userScenario``
   User-defined scenario.

``SideEffects::modifyExternal``
   Modifies external state (stdout, files, globals).

``SideEffects::accessExternal``
   Reads external state.

``SideEffects::modifyArgument``
   Mutates a reference argument.

``SideEffects::accessGlobal``
   Reads shared global state.

``SideEffects::invoke``
   Calls daslang callbacks (blocks, functions, lambdas).

``SideEffects::worstDefault``
   Safe fallback — assumes all side effects.


Callbacks — blocks, functions, lambdas
==========================================

Three closure types exist for calling daslang code from C++:

.. list-table::
   :header-rows: 1

   * - Type
     - Template
     - Invocation
     - Lifetime
   * - Block
     - ``TBlock<Ret, Args...>``
     - ``das_invoke<Ret>::invoke(ctx, at, blk, args...)``
     - Stack-bound (current call only)
   * - Function
     - ``TFunc<Ret, Args...>``
     - ``das_invoke_function<Ret>::invoke(ctx, at, fn, args...)``
     - Context-bound (storable)
   * - Lambda
     - ``TLambda<Ret, Args...>``
     - ``das_invoke_lambda<Ret>::invoke(ctx, at, lmb, args...)``
     - Heap-allocated (captures variables)

Block callback example:

.. code-block:: cpp

   void with_values(int32_t a, int32_t b,
                    const TBlock<void, int32_t, int32_t> & blk,
                    Context * context, LineInfoArg * at) {
       das_invoke<void>::invoke(context, at, blk, a, b);
   }

   addExtern<DAS_BIND_FUN(with_values)>(*this, lib, "with_values",
       SideEffects::invoke, "with_values")
           ->args({"a", "b", "blk", "context", "at"});

Use ``SideEffects::invoke`` for any function that invokes script callbacks.

See :ref:`tutorial_integration_cpp_callbacks`.


.. _modules_file_access:

File access
===========

``FsFileAccess`` is the default file system access layer.  Override
``getNewFileInfo`` and ``getModuleInfo`` for custom file resolution:

.. code-block:: cpp

   struct ModuleInfo {
       string  moduleName;
       string  fileName;
       string  importDir;
   };

``setFileInfo`` registers virtual files (strings as files):

.. code-block:: cpp

   auto fAccess = make_smart<FsFileAccess>();
   auto fileInfo = make_unique<TextFileInfo>(
       text.c_str(), uint32_t(text.length()), false);
   fAccess->setFileInfo("virtual.das", das::move(fileInfo));

See :ref:`tutorial_integration_cpp_dynamic_scripts`.


.. _modules_project:

Project file resolution
========================

A custom ``module_get`` function resolves ``require`` paths:

.. code-block:: das

   options gen2
   require daslib/ast_boost public

   [function_macro(name="module_get")]
   class ModuleGetMacro : AstModuleGetMacro {
       def override getModule(
           req : string;
           from : string
       ) : ModuleInfo {
           // resolve require paths here
       }
   }

See :ref:`tutorial_integration_cpp_custom_modules` for the C++ equivalent.
