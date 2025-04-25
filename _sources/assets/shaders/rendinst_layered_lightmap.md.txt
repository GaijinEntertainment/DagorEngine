# Shader: rendinst_layered_lightmap

## Overview

This is a self-illumination shader designed for objects with blended detail
textures.

It works exactly like [rendinst_layered](rendinst_layered.md) shader, but with
one key difference: the 8th slot (numbering starts from 0, so this corresponds
to the 7th slot in *3ds Max*) is used for the emission mask. The shader uses
this mask to control the emission effect, with the color of the mask determining
the color of the illumination.

The second UV channel is used, similar to an overlay.

<table style="text-align:center; width:96%"><tr>
  <th style="text-align:center; width:25%"><p>Overlay</p></th>
  <th style="text-align:center; width:25%"><p>Emission Mask</p></th>
  <th style="text-align:center; width:45%"><p>Result</p></th></tr>
</table>

<img src="_images/rendinst_layered_lightmap_01.jpg" width="25%" class="bg-primary">
<img src="_images/rendinst_layered_lightmap_02.jpg" width="25%" class="bg-primary">
<img src="_images/rendinst_layered_lightmap_03.jpg" width="45%" class="bg-primary">

<img src="_images/rendinst_layered_lightmap_04.jpg" width="25%" class="bg-primary">
<img src="_images/rendinst_layered_lightmap_05.jpg" width="25%" class="bg-primary">
<img src="_images/rendinst_layered_lightmap_06.jpg" width="45%" class="bg-primary">


