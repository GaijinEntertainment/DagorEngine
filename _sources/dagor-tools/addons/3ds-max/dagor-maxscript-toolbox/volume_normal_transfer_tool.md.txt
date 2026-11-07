# Dagor Volume Normals Tool

## Installation

[Install the script](installation.md) following the provided instructions.

```{admonition} 3ds Max Version Requirement
:class: warning

This script requires **3ds Max 2025 or later**.
```

## Overview

Dagor Volume Normals Tool makes a volume mesh around a selected object.
It projects normals from this volume mesh to the selected object.
This smooths joints between GrowFX branches and trunks.
You can also use the tool on other meshes.

## Use Dagor Volume Normals Tool

1. Start 3ds Max.
2. Open a scene that contains the mesh to process.
3. Open the **GaijinTools** menu {cnum}`1`.
4. Select **Dagor Volume Normals Tool...** {cnum}`2`.
5. Select the target mesh {cnum}`13`.
6. Click **TRANSFER!** {cnum}`3`.

```{eval-rst}
.. image:: _images/vol_normal_gen_02.png
   :alt: Dagor Volume Normals Tool
   :width: 50em
   :align: center
```

```{important}
The tool can take much time to run.
The blue progress bar {cnum}`10` shows its total progress.
The **Projection Tris Count** value {cnum}`5` affects run time.
A mesh with 30,000 to 40,000 triangles can take more than one minute when this value is 25,000.
```

The result has smooth normal changes between branches {cnum}`1`.
The material {cnum}`2` named *branch* keeps its original normals.
The **Keep Normals on Material Name** setting {cnum}`7` excludes this material from processing.

```{eval-rst}
.. image:: _images/vol_normal_gen_03.png
   :alt: Dagor Volume Normals Tool
   :width: 50em
   :align: center
```

Other parameters:

- **Display Normals Length** sets the length of displayed normals in the viewport.
- **Projection Tris Count** sets the triangle count for the volume mesh. A higher value can improve small details and can increase run time.
- **Delete Volume after Creation** removes the volume mesh after the tool uses it. Clear this option when you must check the generated volume mesh.
- **Save Settings** saves the current settings.

## Support

- **Open Local Documentation** opens this local page.
- **Contact with Developer** opens the developer page.

Check the mesh with the STL Check modifier before you run the tool.
Select an object with missing triangles {cnum}`12`.

```{eval-rst}
.. image:: _images/vol_normal_gen_04.png
   :alt: Dagor Volume Normals Tool
   :width: 50em
   :align: center
```

STL Check {cnum}`1` reports four errors {cnum}`3`.
Show only open edges {cnum}`4` to see the edge errors.
The open edges {cnum}`2` are also visible in the viewport.

```{important}
Correct all STL Check errors before you run the tool.
Bad geometry can cause 3ds Max to stop.
The tool can correct some errors, but it cannot correct every error.
```

Select an object with valid geometry {cnum}`11` and run the tool.
The tool can generate normals for this object.
The original mesh still has visible errors.

```{eval-rst}
.. image:: _images/vol_normal_gen_05.png
   :alt: Dagor Volume Normals Tool
   :width: 50em
   :align: center
```

The result has bad geometry near {cnum}`1`.
It also has missing geometry at {cnum}`2`.
Use STL Check to check the mesh before you run the tool.

```{tip}
If you have questions, contact the author directly at
{octicon}`mail;1.4em;sd-text-info` <a.vlasov@gaijin.team>.
```
