# Setting up shader compilation for a project

How to wire up dsc2 shader compilation for a game/sample project.

Compile via the per-target exe and the project's config BLK (`myProject`
below stands for your game/sample directory; exe paths are relative to
its `shaders/` dir):

```bash
cd prog/samples/myProject/shaders
# DX11:
../../../../tools/dagor_cdk/windows-x86_64/dsc2-hlsl11-dev.exe ./shaders_pc11.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr -o output_dir
# DX12:
../../../../tools/dagor_cdk/windows-x86_64/dsc2-dx12-dev.exe ./shaders_dx12.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr -o output_dir
```

Most samples and games have bat files for this, e.g.
`prog/samples/myProject/shaders/compile_shaders_dx12.bat`.
The prebuilt compilers ship at `tools/dagor_cdk/windows-x86_64/`
(also dx12, spirv, metal variants).

## shader_global.dshl

Every project needs a `shader_global.dshl` in its shaders/source/
directory. It MUST include alpha test macros (`NO_ATEST`, `USE_ATEST_1`,
`USE_ATEST_255`, `USE_ATEST_HALF`, `USE_ATEST_VALUE`,
`USE_ATEST_DYNAMIC_VALUE`) because `gui_default.dshl` from commonShaders
depends on them. Copy from
`prog/samples/testBed/shaders/source/shader_global.dshl` as a starting
point.

Required includes in shader_global.dshl:
```dshl
include "hardware_defines.dshl"    # From gameLibs/render/shaders - provides VS_OUT_POSITION, etc.
include "postfx_inc.dshl"          # From gameLibs/render/shaders
include "mulPointTm_inc.dshl"      # From gameLibs/render/shaders
```

## Shader config BLK (shaders_pc11.blk)

```blk
shader_root_dir:t="."
outDumpName:t="../../../game/compiledShaders/game"
incDir:t="../../../gameLibs/render/shaders"    # hardware_defines.dshl, postfx_inc.dshl
incDir:t="../../commonShaders"                  # gui_default.dshl
source {
  includePath:t="./source"
  include shadersList.blk
}
Compile {
  fsh:t="5.0"
  assume_vars { }
}
```

## DSHL vertex channels

Map to HLSL semantics:
```dshl
channel float3 pos = pos;        # POSITION
channel float3 tc[0] = tc[0];    # TEXCOORD0
channel float3 tc[1] = tc[1];    # TEXCOORD1
channel float2 tc[0] = tc[0];    # TEXCOORD0 (float2)
channel color8 vcol = vcol;      # COLOR0
```

Corresponding C++ vertex declarations use `VSD_REG(VSDR_POS, VSDT_FLOAT3)`,
`VSD_REG(VSDR_TEXC0, VSDT_FLOAT3)`, etc. from `drv/3d/dag_consts_base.h`.
