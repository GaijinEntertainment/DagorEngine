.. _type_mangling:

.. index::
   single: Type Mangling
   single: Mangled Names

==============
 Type Mangling
==============

daslang uses a compact text encoding called **type mangling** to represent
types as strings.  Mangled names are used internally for function overload
resolution, ABI hashing, and debug information.  They are also the format
accepted by the C integration API (``daScriptC.h``) when binding interop
functions.

The mangling is defined by ``TypeDecl::getMangledName`` and parsed by
``MangledNameParser::parseTypeFromMangledName`` in
``src/ast/ast_typedecl.cpp``.


Primitive types
===============

Each primitive type has a short mnemonic:

============  ========  ============================
daslang       Mangled   Notes
============  ========  ============================
``void``      ``v``
``bool``      ``b``
``int``       ``i``
``int2``      ``i2``    SIMD 2-component vector
``int3``      ``i3``
``int4``      ``i4``
``int8``      ``i8``    8-bit signed integer
``int16``     ``i16``   16-bit signed integer
``int64``     ``i64``   64-bit signed integer
``uint``      ``u``
``uint2``     ``u2``
``uint3``     ``u3``
``uint4``     ``u4``
``uint8``     ``u8``
``uint16``    ``u16``
``uint64``    ``u64``
``float``     ``f``
``float2``    ``f2``
``float3``    ``f3``
``float4``    ``f4``
``double``    ``d``
``string``    ``s``
``range``     ``r``
``urange``    ``z``
``range64``   ``r64``
``urange64``  ``z64``
============  ========  ============================


Qualifiers and modifiers
========================

Qualifiers are **prepended** before the base type they modify.  For
example, ``const int`` is ``Ci`` and ``const string&`` is ``C&s``.

============  ========  ============================
Qualifier     Mangled   Meaning
============  ========  ============================
const         ``C``     Constant
ref (``&``)   ``&``     Reference
temporary     ``#``     Temporary value
implicit      ``I``     Implicit type
explicit      ``X``     Explicit type match
============  ========  ============================


Pointers
========

A raw pointer is ``?``, followed by an optional smart-pointer marker:

====================  ========
daslang               Mangled
====================  ========
``Foo?``              ``?``
``smart_ptr<Foo>``    ``?M``
``smart_ptr<Foo>``    ``?W``    (native smart pointer)
====================  ========

The pointed-to type is encoded as a *first-type* prefix (see `Composite
prefixes`_ below).


Composite prefixes
==================

Some types carry sub-types.  These use numbered prefix wrappers:

========  =======================================  ==================================
Prefix    Meaning                                  Example
========  =======================================  ==================================
``1<T>``  First sub-type (element type)            ``array<int>`` → ``1<i>A``
``2<T>``  Second sub-type (value type for tables)  ``table<string;int>`` → ``1<s>2<i>T``
========  =======================================  ==================================


Container types
===============

=============  ========  ===============================
daslang        Mangled   Encoding pattern
=============  ========  ===============================
``array``      ``A``     ``1<element>A``
``table``      ``T``     ``1<key>2<value>T``
``iterator``   ``G``     ``1<element>G``
``tuple``      ``U``     ``0<types...>U``
``variant``    ``V``     ``0<types...>V``
=============  ========  ===============================

For example:

- ``array<int>`` → ``1<i>A``
- ``array<float>`` → ``1<f>A``
- ``table<string;int>`` → ``1<s>2<i>T``
- ``iterator<int>`` → ``1<i>G``


Fixed-size arrays (dim)
=======================

Fixed-size dimensions are encoded with square brackets before the base type:

- ``int[3]`` → ``[3]i``
- ``float[2][4]`` → ``[2][4]f``


Callable types
==============

daslang has three callable types, each with its own suffix:

================  ========  ============================
daslang           Mangled   Characteristics
================  ========  ============================
function pointer  ``@@``      No capture, cheapest call
lambda            ``@``       Heap-allocated, captures variables
block             ``$``       Stack-allocated, cannot outlive scope
================  ========  ============================

The argument list uses the ``0<args...>`` prefix with arguments separated
by semicolons.  The callable's own **return type** is not encoded in the
mangled name — it is determined by the enclosing context.

Examples:

- ``function<(a:int; b:float) : string>`` → ``0<i;f>@@``
- ``lambda<(a:int; b:float) : string>``   → ``0<i;f>@``
- ``block<(a:int; b:float) : string>``    → ``0<i;f>$``
- ``block<(a:int) : void>``               → ``0<i>$``
- ``function<() : void>``                 → ``@@`` (no args → no ``0<>`` prefix)


