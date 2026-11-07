# darg_fullscreen_background_shaders

Fullscreen daRg `ROBJ_SHADER` shaders for screen backgrounds: login, loading
and menu screens. The lib holds shaders only, in `shaders/`. Their output is
premultiplied alpha, as for every `darg_ps`.

## Use the lib

- A game with `setup_libs.py`: add `"render/darg_fullscreen_background_shaders"`
  to its `gamelibs` list and run the script, which puts `_shaders.blk` into the
  generated `libs_shaders.blk`.
- Any other shader list: include `_shaders.blk`.

The shader config needs `prog/gameLibs`, `prog/gameLibs/daRg/shaders` and
`prog/gameLibs/render/shaders` as `incDir`. Show a shader with
`rendObj = ROBJ_SHADER` and its name in `shaderName`, for example
`shaderName = "atmo_fog"`.

## Atmosphere overlays

The `atmo_*` shaders give a still background image a slow, subtle motion. Each
one is a transparent layer that you put on top of the image. A shader cannot
read the image, so it only adds light or haze. Put two layers on top of each
other for a mix, for example fog under embers. `shaders/atmo_darg.dshl`
declares the six shaders, each `atmo_*_darg.hlsl` holds one effect, and
`atmo_common.hlsl` the helpers they share.

| Shader | Effect | Default tint |
|--------|--------|--------------|
| `atmo_embers` | Sparks that go up, sway in the wind and cool down, plus a weak firelight flicker from below | orange |
| `atmo_ash` | Ash flakes that fall, turn and sway; some near flakes are out of focus | dark grey |
| `atmo_dust` | Dust motes that float and glint, brighter in slow light shafts; out of focus motes near the camera | warm white |
| `atmo_fog` | Low mist that moves to the side in two depth layers and slowly gets thicker and thinner | cold grey |
| `atmo_smoke` | Thin smoke that goes up and curls | grey brown |
| `atmo_rain` | Thin slanted rain streaks in three depth layers, in gusts, with a rare faint lightning | cold white |

All six use the same parameter layout. A zero value gives the default:

| Parameter | x | y | z | w |
|-----------|---|---|---|---|
| `params0` | tint r | tint g | tint b | strength (1) |
| `params1` | density (1) | speed (1) | size (1): particle radius (embers, ash, dust, rain) or pattern scale (fog, smoke) | extra (1): firelight (embers), light shafts (dust), lightning (rain); a negative value turns it off |
| `params2` | fade from x (uv) | fade to x (uv) | strength that stays after the fade | - |
| `params3` | wind, positive to the right (embers, ash, smoke, fog, rain) or light direction x (dust) | fog top y (fog) or light direction y (dust) | - | seed: moves the pattern, gives each use a different look |

`params2` fades the effect out to the right, where a login panel can be.
For example `Color4(0.6, 0.95, 0.25, 0)` keeps 25% of the effect at the right
edge.
