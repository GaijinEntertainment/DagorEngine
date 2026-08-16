# dsc2 file map

Subsystem-by-subsystem map of prog/tools/ShaderCompiler2. Only files
whose role is not obvious from the name, or that carry a non-obvious
contract, get their own line; trivial helpers are grouped.

Driver and diagnostics:
- `main.cpp` / `winmain_con.cpp` - CLI entry, config BLK parsing; the
  debug log name (`ShaderLog-<target>`) is chosen in winmain_con.
- `shCompiler.*` - per-source orchestration (`compileShader`,
  `buildShaderBinDump`), job scheduling.
- `globalConfig.*` - global flags (`shc::config()`), including cppStcode
  mode.
- `processes.*` - worker process management for `-cjN` parallel compile.
- `shCompContext.h`, `shCompilationInfo.*` - state/data for the whole
  compilation.
- `compileResult.h` - what a platform backend returns from one HLSL
  compilation.
- `gitRunner.*` - last-commit timestamps of sources, used for cache
  validation; returns 0 when the tree is dirty.
- `sh_stat.*`, `shErrorReporting.h`, `shLog.*`, `DebugLevel.h`,
  `optimizationLevel.h` - stats and diagnostics.

Front-end (parsing):
- `shlexterm.*` - hand-maintained parser driver.
- `loadShaders.*` - loads shader obj files (not parsing).
- `shlex.dlp` / `shsyn.whl` - dolphin/whale grammar sources for the
  DSHL lexer/parser; outputs (`shlex.*`, `shsyn.*`, `shsyntok.h`) are
  regenerated automatically by jam (`jamfile-parser-gen`), or manually
  via `mkd.bat` / `mkw.bat` (`mk_cond.bat` for the condition parser).
  Edit the .dlp/.whl, not the generated .cpp.
- `parser/` - base lexer/parser framework (bparser, base_lex, base_par).

Variants:
- `shaderVariant.*`, `shaderVariantSrc.*` - static/dynamic variant type
  tables.
- `variantSemantic.*`, `variantAssembly.*` - per-variant semantic and
  assembly functionality.
- `shAssumes.*` - `assume` handling, prunes impossible variants.
- `intervals.*` - interval-valued dynamic variables.
- `gatherVar.*`, `variablesMerger.*` - collect/merge the variables a
  shader uses.
- `shVariantContext.h` - variant evaluation context.

Semantic layer:
- `shSemCode.*` - the semantic IR (`ShaderSemCode`); helpers in
  `shaderSemantic.*`, `semUtils.*`.
- `hwSemantic.*`, `hwAssembly.*` - target-hardware-specific semantic
  and assembly handling.
- `preshaderCompilation.*` - CPU-evaluated preshader code.
- `namedConst.*`, `samplers.*`, `hlslRegisters.*` - cbuffer constants,
  sampler state, register bookkeeping.
- `hlslStage.h`, `const3d.h` - stage enums, const-table limits/types.
- `shCode.cpp`/`shcode.h`, `shFunc.*`, `shLocVar.*`, `varTypes.h`,
  `varMap.h`, `shVarBool.h`, `shVarVecTypes.h` - code containers,
  functions/locals, variable types.

Global vars and link:
- `globVar*.cpp/h`, `globvar.cpp` - global variable table
  (accumulating/tracking).
- `globVarSem.*` - compilation callbacks for most global DSHL
  declaration entities.
- `shShaderContext.h`, `shTargetContext.h`, `shTargetStorage.*` -
  per-target storage of compiled shader classes.
- `linkShaders.*` - final link of variants into shader classes.

Codegen and backends:
- `codeBlocks.*` -> `assemblyShader.*` - per-variant code assembly.
- `hlslCompiler/<target>.cpp` - emits final HLSL and invokes the
  platform compiler (dxc/fxc) per target; `settings.cpp` holds shared
  settings.
- `dx12/`, `hlsl2spirv/`, `hlsl2metal/`, `hlsl11transcode/`,
  `ps4transcode/`, `ps5transcode/` - backend transcoders (see the
  Pipeline section in CLAUDE.md); shared helpers in
  `transcodeShader.*`, `transcodeCommon.h`.
- `ver_obj_*.h` - per-target object-format version stamps; bump to
  invalidate stale caches.

Stcode (state-setting code):
- `cppStcode.*`, `cppStcodeAssembly.*`, `cppStcodeBuilder.h`,
  `cppStcodePasses.*`, `cppStcodeUtils.h`, `cppStcodePlatformInfo.h` -
  C++ stcode generation and its optimization passes.
- `_stcodeTemplates/` - .fmt templates for the generated C++.
- `stcodeBytecode.h` - interpreted bytecode stcode format.
- `refinedBlockLayout.*`, `refinedBlockRegisterAllocator.*` - const
  block layout and register allocation for stcode.
- `make_cppstcode_debuginfo_zip.py` - packs cpp stcode debug info
  (`-cppStcodeSaveDebugInfoAndSourcesToZip`).

Bindump, serialization, cache:
- `makeShBinDump.*` - bindump writer (output consumed by
  prog/engine/shaders); helpers in `binDumpUtils.*`, `shaderSave.*`.
- `shaderBytecodeCache.*` - runtime bytecode cache.
- `shCacheVer.h`, `sha1_cache_version.h` - version stamps for the
  compile-time sha1 bytecode cache.
- `deSerializationContext.*` - context required by bindump
  (de)serialization.
- `encodedBits.h`, `hash.h`, `hashStrings.h`, `hashed_cache.h`,
  `blkHash.h` - bit packing and hashing (blk hash drives rebuild
  checks).
- `nameMap.h` (`SCFastNameMap`), `shaderTab.h` (`TabVpr`/`TabFsh`/...),
  `commonUtils.h`, `str.h`, `defer.h`, `fast_isalnum.h` - common
  containers/utilities.
