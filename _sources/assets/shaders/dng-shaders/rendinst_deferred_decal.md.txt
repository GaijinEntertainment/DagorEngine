# Shader: rendinst_deferred_decal

## Overview

`rendinst_deferred_decal` is a shader for projection decals with a normal map.

It is a more performance-intensive version of the
[rendinst_blend_diffuse_decal](./rendinst_blend_diffuse_decal.md) shader, as it
adds extra checks to prevent the decal from being projected "in the air" when
there is no geometry behind it.

