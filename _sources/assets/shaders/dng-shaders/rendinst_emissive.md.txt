# Shader: rendinst_emissive

## Overview

This shader is designed for self-illuminating surfaces.

This shader supports the [flickering
effect](../../lighting/flickering_lights.md).

**Parameters:**

- `use_alpha_for_emission_mask=1` – Uses the alpha channel of the albedo as an
  emission mask. Values: `0`/`1`.
- `emissive_color=1,1,1,1` – Specifies the emissive color and intensity.
  - The first three components represent the color (RGB)
  - The fourth component is the intensity. If the intensity is negative, the
  emission mask is inverted.
- `emission_albedo_mult=1` – Multiplies the emissive color by the albedo color.
  Values range from `0` to `1`.
- `nightly=0` – Emission only occurs at night. Values: `0`/`1`.

## Textures

- **tex0:** Diffuse
- **tex2:** Normals

The alpha channel of the diffuse color texture can be used to store the emission
mask (areas that emit light are white).

<img src="_images/rendinst_emissive_01.jpg" width="49%" class="bg-primary">
<img src="_images/rendinst_emissive_02.jpg" width="49%" class="bg-primary">

## Application

- **Entire Model**: Apply the shader to the entire model if you want widespread
  emissive elements, such as simulating light reflections from a bulb on
  chandelier parts. In this case, using an emission mask is recommended.
- **Specific Parts**: Alternatively, apply the shader only to specific glowing
  parts of the model. The emission mask may not be necessary in this scenario.

By default, the emissive color is derived from the albedo color.

<img src="_images/rendinst_emissive_03.jpg" width="52%" class="bg-primary">
<img src="_images/rendinst_emissive_04.jpg" width="46%" class="bg-primary">

## Parameters

```{important}
This shader often requires custom parameter settings for each asset, rather than
reusing values from other assets. Therefore, it's important to configure the
parameters specifically for your use case. Do not blindly copy parameters from
other assets.
```

- `use_alpha_for_emission_mask=1`

This parameter controls the use of the albedo’s alpha channel as the emission
mask.
  - `1`: The alpha channel of the albedo is used as the emission mask.
  - `0`: The entire geometry with the shader emits light.

<table style="text-align:center; width:98%">
  <tr>
    <th style="text-align:center; width:49%"><p>use_alpha_for_emission_mask=1</p></th>
    <th style="text-align:center; width:48.5%"><p>use_alpha_for_emission_mask=0</p></th></tr>
  <tr>
    <td style="text-align:center; width:49%"><p>Only the areas defined by the albedo's alpha channel will emit light.</p></td>
    <td style="text-align:center; width:48.5%"><p>The entire geometry emits light. The difference in color is due to the non-red parts of the albedo where the emission fades out according to the mask, while the entire geometry emits light (including the grey areas).</p></td></tr>
</table>

<img src="_images/rendinst_emissive_05.jpg" width="49%" class="bg-primary">
<img src="_images/rendinst_emissive_06.jpg" width="48.5%" class="bg-primary">

- `emissive_color=1,1,1,1`

This parameter defines the color and intensity of the emission.

- The first three components are the RGB color of the emission.
- The fourth component is the emission intensity.
  - If positive, the alpha channel of the albedo is used as a direct mask.
  - If negative, the mask is inverted.

By default, all parameters are set to `1`, meaning the albedo color and the
emission mask are used without modification.

```{note}
If a multicolored emission is needed for a single material (e.g., a stained
glass window), it is recommended to set the color in the diffuse texture and
multiply it with a white emissive color.
```

```{important}
Be sure to observe the differences between the [*Asset
Viewer*](../../../dagor-tools/asset-viewer/asset-viewer/asset_viewer.md) and the
in-game results. This discrepancy underscores the importance of verifying and
fine-tuning your work by reviewing it directly within the game environment.
```

Examples with white albedo (as it is easier to override compared to the bright
red in previous examples):

<table style="text-align:center; width:98%">
  <tr>
    <th style="text-align:center; width:50%"><p>emissive_color=0,1,0,1</p></th>
    <th style="text-align:center; width:48%"><p>emissive_color=0,1,0,10</p></th></tr>
  <tr>
    <td style="text-align:center; width:50%"><p>Green emissive color is applied. In the Asset Viewer, it is barely noticeable, but it is very strong in-game.</p></td>
    <td style="text-align:center; width:48%"><p>A strong green emission is applied. The glow effect in-game is very prominent.</p></td></tr>
</table>

