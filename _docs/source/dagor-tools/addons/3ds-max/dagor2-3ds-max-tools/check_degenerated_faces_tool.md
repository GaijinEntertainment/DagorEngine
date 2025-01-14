# Dagor 2 Check Degenerated Faces Tool

## Installation

[Install the script](installation.md) following the provided instructions.

```{important}
This script requires 3ds Max 2014 or newer version to run.
```
## Overview

This tool is designed to identify degenerate triangles in a scene by evaluating
two key criteria: the minimum area of triangles and the minimum angle within
triangles.

Example of the tool's output:

```{eval-rst}
.. video:: _images/check_deg_faces_00.webm
   :width: 90%
```

## Accessing the Check Degenerated Faces Tool

1. Navigate to **Gaijin Tools** {bdg-dark-line}`1` **> Degenerated Triangles
   Checker...**. This will open the **Error Log** window.

2. To verify the version {bdg-dark-line}`3` of the script, go to **Gaijin
   Tools** {bdg-dark-line}`1` **> About** {bdg-dark-line}`2`. The **About**
   window will display the current version. It's important to check this
   regularly to ensure your script is up to date.

   <img src="_images/check_deg_faces_01.png" alt="Check Degenerated Faces Tool" align="center" width="60em">

```{note}
Make sure that the plugin version is at least `1.7`.
```

## Using the Check Degenerated Faces Tool

To get started, download the following test scene:
{download}`degenerated_triangles_test_2021.max <https://drive.google.com/file/d/1ZTidZlfPrMWAbM_uoYjhz9QnvvUVt19l/view?usp=drive_link>`.

```{important}
This scene requires 3ds Max 2021 or a newer version.
```

Open the script by navigating to the menu {bdg-dark-line}`1` and selecting the
script option {bdg-dark-line}`2`.

<img src="_images/check_deg_faces_02.png" alt="Check Degenerated Faces Tool" align="center" width="60em">

The script window will appear with several key options:

- **Minimum face area size** {bdg-dark-line}`3`: specifies the minimum area of
  triangles to be considered degenerate. The value is based on the current scene
  units (e.g., meters, inches).
- **Minimum face angle** {bdg-dark-line}`4`: sets the minimum angle for triangle
  evaluation. If any angle within a triangle is smaller than this value, the
  triangle will be marked as degenerate. The default values in these fields are
  typically optimal for scenes imported from a `.dag` file, as the correct size
  and scale are automatically set.
- **Show Log Window** {bdg-dark-line}`5`: enables a log that lists all objects
  and any degenerate triangles found.
- **Show Degenerated Face(s)** {bdg-dark-line}`6`: automatically adds a new
  modifier to all objects checked for degenerate triangles, highlighting the
  problematic areas for easy identification.
- **Check Degenerated Triangles!** {bdg-dark-line}`7`: initiates the check
  process.
- **Save Settings** {bdg-dark-line}`8`: saves the current script settings.
- **Default Settings** {bdg-dark-line}`9`: loads the default settings.
- **Visit to LearningWeb Site** {bdg-dark-line}`10`: links to this
  documentation.
- **Contact with Developer** {bdg-dark-line}`11`: provides contact information
  for the developer if assistance is needed.
- **Progress Bar Blue** {bdg-dark-line}`12`: displays the overall progress.
- **Progress Bar Green** {bdg-dark-line}`13`: shows the progress for the
  currently selected object.

### Running a Test

To test the loaded scene, select an object and press the start button
{bdg-dark-line}`7`. For example, if you select a simple object, the results will
appear quickly:

<img src="_images/check_deg_faces_03.png" alt="Check Degenerated Faces Tool" align="center" width="60em">

As shown in the log, the object *stalingrad_water_tower_roof_lod01_col_tra_wood*
{bdg-dark-line}`1` contains 20 degenerate triangles {bdg-dark-line}`2`. These
problematic triangles are highlighted in the **Viewport** {bdg-dark-line}`3`
using the modifier {bdg-dark-line}`4`. If you need to revert the scene to its
original state, simply remove the added modifier from the highlighted objects.

```{note}
On average, the tool processes around 1,000 triangles in about two seconds, so
larger objects or scenes may take longer.
```


