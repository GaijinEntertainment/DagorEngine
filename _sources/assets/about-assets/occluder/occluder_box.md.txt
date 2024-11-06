# Occluder Box

## Overview

An *occluder box* is a specialized geometry used to improve performance. Every
object that fully obscured by occluder box is excluded from rendering. Partially
obscured objects stay intact.


If any part of an object extends beyond the occluder box,
it will reappear: see [video
1](https://drive.google.com/file/d/1JOlqzVnGlJNNe9yyexRqOIjiz75PWi74/view?usp=sharing)
and [video
2](https://drive.google.com/file/d/1B6ZvtDnbLyorXTSYhPW4ooVFcv0oTCC_/view?usp=sharing)
for reference.

## Creating an Occluder Box

To create an occluder box for an object, follow these steps:

1. Create a Box object.
2. Name the object `occluder_box`.
3. Create a material using the *Dagor Dagorrat Material 2* shader.

   <img src="_images/occluder_box_01.jpg" width="40%" align="center" class="bg-primary">

   <br>

4. Set the shader class to `gi_black` (for better visibility, it's
   recommended to choose a material color that contrasts with the object, like
   bright red).

   <img src="_images/occluder_box_02.jpg" width="49%" class="bg-primary">
   <img src="_images/occluder_box_03.jpg" width="49%" class="bg-primary">

   ---

5. Assign the material to the object named `occluder_box`.
6. Open the object's properties and uncheck the **Renderable** option (this will
   prevent potential issues later on).

   <img src="_images/occluder_box_04.jpg" width="61%" class="bg-primary">
   <img src="_images/occluder_box_05.jpg" width="29%" class="bg-primary">

   ---

7. In the **User Defined** tab, input the following:

   ```
   renderable:b=no
   cast_shadows:b=no
   occluder:b=yes
   collidable:b=no
   ```

   <img src="_images/occluder_box_06.jpg" width="30%" align="left" class="bg-primary">

   **User Defined Properties:**

   - `renderable:b=no` – Disables the object from being rendered.
   - `cast_shadows:b=no` – Prevents the object from casting shadows.
   - `occluder:b=yes` – Designates the object as an occluder.
   - `collidable:b=no` – Disables collision generation.

   <br clear="left">

   ```{important}
   Unchecking the **collidable** option in the Dagor object properties does not
   achieve the same result!

   To verify this, check the asset in the [*Asset
   Viewer*](../../../dagor-tools/asset-viewer/asset-viewer/asset_viewer.md). If
   the occluder box is visible in the collision data, the setup is incorrect.
   The collision should not include an `occluder_box` node.

   <img src="_images/occluder_box_07.jpg" width="80%" align="center" class="bg-primary">

   ```

8. Convert the `occluder_box` object to `Editable Poly` or apply an `Edit Poly`
   modifier.
9. Adjust the size to fit the object it should occlude. Specific size
   requirements are detailed below.

## Editing the Occluder Box

The *occluder box* is aptly named because only its bounding box matters,
regardless of its orientation. Therefore, the following rules apply when
creating an occluder box:

1. The occluder box should never extend beyond the object's geometry.

   <img src="_images/occluder_box_08.jpg" width="39.5%" class="bg-primary">
   <img src="_images/occluder_box_09.jpg" width="58.5%" class="bg-primary">

   ---

2. It should fill as much of the object's internal space as possible, similar to
   UV mapping, without exceeding the object's boundaries.

   <img src="_images/occluder_box_10.jpg" width="30%" align="center" class="bg-primary">

   <br>

3. Rotating the occluder box is highly discouraged.

   Here is an example of an unrotated occluder box:

   <img src="_images/occluder_box_11.jpg" width="68%" class="bg-primary">
   <img src="_images/occluder_box_12.jpg" width="30%" class="bg-primary">

   <br>

   And here's how it looks when rotated:

   <img src="_images/occluder_box_13.jpg" width="44.5%" class="bg-primary">
   <img src="_images/occluder_box_14.jpg" width="53.5%" class="bg-primary">

   ---

4. **Do not** name the object anything other than `occluder_box`.
5. The occluder box should be placed in the `LOD00` layer.

```{tip}
If you need to remove an occluder box, delete its polygons while retaining the
object and its properties as described.
```

## Verifying Occluder Box Setup

There are several ways to verify if the occluder box is set up correctly:

1. In [*Asset
   Viewer*](../../../dagor-tools/asset-viewer/asset-viewer/asset_viewer.md), the
   occluder box is displayed as a white rectangle.

   If it's not visible, ensure the proper setting is enabled. If the occluder
   box appears misaligned (as shown in the image), you will need to correct it
   in *3ds Max*.

   <img src="_images/occluder_box_15.jpg" width="80%" align="center" class="bg-primary">

   <br>

2. During in-game testing, you can observe the occluder box in action by
   pressing `F3` to display the wireframe, see
   [video](https://drive.google.com/file/d/1JOlqzVnGlJNNe9yyexRqOIjiz75PWi74/view?usp=sharing)
   for reference.

   If objects behind the occluder do not disappear, disappear too early, too
   late, or not at all, recheck the setup based on the guidelines above. If the
   occluder box is incorrectly configured, you will likely encounter issues
   similar to those shown in [video
   1](https://drive.google.com/file/d/11DACcju6HsO-XCd0W82-qG-AouQRvGN-/view?usp=sharing)
   and [video
   2](https://drive.google.com/file/d/15J1qdlZV3IpcrAudYAyTK4kJU-DoYdTB/view?usp=sharing).

## Examples of Well-Configured Occluder Boxes

Here are several example images of correctly set up occluder boxes.

<img src="_images/occluder_box_16.jpg" width="32%" class="bg-primary">
<img src="_images/occluder_box_17.jpg" width="32%" class="bg-primary">
<img src="_images/occluder_box_18.jpg" width="32%" class="bg-primary">

<img src="_images/occluder_box_19.jpg" width="44%" class="bg-primary">
<img src="_images/occluder_box_20.jpg" width="54%" class="bg-primary">

## Commands for Displaying Occluders

- `render.debug occlusion` – Shows occluders and their statistics.
- `render.debug occlusion occluded` – Displays bounding boxes of occluded
  objects.

Toggle commands:
- `render.debug occlusion boxes`
- `render.debug occlusion stats`
- `render.debug occlusion occludedv`
- `render.debug occlusion not_occluded`

Additionally, you can use:
- `occlusion.enabled` – Enables or disables occlusion rendering entirely.
- `rendinst.check_occlusion` – Enables or disables occlusion for render
  instances.