Structures, handled types, and enumerations
============================================

Named types use angle-bracket wrappers with a type-class letter:

==============  ================================  ================================
Type class      Mangled                           Example
==============  ================================  ================================
Structure       ``S<name>`` or ``S<mod::name>``   ``S<MyStruct>``
Handled type    ``H<name>`` or ``H<mod::name>``   ``H<model>``
Enumeration     ``E<name>``                       ``E<Color>``
Enumeration8    ``E8<name>``                      8-bit enum
Enumeration16   ``E16<name>``                     16-bit enum
Enumeration64   ``E64<name>``                     64-bit enum
==============  ================================  ================================

When the type belongs to a module, the module name is included:
``H<tutorial_07::model>`` or ``E<math::MathOp>``.


Type aliases
============

An alias wraps the underlying type:

- ``Y<AliasName>type`` — e.g. ``Y<IntArray>1<i>A`` for
  ``typedef IntArray = array<int>``


Named arguments
===============

Named arguments in callable types use the ``N<names...>`` prefix:

- ``N<a;b>0<i;f>$`` — a block taking named arguments ``a:int`` and
  ``b:float``


Bitfields
=========

==========  ========
daslang     Mangled
==========  ========
bitfield    ``t``
bitfield8   ``t8``
bitfield16  ``t16``
bitfield64  ``t64``
==========  ========


Special / internal types
========================

These are used internally by the compiler and macro system:

==============  ========  ============================
Type            Mangled   Purpose
==============  ========  ============================
auto (infer)    ``.``     Auto-inferred type
option          ``|``     Type option (overload sets)
typeDecl        ``D``     ``typeinfo`` expression type
alias           ``L``     Unresolved alias reference
anyArgument     ``*``     Wildcard argument
fakeContext     ``_c``    Implicit ``__context__`` parameter
fakeLineInfo    ``_l``    Implicit ``__lineinfo__`` parameter
typeMacro       ``^``     Macro-expanded type
aotAlias        ``F``     AOT alias marker
==============  ========  ============================


Remove-qualifiers (generics)
============================

In generic function signatures, qualifiers can be explicitly removed:

============  ========  ============================
Trait         Mangled   Meaning
============  ========  ============================
remove ref    ``-&``    Strip reference
remove const  ``-C``    Strip const
remove temp   ``-#``    Strip temporary
remove dim    ``-[]``   Strip fixed dimensions
============  ========  ============================


Interop function signatures
============================

The C API function ``das_module_bind_interop_function`` takes a mangled
signature string that describes the full function type.  The format is:

.. code-block:: text

   "return_type arg1_type arg2_type ..."

Types are separated by **spaces**.  The first type is the return type,
followed by each argument type.  Each type is a complete mangled name.

Examples
--------

.. code-block:: text

   "v i"                   →  void func(int)
   "v s"                   →  void func(string)
   "i i i"                 →  int func(int, int)
   "f H<Point2D>"          →  float func(Point2D)
   "s H<Point2D>"          →  string func(Point2D)
   "s 0<i;f>@@ i f"        →  string func(function<(int,float):string>, int, float)
   "s 0<i;f>@  i f"        →  string func(lambda<(int,float):string>, int, float)
   "s 0<i;f>$  i f"        →  string func(block<(int,float):string>, int, float)
   "v 0<i;f>$"             →  void func(block<(int,float)>)
   "1<H<model>>? C1<f>A"   →  model? func(const array<float>)


Querying mangled names at runtime
=================================

From daslang code, mangled names can be obtained with ``typeinfo``:

.. code-block:: das

   options gen2

   def my_function(a : int; b : float) : string {
       return "{a} {b}"
   }

   [export]
   def main() {
       // Function mangled name via @@
       print("mangled: {typeinfo mangled_name(@@my_function)}\n")
   }

This is useful for debugging type signatures and verifying that C-side
mangled strings match the daslang-side expectations.


.. seealso::

   :ref:`tutorial_integration_c_binding_types` — binding custom types with mangled names

   :ref:`tutorial_integration_c_callbacks` — callable type mangling for function pointers, lambdas, and blocks

   :ref:`tutorial_integration_c_unaligned_advanced` — unaligned ABI interop functions

   :ref:`contexts` — runtime context lookups by mangled name hash
