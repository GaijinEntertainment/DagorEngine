# Contributing to Compiler

This section describes the rules of versioning and why they are important.

## How Compiler Works

Dagor Shader Compiler (DSC) is used to create compiled shader dumps (which are separate for each specific driver and shader model). For a given compiler run, the end result is usually a single `<name><driver>.[bindless.]<shader model>.shdump.bin` file and (optionally) a dynamic cpp stcode library. To reduce compile time, intermediate files are cached and reused in future runs:

- SHA cache (binary): located in `<intermediate dir>/../../shaders_sha~<platform>-O<optimization level>[-bindless]/bin`, contains driver-specific binaries compiled for specific shaders. Reused if exactly the same HLSL (or HLSL-like) code is compiled in future compiler runs.

- SHA cache (source): located in `<intermediate dir>/../../shaders_sha~<platform>-O<optimization level>[-bindless]/src`, may contain the final HLSL (or HLSL-like) source code for a specific shader to be compiled. References the binary cache file if it was generated.

- `.obj` files: located in `<intermediate dir>`, contain full compiler output for a given `.dshl` file. Reused if the `.dshl` file is not modified since the last compiler run.

Here is a list of the available `<driver>` values:
  - `DX12x` - Xbox One platform
  - `DX12xs` - Xbox Series platform
  - `DX12` - DirectX 12 API
  - `SpirV` - Vulkan API
  - `PS4` - PlayStation 4 platform
  - `PS5` - PlayStation 5 platform
  - `MTL` - Metal API
  - empty string - DirectX 11 API

The input `.blk` **MUST** have the variable `outDumpName:t="<dump path>/< name><driver>"`.

The presence of `[bindless]` is defined by the DSC flag `-enableBindless:on`.

`<intermediate dir>` is defined by the DSC flag `-o`.

## Versioning

Dagor Shader Compiler has 3 different “versions”, each of which must be updated separately:

- If generated HLSL (or HLSL-like) code **MAY** produce a bytecode different from the previous version (for example, if you updated libs for HLSL compiler), the corresponding `sha1_cache_version` **MUST** be updated in `prog/tools/ShaderCompiler2/sha1_cache_version.h`, because it is used for cache in `_output/shaders_sha~<platform>-O<optimization level>[-bindless]/`

- If changes require rebuilding of all `.obj` (if `.dshl` file **MAY** produce `.hlsl` different from the previous version or stcode generation was changed) for

  - some platforms, the corresponding `_MAKE4C('version')` **MUST** be updated in `prog/tools/ShaderCompiler2/ver_obj_<platform>.<h or cpp>`

  - all platforms, `SHADER_CACHE_COMMON_VER` **MUST** be updated in `prog/tools/ShaderCompiler2/shCacheVer.h`

- If you implement some new feature or fix something, `CompilerConfig::version` **MUST** be updated in `prog/tools/ShaderCompiler2/globalConfig.h`. It is only used for logging.