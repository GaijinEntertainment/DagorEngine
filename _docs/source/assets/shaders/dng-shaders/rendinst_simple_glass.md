# Shader: rendinst_simple_glass

## Overview

This shader can be assigned as `glass` when setting materials in *3ds Max*.

For dynamic models, it is remapped to `dynamic_simple_glass`.

## Parameters

- `script:t="emission=X"` – Activates emission during dark hours, `X` is the
  brightness of the emission.

- `script:t="is_window=1"` – A common parameter across most glass shaders.

In this shader, the `is_window` parameter is **enabled by default** since the
majority of assets using this shader are window glass.

This parameter should be enabled for glass objects located at the boundary of an
`envi_probe`, meaning the glass needs to reflect two different environments:

- The interior reflection, baked into the `envi_probe`.
- The exterior reflection, typically reflecting the outside environment.

The parameter should be **disabled** for all glass objects used either entirely
inside or outside a building. Examples include glassware, lamp covers, vehicle
windows, etc.

```{seealso}
For more information on setting up emission, see
[rendinst_emissive](./rendinst_emissive.md).
```


