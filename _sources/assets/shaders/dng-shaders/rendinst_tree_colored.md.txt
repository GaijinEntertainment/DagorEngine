# Shader: rendinst_tree_colored

## Overview

A basic shader for rendering vegetation.

## Textures

- **tex0:** Diffuse texture with alpha channel.

## Parameters

Default values are indicated in quotes.

- `script:t="atest=0"` – Enables alpha test.

- `script:t="interactions=0"` – Configures interaction between characters and
  vegetation.
   - `0`: Disabled
   - `1`: Tall plants
   - `2`: Bushes

- `script:t="interaction_strength=0.8"` – Controls the strength of interactions,
  ranging from `0` to `1` (values above `1` might work as well). This is a
  multiplier and is only effective when `interactions` is set to `1` or `2`.
  This is now only in use for vegetation for which `is_pivoted` is `no`.
  For everything else the `interaction_force_max` should be used.

- `script:t="interaction_force_max=3.0` - Sets the maximum calculated force
  for vegetation which uses pivot painter (`is_pivoted` is `yes` -> has
  skinning structure). For these vegetations the interaction force is
  between 0 and `interaction_force_max` based on the obstacle's
  distance from the vegetation's center. This is also only effective
  when `interactions` is set to `1` or `2`.

- `script:t="angle_dissolve=1"` – Toggles angle-based dissolve effects.

- `script:t="spherical_normals=1"` – Applies spherical normals over branch
  normals to smooth sharp edges and reduce low-poly appearance.

  ```{note}
  This feature is rarely used and may behave unexpectedly.
  ```

## Vertex Colors

Unlike in *War Thunder*, the third channel is not used.

- **Red Channel:** Controls fine vertex jitter. The shift occurs along the
  vertex normals but only in the horizontal plane.
- **Green Channel:** Controls oscillation phase. It shifts the amplitude
  oscillation by 90 degrees, randomizing the fine vertex jitter.



