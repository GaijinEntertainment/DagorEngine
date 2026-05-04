# Darg ROBJ_SHADER -- Custom Pixel Shaders in UI

`ROBJ_SHADER` lets you render user-authored pixel shaders as darg UI elements.
Each effect is a single `.hlsl` file that exports one function:

```hlsl
float4 darg_ps(float2 uv)
```

`uv` contains normalized coordinates (0..1) within the element bounds.
Return a `float4(r, g, b, a)` color; alpha is pre-multiplied.


## Built-in Globals

These are available in every darg shader without any declarations:

| Name | Type | Description |
|------|------|-------------|
| `darg_time` | `float` | Playback time in seconds |
| `darg_opacity` | `float` | Element opacity (0..1), set by animations / component tree |
| `darg_resolution` | `float2` | Element size in pixels `(width, height)` |
| `darg_cursor` | `float4` | Cursor position; `xy` = normalized pos (0..1), `-1` if cursor is outside |
| `darg_params0` | `float4` | Custom parameter from script (`shaderParams.params0`) |
| `darg_params1` | `float4` | Custom parameter from script (`shaderParams.params1`) |
| `darg_params2` | `float4` | Custom parameter from script (`shaderParams.params2`) |
| `darg_params3` | `float4` | Custom parameter from script (`shaderParams.params3`) |


## Usage in daRg Script (.nut)

```squirrel
from "dagor.math" import Color4

let myEffect = {
  rendObj = ROBJ_SHADER
  size = [sh(30), sh(20)]

  // Dev path: runtime-compiled HLSL (PC with DX11/DX12 only)
  shaderSource = "my_effect.hlsl"

  // Production path: pre-compiled DSHL shader name (all platforms)
  shaderName = "my_effect"

  // Optional custom parameters (up to 4 x Color4)
  shaderParams = {
    params0 = Color4(1.0, 0.5, 0.0, 1.0)
  }
}
```

**Dual-path loading:**
- `shaderSource` -- path to `.hlsl` file relative to the shaders/source directory.
  Runtime compilation, PC only (DX11/DX12). Takes priority when set.
- `shaderName` -- name of a pre-compiled DSHL shader from the shader dump.
  Works on all platforms. Required for shipping.

During development on PC you can iterate with `shaderSource` alone.
For production builds (or consoles) you need a `.dshl` wrapper and `shaderName`.


## Creating a Production Shader (DSHL)

1. Place your `.hlsl` in `shaders/source/`.

2. Create a `.dshl` wrapper next to it:

```
include "darg_shader_inc.dshl"

shader my_effect
{
  DARG_PIXEL_SHADER_SETUP()

  hlsl(ps) {
    #include "my_effect.hlsl"

    float4 main_ps(VsOutput input) : SV_Target {
      return darg_ps(input.texcoord);
    }
  }
  compile("target_ps", "main_ps");
}
```

3. Add the `.dshl` file to `shaders/shadersList.blk`:
```
file:t = "./source/my_effect.dshl"
```

4. Rebuild shaders with `jam` in the shaders directory.


## Minimal Shader Example

```hlsl
// my_effect.hlsl
float4 darg_ps(float2 uv)
{
  float t = darg_time;
  float3 col = 0.5 + 0.5 * cos(t + uv.xyx * float3(1, 2, 4));
  return float4(col, darg_opacity);
}
```


## Existing Examples

| File | Description |
|------|-------------|
| `darg_test_plasma.hlsl` | Animated plasma with sine waves, optional color tint via `darg_params0` |
| `darg_gradient.hlsl` | Pulsing radial gradient with ring animation |
| `darg_mousetest.hlsl` | Cursor-interactive grid with glow that follows `darg_cursor` |
| `plasma_waves.hlsl` | Converted Shadertoy: grid-based plasma with animated wave lines |
| `menu_effect.hlsl` | Converted Shadertoy: raymarched wave surface with starfield |

Script usage sample: `gamebase/samples_prog/_basic/render/robj_shader.ui.nut`


---

# How to Convert Shadertoy Shaders to Darg HLSL

Shadertoy shaders are written in GLSL and use a specific set of uniforms and conventions.
Darg shaders are HLSL with a different entry point and built-in globals.
Below is a step-by-step conversion guide.


## 1. Entry Point

**Shadertoy:**
```glsl
void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = fragCoord / iResolution.xy;
    // ...
    fragColor = vec4(col, 1.0);
}
```

**Darg HLSL:**
```hlsl
float4 darg_ps(float2 uv)   // uv is already normalized 0..1
{
    // ...
    return float4(col, darg_opacity);
}
```

- `fragCoord` is pixel coordinates; in darg, `uv` is already normalized (0..1).
- To get pixel coordinates when needed: `float2 fragCoord = uv * darg_resolution`.
- Replace `fragColor = ...` with `return float4(...)`.
- Use `darg_opacity` for alpha to respect the UI element's opacity.


## 2. Type Mapping

| GLSL | HLSL |
|------|------|
| `vec2`, `vec3`, `vec4` | `float2`, `float3`, `float4` |
| `ivec2`, `ivec3`, `ivec4` | `int2`, `int3`, `int4` |
| `uvec2`, `uvec3`, `uvec4` | `uint2`, `uint3`, `uint4` |
| `bvec2`, `bvec3` | `bool2`, `bool3` |
| `mat2`, `mat3`, `mat4` | `float2x2`, `float3x3`, `float4x4` |

