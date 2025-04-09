# Shader: rendinst_flag_layered

## Overview

This shader is designed for fabric animation with detail blending based on
vertex color.

## Parameters

- `script:t="frequency_amplitude=1,1,1,1"` – Sets the random frequency and
  amplitude intervals.
- `script:t="wave_length=1"` – Defines the wavelength.
- `script:t="wind_direction=0,0,1,0"` – Specifies the wind direction.

- `script:t="details_tile=1,1,1,0"` – Controls the tiling of the details.
- `script:t="invert_heights=0,0.5,0,0"` – Inverts the heightmaps of the details
  (can be fractional).

- `script:t="paint_details=1,0,0,1"` – Specifies the coloring of the details.
- `script:t="overlay_color_from=0,0,0,0"` – Defines the "from" and "to" colors
  for blending via linear interpolation (lerp) using the blue channel of the
  vertex color as the mask.
- `script:t="overlay_color_to=0,0,0,0"` – Specifies the color range from `0`
  (RGB value `0`) to `1` (RGB value `255`).

- `script:t="micro_detail_layer=0"` – Parameters for micro details.
- `script:t="micro_detail_layer_v_scale=1"`
- `script:t="micro_detail_layer_uv_scale=1"`

- `script:t="atest=1"` and `script:t="details_alphatest=0,0,0,0"` – Work
  together. The first parameter enables alpha test, while the second defines the
  alpha test strength for each detail.

## Functionality

This shader operates on the same principles as its parent shaders:

- Shader [rendinst_flag_colored](rendinst_flag_colored.md) – Controls the
  animation behavior.
- Shader [rendinst_vcolor_layered](rendinst_vcolor_layered.md) – Manages detail
  painting, overlay, alpha test, tiling, and heightmap inversion.

All these parameters are fully described in the relevant shader documentation,
so they will not be repeated here – refer to the corresponding sections for
details.

The only key difference lies in the channels used for detail blending. Since
this shader handles not just three details but also includes animation, the
blending is controlled by different channels than in
[rendinst_vcolor_layered](rendinst_vcolor_layered.md).

- **Red channel** – Controls animation.
- **Green channel** – Subtracts the second detail from the first.
- **Blue channel** – Subtracts the third detail from the first two.

```{important}
The blue channel also controls the overlay (see
[rendinst_vcolor_layered](rendinst_vcolor_layered.md)). Therefore, you must
decide which aspect is more important for a specific asset – overlay or an
additional detail.

If the overlay is needed, assign the same detail to the blue channel as you use
for the first (or second) detail, depending on which detail takes priority.
```


