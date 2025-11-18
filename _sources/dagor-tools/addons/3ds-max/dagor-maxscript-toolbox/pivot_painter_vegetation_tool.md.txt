# Dagor Pivot Painter Vegetation Tool

## Installation

[Install the script](installation.md) following the provided instructions.

```{admonition} 3ds Max Version Requirement
:class: warning

This script requires **3ds Max 2015 or later**.
```

## Accessing Pivot Painter Vegetation Tool

1. Navigate to **Gaijin Tools > Pivot Painter Vegetation Tool**. This will open
   the main window of the Dagor Pivot Painter Tool.

2. To verify the version {cnum}`3` of the script, go to **Gaijin Tools**
   {cnum}`1` **> About** {cnum}`2`. The **About** window will display the
   current version. It's important to check this regularly to ensure your script
   is up to date.

   ```{eval-rst}
   .. image:: _images/pivot_painter_01.png
      :alt: Pivot Painter Vegetation Tool
      :width: 60em
      :align: center
   ```

   ```{admonition} Plugin Version Requirement
   :class: warning

   Requires plugin version **1.4 or higher**.
   ```

## Using Pivot Painter Vegetation Tool

To begin, open the **Pivot Painter Vegetation Tool** by navigating to **Gaijin
Tools > Pivot Painter Vegetation Tool**.

Download the following test scene:
{download}`tree_oak_test_2021.max <_examples/tree_oak_test_2021.zip>`.

```{admonition} 3ds Max Version Requirement
:class: warning

This scene requires **3ds Max 2021 or later** and **GrowFX 2.0.1 or higher**.
```

The tool window will appear as shown below:

```{eval-rst}
.. image:: _images/pivot_painter_02.png
   :alt: Pivot Painter Vegetation Tool
   :width: 60em
   :align: center
```

### Key Features and Controls

1. **Pick GrowFX Object** {cnum}`1`: choose the GrowFX object that will serve as
   the parent for all other elements of the tree.

2. **Texture Save Path** {cnum}`2`: select the directory where the generated
   position and rotation textures will be saved.

3. **Final Object Name** {cnum}`3`: specify a custom name for the resulting
   object. By default, the tool assigns the name of the selected object.

4. **START CREATION** {cnum}`4`: initiates the tool's operation.

5. **Open Local Documentation** {cnum}`5`: links to this documentation.

6. **Contact with Developer** {cnum}`6`: provides contact information for the
   developer if assistance is needed.

7. **Group by Path Color** {cnum}`7`: If the number of hierarchy levels in
   GrowFX object exceeds 4, this checkbox should be enabled. Also in GrowFX make
   colors of Paths not more than 4x. Hierarchy will be generated in accordance
   with the specified colors.

   ```{seealso}
   For more information about how this works can be found in the GrowFX
   documentation:
   [Wire color](https://exlevel.com/growfx2-manual/Node_Path.html).
   ```

8. **Save Settings** {cnum}`8`: Saved current cettings to
   `..base/GJ_Dagor2ppainter.ini` file. If you want reset to default please
   delete this file.

### Workflow Example

1. Use button {cnum}`2` to specify the output path for the resulting `.dds`
   textures. Press **START CREATION** button {cnum}`4` to generate the textures
   `pivot_pos.dds` and `pivot_dir.dds`, along with a new **WIND** layer
   containing the completed object with the assigned name.

   ```{eval-rst}
   .. image:: _images/pivot_painter_05.png
      :alt: Pivot Painter Vegetation Tool
      :width: 60em
      :align: center
   ```

2. After generating the textures, all is done.

```{important}
- **Script Limitations.** Unlike the Houdini graph version, this script is
  limited to a maximum of 4 hierarchy levels within GrowFX. Consequently, the
  number of allowed links is restricted to 4x. In Houdini, any levels beyond
  the 4th will be merged into the last one, while this script will discard and
  not display them.
- **Object Limitations.** Both this script and Houdini impose a limit of 2048
  objects, derived from the maximum allowed texture size. Any objects exceeding
  this limit will be ignored by the script, and Houdini will also disregard
  them.
```

