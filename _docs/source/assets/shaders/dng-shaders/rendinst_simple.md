# Shader: rendinst_simple / simple_aces

## Overview

A basic shader used for objects that require a texture along with a normals,
metalness, and smoothness map. It supports alpha test and
[microdetails](../../about-assets/microdetails/microdetails.md).

```{note}
Technically, this is the *rendinst_simple* shader. However, it may be referred
as "simple_aces" on old assets, but the game interprets both as *rendinst_
simple*.
```

<img src="_images/rendinst_simple_01.jpg" width="54%" class="bg-primary">
<img src="_images/rendinst_simple_02.jpg" width="44%" class="bg-primary">

<br>

## Textures

The shader uses the first UV channel for mapping.

- **tex0:** – Diffuse albedo map
- **tex2:** – Texture combining normals, metalness, and smoothness:
  - **RG** – Normal map
  - **B** – Metalness
  - **Alpha** – Smoothness

<img src="_images/rendinst_simple_03.jpg" width="75%" align="center" class="bg-primary">

<br>

## Parameters

<img src="_images/rendinst_simple_04.jpg" width="75%" align="center" class="bg-primary">

<br>

The shader is configured using the following parameters:

- **"2 sided" checkbox** ("two_sided" in [*Asset
  Viewer*](../../../dagor-tools/asset-viewer/asset-viewer/asset_viewer.md)) –
  The original, outdated method for double-sided geometry. It works imperfectly
  with shadows and is slightly more expensive with small triangle counts, but
  performs well and is cost-effective for large triangle counts. Currently, it's
  only used for trees and not for environment assets (though older assets may
  still have it, as this was the only option previously).

- **"2 sided real" checkbox** ("real_two_sided" in [*Asset
  Viewer*](../../../dagor-tools/asset-viewer/asset-viewer/asset_viewer.md)) –
  The second method for double-sided geometry. It duplicates and flips the
  geometry where the material is applied (one triangle becomes two). This method
  works correctly with shadows but doubles the number of triangles to which the
  material is applied.

  ```{important}
  Avoid overusing this parameter or duplicating existing materials just to apply
  it to parts of an asset. If a texture set is used and part of the geometry
  needs to be double-sided while another part remains single-sided, it's more
  efficient to create a single material for the entire model with this texture
  set and handle the double-sided geometry by flipping it directly in the LODs.
  This approach is cheaper than using two nearly identical materials and
  provides the same visual result.
  ```

- **atest** – Alpha channel cutoff threshold. A switch for transparency based on
  the diffuse's alpha channel. The mask is compressed to a single bit: anything
  darker than `127` becomes transparent, and the rest is opaque.

  ```{note}
  Although the **atest** parameter defaults to `127`, it functions as a Boolean
  switch. If the value is greater than `0`, transparency is enabled; otherwise,
  it's not used. The threshold cannot be adjusted.

  A smooth gradient from black to white will be cut off at the same point,
  regardless of whether **atest** is set to `1`, `255`, or `1000`.
  ```

- **use_painting** – Controls painting from the palette defined in the shader
  variables of the scene.
  - `1`: Enable painting
  - `0`: Disable painting
  - Values between `0.(0)1` and `0.(9)`: Apply partial painting (acts as a
    multiplier). Painting is applied based on the diffuse alpha channel
    multiplied by the **use_painting** value. Values from `1.(0)1` to `1.(9)`
    also influence the strength of the paint from `0.0` to `1.0` but disable the
    random pixel selection from the painting stripe when the object is offset in
    height. This ensures consistency in colors, for instance, when painting
    modular skyscrapers the same color. The palette used is named
    `paint_colors.dds` and is located here:
    `<project_name>/develop/assets/textures/colorize_textures`.

  ```{seealso}
  For more information, see [Procedural Rendinst
  Painting](../../about-assets/procedural-rendinst-painting/procedural_rendinst_painting.md).
  ```

- **painting_line** – The painting stripe (from `0` to the last stripe in the
  texture).

- **micro_detail_layer** – The index of the microdetail layer (from `0` to
  `11`).

  ```{seealso}
  For more information, see [Microdetails on
  Assets](../../about-assets/microdetails/microdetails.md).
  ```

- **micro_detail_layer_v_scale** – Vertical scale of the microdetail.

- **micro_detail_layer_uv_scale** – UV scale of the microdetail texture. The
  larger the value, the smaller the detail pattern will appear (range: `0` -
  infinity).