Constructor syntax changes:
```glsl
// GLSL
vec3 c = vec3(1.0, 0.5, 0.0);
mat2 m = mat2(a, b, c, d);
```
```hlsl
// HLSL
float3 c = float3(1.0, 0.5, 0.0);
float2x2 m = float2x2(a, b, c, d);
```


## 3. Function Mapping

| GLSL | HLSL | Notes |
|------|------|-------|
| `mix(a, b, t)` | `lerp(a, b, t)` | |
| `fract(x)` | `frac(x)` | |
| `mod(x, y)` | `glsl_mod(x, y)` | See note below |
| `atan(y, x)` | `atan2(y, x)` | |
| `texture(sampler, uv)` | -- | No texture inputs; use procedural noise |
| `texelFetch(...)` | -- | Same as above |
| `dFdx(x)` | `ddx(x)` | |
| `dFdy(x)` | `ddy(x)` | |

**Important: `mod` vs `fmod`**

GLSL `mod(x, y)` always returns a non-negative result: `x - y * floor(x/y)`.
HLSL `fmod(x, y)` truncates toward zero and can return negative values.
Define a helper for GLSL-compatible behavior:

```hlsl
float glsl_mod(float x, float y) { return x - y * floor(x / y); }
float2 glsl_mod(float2 x, float2 y) { return x - y * floor(x / y); }
```


## 4. Uniform Mapping

| Shadertoy | Darg | Notes |
|-----------|------|-------|
| `iResolution.xy` | `darg_resolution` | `float2` (width, height) in pixels |
| `iResolution.x / iResolution.y` | `darg_resolution.x / darg_resolution.y` | Aspect ratio |
| `iTime` | `darg_time` | Seconds |
| `iMouse` | `darg_cursor` | Normalized 0..1 (not pixel coords); xy = -1 when outside |
| `iChannel0..3` | -- | No texture channels; use procedural generation |
| `iTimeDelta` | -- | Not available |
| `iFrame` | -- | Not available |
| `iDate` | -- | Not available |


## 5. Coordinate Transforms

Shadertoy's `fragCoord` is in pixel units. Darg's `uv` is normalized 0..1.

Common Shadertoy pattern and its darg equivalent:

```glsl
// Shadertoy: centered coords with aspect ratio correction
vec2 p = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;
```
```hlsl
// Darg equivalent
float2 p = (uv - 0.5) * float2(darg_resolution.x / darg_resolution.y, 1.0);
```

Scaled coordinates:
```glsl
// Shadertoy
vec2 space = (fragCoord - iResolution.xy / 2.0) / iResolution.x * 2.0 * scale;
```
```hlsl
// Darg
float2 space = (uv - 0.5) * 2.0 * scale * float2(1.0, darg_resolution.y / darg_resolution.x);
```


## 6. GLSL Array Initialization

GLSL:
```glsl
const vec4[] bgColors = vec4[](
    vec4(0.1, 0.2, 0.5, 1.0),
    vec4(0.05, 0.1, 0.3, 1.0)
);
```

HLSL:
```hlsl
static const float4 bgColors[2] = {
    float4(0.1, 0.2, 0.5, 1.0),
    float4(0.05, 0.1, 0.3, 1.0)
};
```


## 7. Other Differences

- **`const` locals:** GLSL `const float x = 1.0;` -> HLSL `static const float x = 1.0;` (file scope) or just `float x = 1.0;` (local scope, compiler will optimize).
- **Boolean casts:** GLSL `float(bool_val)` works the same in HLSL.
- **Swizzle:** `.xyzw` and `.rgba` swizzles work in both languages.
- **`#define` macros:** Work the same in HLSL.
- **`out` parameters:** Supported in HLSL, but prefer return values for `darg_ps`.


## 8. Replacing Texture Lookups

Many Shadertoy shaders use `iChannel0` for noise textures. Since darg shaders have
no texture inputs, replace texture-based noise with procedural alternatives:

```hlsl
// Procedural hash (replaces texture-based noise)
float hash12(float2 p)
{
    uint2 q = uint2(int2(p)) * uint2(1597334673u, 3812015801u);
    uint n = (q.x ^ q.y) * 1597334673u;
    return float(n) * (1.0 / 4294967296.0);
}

// Value noise from hash
float value2d(float2 p)
{
    float2 pg = floor(p), pc = p - pg;
    pc *= pc * pc * (3.0 - 2.0 * pc);  // smoothstep
    float2 k = float2(0, 1);
    return lerp(
        lerp(hash12(pg + k.xx), hash12(pg + k.yx), pc.x),
        lerp(hash12(pg + k.xy), hash12(pg + k.yy), pc.x),
        pc.y
    );
}
```


## 9. Conversion Checklist

- [ ] Change function signature to `float4 darg_ps(float2 uv)`
- [ ] Replace `fragColor = ` with `return`
- [ ] Replace all `vecN` types with `floatN`
- [ ] Replace `mix` -> `lerp`, `fract` -> `frac`
- [ ] Add `glsl_mod` helper if `mod()` is used
- [ ] Replace `iTime` -> `darg_time`, `iResolution` -> `darg_resolution`
- [ ] Remove `fragCoord / iResolution` normalization (uv is already 0..1)
- [ ] Replace texture lookups with procedural noise
- [ ] Multiply final alpha by `darg_opacity`
- [ ] Test with `shaderSource = "my_shader.hlsl"` in a darg component
