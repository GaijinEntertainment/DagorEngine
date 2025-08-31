# Dagor Transfer Vertex Position to Vertex Color Tool

## Installation

[Install the script](installation.md) following the provided instructions.

```{admonition} 3ds Max Version Requirement
:class: warning

This script requires **3ds Max 2010 or later**.
```

## Overview

This tool facilitates the transfer and baking of vertex position changes from
one object to another using vertex colors, enabling the creation of detailed
morph effects. The tool captures the positional differences between
corresponding vertices of two objects and encodes this information into the
vertex color channels.

Below is an example of the tool's output:

```{eval-rst}

.. only:: html

   .. image:: _images/transfer_vertex_01.gif
      :alt: Transfer Vertex Position to Vertex Color Tool
      :width: 70em
      :align: center


.. only:: latex

   `Download video (GIF) <https://github.com/GaijinEntertainment/DagorEngine/blob/main/_docs/source/dagor-tools/addons/3ds-max/dagor-maxscript-toolbox/_images/transfer_vertex_01.gif>`_

```

## Accessing Transfer Vertex Position to Vertex Color Tool

1. Navigate to **Gaijin Tools** {cnum}`1` **> Dagor Vertex Pos to VColor...**.
   This will open the main window of the Dagor Vertex Position to VColor Tool.

2. To verify the version {cnum}`3` of the script, go to **Gaijin Tools**
   {cnum}`1` **> About** {cnum}`2`. The **About** window will display the
   current version. It's important to check this regularly to ensure your script
   is up to date.

   ```{eval-rst}
   .. image:: _images/transfer_vertex_02.png
      :alt: Transfer Vertex Position to Vertex Color Tool
      :width: 50em
      :align: center
   ```

   ```{admonition} Plugin Version Requirement
   :class: warning

   Requires plugin version **1.6 or higher**.
   ```

## Using Transfer Vertex Position to Vertex Color Tool

To begin, open the tool panel by navigating to **Gaijin Tools > Dagor Vertex Pos
to VColor**.

Download the following test scene:
{download}`VColorMorphDemo.max <_examples/VColorMorphDemo.zip>`.

```{admonition} 3ds Max Version Requirement
:class: warning

This scene requires **3ds Max 2024 or later**.
```

### Use Case: Bent Metal Effect

The script was originally developed to create a bent metal effect, where a new
car model gradually morphs into a damaged car model. Storing two full models is
resource-intensive, so this tool allows us to store the positional changes as
vertex color data. This approach requires that the vertex count and order in
both the original and deformed models are identical.

### Encoding Process

1. **Channel Assignment**

   - The **Red** channel stores the X-axis offset.
   - The **Green** channel stores the Y-axis offset.
   - The **Blue** channel stores the Z-axis offset.

   ```{note}
   The Z-axis is upward in most coordinate systems, but remember that in some
   systems like DirectX, the Y-axis is considered upward.
   ```

2. **Brightness Range**

   - The brightness range is split into two intervals: `0-127` and `127-255`.
   - `127` represents zero offset, meaning no positional change between the
     original and deformed model.
   - Values `0-126` represent negative offsets, while `128-255` represent
     positive offsets.

3. **Normalization**

   - To fit the positional offsets within the 128-step brightness range,
     normalization is applied.
   - The script identifies the maximum distance between corresponding vertices
     across both models. For example, if the maximum distance is 256 units, the
     script normalizes all distances using a multiplier of `256/128 = 2.0`. This
     normalization ensures that the offsets are encoded correctly within the
     available brightness range. The multiplier and normalization are
     automatically calculated after the script processes the models, as shown in
     the following example:

   ```{eval-rst}
   .. image:: _images/transfer_vertex_03.png
      :alt: Transfer Vertex Position to Vertex Color Tool
      :width: 15em
      :align: center
   ```

### Script Interface Overview

The script interface is divided into three main sections:

```{eval-rst}
.. image:: _images/transfer_vertex_04.png
   :alt: Transfer Vertex Position to Vertex Color Tool
   :width: 50em
   :align: center
```

1. **Base Object Selection (Block 1)**

   Select the base object, which remains unchanged and serves as the reference
   for positional comparison. Choose the target object where the vertex color
   (vColor) data will be written.

2. **Calculation and Output (Block 2)**

   Initiate the calculation and output of the positional differences.

3. **Inverse Transformation (Block 3)**

   Optionally, verify the accuracy of the process by performing an inverse
   transformation, converting the vertex color data back into vertex positions
   to check for consistency.

### Example Workflow

To illustrate, let's transfer the vertex position differences from an object
named `StartMorph` to another named `EndMorph`. As `EndMorph` has a point offset
along the X-axis, the Red channel will reflect this shift – positive values on
one side and negative values on the other. A gray color `(127, 127, 127)`
indicates no positional change.

```{eval-rst}
.. image:: _images/transfer_vertex_05.png
   :alt: Transfer Vertex Position to Vertex Color Tool
   :width: 50em
   :align: center
