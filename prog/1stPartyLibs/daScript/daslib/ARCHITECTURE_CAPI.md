# daslib architecture notes - the generated C API

Companion to `ARCHITECTURE.md` in this folder; section numbers are unique across the family.
The header this section describes is the one BOTH backends write: `aot_standalone` for a
standalone context, `inject_lib` for `daslang -lib`.

## 30. c_api_header

- **One module writes every generated header, and one module owns the C declarations in it.** The
  C declarations come first; the C++ half (`CppApi`) follows under `#ifdef __cplusplus`, and both
  hosts include one file without seeing the other's half. `aot_standalone` emits the matching
  `extern "C"` bodies and `inject_lib` the matching thunks, both off this describer, so JIT and
  AOT cannot disagree about what crosses or what it is called.
- **C sits ON TOP of C++, not under it, and only in the AOT tier.** A standalone context's C++
  methods are DEFINED in the generated `.cpp` and call the AOT functions directly; each
  `extern "C"` entry point is a wrapper over the method beside it. So the C++ API is the wide one -
  every export, in native C++ types, spelled by `aot_standalone` because it owns C++ types the way
  this module owns C ones - and C is the subset C can express, not a ceiling over it. A method
  keeps the daslang name: `[export_c(name = ...)]` renames the C symbol only. `daslang -lib` cannot
  follow, because a jitted library has no C++ source to put underneath: its C++ half stays inline
  proxies over C, so the two tiers' C++ APIs differ ON PURPOSE, and only their C surface is twinned.
- **The generated `.cpp` carries the header's text instead of including it.** The header is the
  HOST's file - it may move it or edit it - and the implementation must not be breakable that way.
  Types are declared once (`type_defs` never reaches the bodies), so inlining is a substitution for
  the include rather than a second copy. A host header that drifts from the library still fails
  loudly: a changed or removed signature is a link error, a changed layout trips the header's own
  size and offset asserts.
- **`Function.flags.exports` is the whole selection truth.** `[export_c]`
  (`ExportCAnnotation`, `daslib/export_c.das` - a das `[function_macro]`, so the C surface is
  decided entirely in daslang) sets it, and
  `policies.export_public_functions` sets it for every public entry-module function under
  `-lib-export-all` (`MarkSymbolUse::exportPublicFunctions`, `src/ast/ast_export.cpp`). The
  selection forms are therefore one bit read several ways: `-lib` alone accepts only what carries
  the annotation, `--jit-lib-export-marked` and a standalone context accept the bit however it was
  set (so `[export]` selects), and `-lib-export-all` is that same acceptance plus the marking
  policy. Accepting the bit however set is what licenses skipping an unspellable signature with a
  warning; one that ASKED for C with `[export_c]` and cannot cross is an error. Whether a signature
  CAN cross is decided here, after infer, because argument types do not exist at annotation time.
- **`[export_c]` reaches a library source with no `require` because `daslib/export_c` is
  `!inscope`.** That marker sets `visibleEverywhere`, which `Module::isVisibleDirectly` honors
  ahead of the require map - the mechanism that makes `daslib/builtin.das` universal. Being visible
  still needs the module LOADED, and `daslib/just_in_time.das` - injected whenever the JIT is on -
  requires it, so `-lib`, `-jit` and `-exe` carry the annotation for free; a compile with no JIT
  needs `require daslib/export_c`. It lives in its own module rather than in `c_api_header` because
  that require costs `ast_boost` alone, not the header emitter's whole graph.
- **Refusal is per stage, not per module**: `collect_c_exports` returns its rejections and logs
  its skips. An `[export_c]` that cannot cross comes back for the caller to report - `macro_error`
  during compilation, the jit error log during codegen - so this module needs no `ProgramPtr` and
  no reporting policy of its own.
- **The scalar widths and the vector layouts are C++-side facts this emitter mirrors.**
  `bool` is one byte (the `static_assert(sizeof(bool)==1)` in `getTypeBaseSize`,
  `src/simulate/debug_info.cpp`), so das `bool` meets C as `bool`. `float3` is `{x, y, z}` at
  12 bytes and 4-byte alignment, because `vec3<TT>` (`include/daScript/misc/vectypes.h`)
  is a plain three-field struct with no `alignas` - the 16-byte vec4f shape is the JIT's
  register ABI, not the memory layout a header has to mirror. So no vector or structure carries
  an alignment attribute, and every declared struct carries a size assert plus one offset assert
  per field. `Structure.sizeOf` is already rounded to the struct's alignment
  (`Structure::getSizeOf`, `src/ast/ast.cpp`), so `sizeof` in C matches it directly.
- **A bound value type crosses as the wrap type its annotation carries.** A
  `ManagedValueAnnotation` is not a ref type and holds `makeValueType()` - the `WrapType<OT>::type`
  das moves the value through, reachable from das as `get_underlying_value_type`. The header
  typedefs that shape under the das type's name and the body assigns the das type through it; a C++
  host still gets the real type, because the C++ half carries the module's `aotRequire` include. It
  is the one C type with an alignment attribute - a handle's alignment is not its wrap type's
  (`BigEntityId` is 16-aligned, four floats are 4-aligned) - on ONE declarator, since the attribute
  applies per declarator. Size and alignment are both asserted. A ref-type handle stays `void *`.
- **An enumeration is a typedef of its base integer plus loose enumerators, never a C
  `enum`** - a C enum's underlying type is implementation-defined, which would break the
  size assert on the 8/16/64-bit bases and on negative values. The values are read off the
  entry's folded constant, so no smart pointer is needed to reach `find_enum_value`.
- **Types are emitted only when a signature reaches them, in post-order.** A by-value
  field's structure is defined before the structure holding it; every structure also gets a
  forward typedef ahead of all definitions, which is what lets a self-referential
  (`Node?`) field compile. A pointer's target only has to be NAMEABLE, so one whose fields C
  cannot spell is forward-declared and never DEFINED - defining it emits fields with no type at
  all - while a representable target is defined, so a host can read through the pointer. A cycle
  terminates either way, and a pointer to something C cannot even name degrades to `void *`
  rather than refusing the function.
- **The same describer writes the daslang twin of the header.** `build_das_bindings`
  (`--jit-lib-bindings`) emits one `[extern(cdecl, late, ...)]` per entry point plus a das struct
  per crossing structure, so a das host requires what the generator wrote instead of hand-writing
  declarations or parsing the C header through clang. `late` is what lets the file compile before
  the library exists, which is what a build system needs to generate it as an ordinary output. It
  re-exports `dasbind` (`require dasbind public`), because the declarations land in that module and
  a host requiring only the bindings would not see them. Types are spelled as das names them -
  `float3`, `range`, `int` for an enumeration - so a host passes `safe_addr` of the real type and
  writes no `reinterpret`; a type das cannot restate as the C ABI carries it degrades that one
  entry point to a comment, not the file. Only `-lib` writes bindings: a standalone context emits
  `extern "C"` bodies for the host to compile IN, with no export decoration, so a shared library
  built from them exports nothing for dasbind to load.
- **A `fixed_array` argument crosses (as `const T *`, which the das ABI already passes) but a
  `fixed_array` result does not** - that is a CMRES of an array, a pointer nothing in C sizes.
