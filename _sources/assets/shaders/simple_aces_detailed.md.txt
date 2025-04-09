# Shader: simple_aces_detailed

## Overview

This shader is designed for prefabs and render instances with additional
detailing, including a normal map.

## Textures

In *3ds Max*, the shader's class name is `simple_aces_detailed`. It uses the
following texture slots:

- **Slot 1** – Base texture (colormap)
- **Slot 3** – Normal map for the base texture
- **Slot 4** – Normal map for the detail texture
- **Slot 8** – Detail texture, with tiling controlled by the parameters
  `detail_scale_u=8` and `detail_scale_v=8`

## Parameters

For example, in the `.dag` file, the material configuration appears as follows:

```
material{
  name:t="19 – Default"
  class:t="simple_aces_detailed"
  tex16support:b=yes
  twosided:i=0
  amb:ip3=255, 255, 255
  diff:ip3=255, 255, 255
  spec:ip3=255, 255, 255
  emis:ip3=0, 0, 0
  power:r=32
  script:t="real_two_sided=no"
  script:t="detail_scale_u=8"
  script:t="detail_scale_v=8"
  tex0:t="./karelia_cliff_a_b_tex_d.tif"
  tex2:t="./karelia_cliff_a_b_tex_n.tif"
  tex3:t="./rock_detail_a_tex_n.tif"
  tex7:t="./rock_detail_a_tex_d.tif"
}
```

**Explanation of Key Parameters:**

- `name:t="19 – Default"` – The material's name.
- `class:t="simple_aces_detailed"` – The shader class being used for the
  material.
- `tex16support:b=yes` – Enables support for 16-bit textures.
- `twosided:i=0` – Determines whether the material is rendered double-sided. In
  this case, `0` means it's single-sided.
- `amb:ip3=255, 255, 255` – Ambient color (RGB: 255, 255, 255).
- `diff:ip3=255, 255, 255` – Diffuse color (RGB: 255, 255, 255).
- `spec:ip3=255, 255, 255` – Specular color (RGB: 255, 255, 255).
- `emis:ip3=0, 0, 0` – Emission color (RGB: 0, 0, 0), meaning no
  self-illumination.
- `power:r=32` – Specular power (shininess).
- `script:t="real_two_sided=no"` – Explicitly disables double-sided rendering.
- `script:t="detail_scale_u=8"` – Sets the tiling scale for the detail texture
  along the U axis (horizontal).
- `script:t="detail_scale_v=8"` – Sets the tiling scale for the detail texture
  along the V axis (vertical).
- `tex0:t="./karelia_cliff_a_b_tex_d.tif"` – Path to the base texture
  (colormap).
- `tex2:t="./karelia_cliff_a_b_tex_n.tif"` – Path to the normal map for the base
  texture.
- `tex3:t="./rock_detail_a_tex_n.tif"` – Path to the normal map for the detail
  texture.
- `tex7:t="./rock_detail_a_tex_d.tif"` – Path to the detail texture.