```

1. **Select the Source Object**

   Choose `StartMorph` using the **Source Object** {cnum}`1` button.

2. **Select the Target Object**

   Select `EndMorph` {cnum}`3`, the object to which vColor data will be applied.

3. **Display the Result**

   Check the **Show VColor Result** {cnum}`4` option to display the vColor in
   the Viewport. If needed, specify a different channel {cnum}`5` for the
   output, such as channel 8 (used for Ambient Occlusion in Dagor Engine).

4. **Start the Process**

   Click the **Transfer Vertex Position to VColor!** {cnum}`6` button to begin
   the transfer. The resulting vColor will illustrate the deformation, as shown
   by the color changes in the sphere, examples {cnum}`9` and {cnum}`10`.

This is an illustrative example when deformation of an object where the vColor
has changed. In a real task, we need to transfer the position the other way
around from the deformed object to the undeformed object. The selection will be
inverted.

### Verification

To verify the accuracy of the transformation:

**Inverse Transformation:**

1. Set the **Global Distance Multiplier** {cnum}`11` to the value calculated
   during the initial process.

2. Select the model with the generated vColor.

3. Click **Restore VertexPos to Start Pos!** {cnum}`12` button to revert the
   vColor back into vertex positions.

The process may introduce minor distortions due to the limited precision
available within the 128-step brightness range.

## Prototype Car Destruction in Unreal Engine

This section demonstrates a practical application of the script for simulating
car damage in Unreal Engine:

```{eval-rst}

.. only:: html

   .. video:: _images/transfer_vertex_05.webm
      :width: 90%

.. only:: latex

   `Download video (WEBM) <https://github.com/GaijinEntertainment/DagorEngine/blob/main/_docs/source/dagor-tools/addons/3ds-max/dagor-maxscript-toolbox/_images/transfer_vertex_05.webm>`_

```

### Nissan Car Damage Example

For a hands-on example, you can download the scene files for a damaged Nissan
car: {download}`nissan_sunny.zip <_examples/nissan_sunny.zip>`.

```{admonition} 3ds Max Version Requirement
:class: warning

This scene requires **3ds Max 2024 or later**.
```

#### Step-by-Step Instructions

1. **Open the Scene in 3ds Max**

   The scene contains two versions of the car: a dented version {cnum}`2` and an
   undamaged version {cnum}`1`.

   ```{eval-rst}
   .. image:: _images/transfer_vertex_06.png
      :alt: Transfer Vertex Position to Vertex Color Tool
      :width: 50em
      :align: center
   ```

2. **Select the Undamaged Car**

   Begin by selecting the undamaged car model {cnum}`1`.

3. **Specify the Dented Car as the Target**

   With the undamaged car still selected, press the selection button {cnum}`3`
   to specify the dented car as the target for the transformation.

4. **Run the Script**

   Press the execute button {cnum}`4` and wait for the script to complete its
   process.

   The car should now display a vertex color indicative of the deformation
   process, similar to the example shown below:

   ```{eval-rst}
   .. image:: _images/transfer_vertex_07.png
      :alt: Transfer Vertex Position to Vertex Color Tool
      :width: 50em
      :align: center
   ```

5. **Verify the Results**

   To verify the process, make a copy of the undamaged car {cnum}`1` and press
   the button {cnum}`2` to apply the vertex color transformation.

   ```{eval-rst}
   .. image:: _images/transfer_vertex_08.png
      :alt: Transfer Vertex Position to Vertex Color Tool
      :width: 50em
      :align: center
   ```

   The result should closely resemble the dented car model, as illustrated here:

   ```{eval-rst}
   .. image:: _images/transfer_vertex_09.png
      :alt: Transfer Vertex Position to Vertex Color Tool
      :width: 50em
      :align: center
   ```

6. **Inspect for Artifacts**

   Upon closer inspection, particularly around the steering wheel, you may
   notice minor artifacts. These artifacts are a result of compressing vertex
   positions into the vertex color space, as shown below:

   ```{eval-rst}
   .. image:: _images/transfer_vertex_10.png
      :alt: Transfer Vertex Position to Vertex Color Tool
      :width: 33em
      :align: center
   ```

   Unfortunately, these artifacts are inherent to the technology used for
   encoding vertex positions in the vertex color channels. However, they are
   generally inconspicuous during gameplay, blending naturally with the car's
   damaged appearance.

### Additional Resources

- **Open Local Documentation** {cnum}`7`: links to this documentation.
- **Contact with Developer** {cnum}`8`: provides contact information for the
  developer if assistance is needed.