<img src="_images/rendinst_emissive_07.jpg" width="50%" class="bg-primary">
<img src="_images/rendinst_emissive_08.jpg" width="48%" class="bg-primary">

<img src="_images/rendinst_emissive_09.jpg" width="50%" class="bg-primary">
<img src="_images/rendinst_emissive_10.jpg" width="45%" class="bg-primary">

```{important}
Avoid excessive emission intensity. It directly impacts the glow effect, as seen
in the screenshots above. Fine-tune the emission carefully for each asset.
Excessive emission not only looks unnatural but can also negatively affect
gameplay by interfering with camera adaptation. Aim for minimal, but noticeable,
glow.

The glow effect also depends on the size of the emitting geometry. For example,
thicker lanterns have a stronger glow than thinner ones, not only because of
different emission intensities but also due to their larger size.

<img src="_images/rendinst_emissive_11.jpg" width="80%" align="center" class="bg-primary">

<br>

In the screenshot above, the glow effect on the thick lanterns is excessive, but
reducing the values further made the glow barely visible – thus, the current
setting was kept. The glow effect on the thin lanterns is adequate, with room
for a slight increase, but not much.
```

- `emission_albedo_mult=1`

This parameter controls the multiplication of the emissive color by the albedo
color, as defined by the `emissive_color` shader parameter. Essentially, it acts
like the "Multiply" blend mode in *Photoshop*.

Values range from `0` to `1`, where:

- `0`: No multiplication. Only the shader's emissive color is used.
- `1`: Full multiplication with the albedo color.

```{important}
When multiplying drastically different colors (e.g., red and green), the result
can be a complete absence of emission since (0,1,0) * (1,0,0) = (0,0,0). Avoid
this scenario.
```

<table style="text-align:center; width:96%">
  <tr>
    <th style="text-align:center; width:32%"><p>
      emission_albedo_mult=0<br>
      emissive_color=0,1,0,1</p></th>
    <th style="text-align:center; width:32%"><p>
      emission_albedo_mult=0.5<br>
      emissive_color=0,1,0,1</p></th>
    <th style="text-align:center; width:32%"><p>
      emission_albedo_mult=1<br>
      emissive_color=0,1,0,1</p></th></tr>
  <tr>
    <td style="text-align:center; width:32%"><p>No multiplication with albedo color. The shader’s emissive color is green. Results in a green emissive layer over the red albedo, leading to a yellow color in the Asset Viewer. The in-game result is correct.</p></td>
    <td style="text-align:center; width:32%"><p>Multiplies the albedo color by 50%. The shader’s emissive color is green. The in-game glow intensity is reduced by half due to the interaction of two contrasting colors.</p></td>
    <td style="text-align:center; width:32%"><p>Full multiplication with the albedo color. The shader’s emissive color is green. Almost no emission is visible since (0,1,0) * (1,0,0) = (0,0,0). Increasing emission intensity (e.g., to 10 or 100) may improve visibility, but this is not a proper solution and will likely not look correct.</p></td></tr>
</table>

- **Asset Viewer:**

<img src="_images/rendinst_emissive_12.jpg" width="32%" class="bg-primary">
<img src="_images/rendinst_emissive_13.jpg" width="32%" class="bg-primary">
<img src="_images/rendinst_emissive_14.jpg" width="32%" class="bg-primary">

- **Game - Result:**

<img src="_images/rendinst_emissive_15.jpg" width="32%" class="bg-primary">
<img src="_images/rendinst_emissive_16.jpg" width="32%" class="bg-primary">
<img src="_images/rendinst_emissive_17.jpg" width="32%" class="bg-primary">

- **Game - Albedo:**

<img src="_images/rendinst_emissive_18.jpg" width="32%" class="bg-primary">
<img src="_images/rendinst_emissive_19.jpg" width="32%" class="bg-primary">
<img src="_images/rendinst_emissive_20.jpg" width="32%" class="bg-primary">

- **Game - Emissive:**

<img src="_images/rendinst_emissive_21.jpg" width="32%" class="bg-primary">
<img src="_images/rendinst_emissive_22.jpg" width="32%" class="bg-primary">
<img src="_images/rendinst_emissive_23.jpg" width="32%" class="bg-primary">

- `nightly=1`

This parameter controls whether the emissive effect is active only during
nighttime.

Values:

- `0`: By default, the emissive effect is always active.
- `1`: The emissive effect is only active during nighttime. The definition of
  "night" is controlled by scripts elsewhere and is not influenced by this
  parameter.



