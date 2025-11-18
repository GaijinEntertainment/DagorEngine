# Dagor Check Degenerated Faces Tool

## Installation

[Install the script](installation.md) following the provided instructions.

```{admonition} 3ds Max Version Requirement
:class: warning

This script requires **3ds Max 2014 or later**.
```

## Overview

This tool is designed to identify degenerate triangles in a scene by evaluating
two key criteria: the minimum area of triangles and the minimum angle within
triangles.

Example of the tool's output:

```{eval-rst}

.. only:: html

   .. video:: _images/check_deg_faces_00.webm
      :width: 90%

.. only:: latex

   `Download video (WEBM) <https://github.com/GaijinEntertainment/DagorEngine/blob/main/_docs/source/dagor-tools/addons/3ds-max/dagor-maxscript-toolbox/_images/check_deg_faces_00.webm>`_

```

## Accessing Check Degenerated Faces Tool

1. Navigate to **Gaijin Tools** {cnum}`1` **> Degenerated Triangles
   Checker...**. This will open the **Error Log** window.

2. To verify the version {cnum}`3` of the script, go to **Gaijin Tools**
   {cnum}`1` **> About** {cnum}`2`. The **About** window will display the
   current version. It's important to check this regularly to ensure your script
   is up to date.

   ```{eval-rst}
   .. image:: _images/check_deg_faces_01.png
      :alt: Check Degenerated Faces Tool
      :width: 60em
      :align: center
   ```

   ```{admonition} Plugin Version Requirement
   :class: warning

   Requires plugin version **1.7 or higher**.
   ```

## Using Check Degenerated Faces Tool

To get started, download the following test scene:
{download}`degenerated_triangles_test_2021.max <_examples/degenerated_triangles_test_2021.zip>`.

```{admonition} 3ds Max Version Requirement
:class: warning

This scene requires **3ds Max 2021 or later**.
```

Open the script by navigating to the menu {cnum}`1` and selecting the script
option {cnum}`2`.

```{eval-rst}
.. image:: _images/check_deg_faces_02.png
   :alt: Check Degenerated Faces Tool
   :width: 60em
   :align: center
```

The script window will appear with several key options:

- **Minimum face area size** {cnum}`3`: specifies the minimum area of triangles
  to be considered degenerate. The value is based on the current scene units
  (e.g., meters, inches).
- **Minimum face angle** {cnum}`4`: sets the minimum angle for triangle
  evaluation. If any angle within a triangle is smaller than this value, the
  triangle will be marked as degenerate. The default values in these fields are
  typically optimal for scenes imported from a `.dag` file, as the correct size
  and scale are automatically set.
- **Show Log Window** {cnum}`5`: enables a log that lists all objects and any
  degenerate triangles found.
- **Show Degenerated Face(s)** {cnum}`6`: automatically adds a new modifier to
  all objects checked for degenerate triangles, highlighting the problematic
  areas for easy identification.
- **Check Degenerated Triangles!** {cnum}`7`: initiates the check process.
- **Save Settings** {cnum}`8`: saves the current script settings.
- **Default Settings** {cnum}`9`: loads the default settings.
- **Open Local Documentation** {cnum}`10`: links to this documentation.
- **Contact with Developer** {cnum}`11`: provides contact information for the
  developer if assistance is needed.
- **Progress Bar Blue** {cnum}`12`: displays the overall progress.
- **Progress Bar Green** {cnum}`13`: shows the progress for the currently
  selected object.

### Running Test

To test the loaded scene, select an object and press the start button {cnum}`7`.
For example, if you select a simple object, the results will appear quickly:

```{eval-rst}
.. image:: _images/check_deg_faces_03.png
   :alt: Check Degenerated Faces Tool
   :width: 60em
   :align: center
```

As shown in the log, the object `stalingrad_water_tower_roof_lod01_col_tra_wood`
{cnum}`1` contains 20 degenerate triangles {cnum}`2`. These problematic
triangles are highlighted in the Viewport {cnum}`3` using the modifier
{cnum}`4`. If you need to revert the scene to its original state, simply remove
the added modifier from the highlighted objects.

```{note}
On average, the tool processes around 1,000 triangles in about two seconds, so
larger objects or scenes may take longer.
```

