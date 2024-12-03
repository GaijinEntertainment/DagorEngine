# Animation Export

### Common Guideline

Animations are stored in the `.a2d` format. To export an animation from 3ds Max,
follow these steps:

1. **Ensure Visibility:** All layers containing bones, as well as the bones
   themselves, must be unhidden and visible.

2. **Export Settings:** Specify the starting and ending frames for the animation
   in the export settings. **Automatic frame detection does not work**.

3. **Add Note Tracks:** In the **Track View**▸**Dope Sheet**, add a Note Track
   to any of the animated bones.

   ```{seealso}
   For more information on adding and editing Note Tracks, see
   [Add Note Track](https://help.autodesk.com/view/3DSMAX/2024/ENU/?guid=GUID-658E12E7-CE32-48BF-A3CE-A289356F48A3).
   ```

   Add two keys to the Note Track:
   - At frame 0: set a key with the label `start`.
   - At the last frame: set a key with the label `end`.

   These keys correspond to the `key_start` and `key_end` parameters in the
   `animtree.blk` file of the unit. You can change the labels as needed.

   ```{seealso}
   For more information, see
   [.blk File Format](../../dagor-tools/blk/blk.md).
   ```

4. **Bone Consistency:** The number of bones and their linking order must match
   between the target model (the one the animation will be applied to) and the
   bones exported to the `.a2d` file.

### Asset Transfer Between Projects

When transferring assets between projects, **shader discrepancies** may occur,
resulting in errors. To address this:
- Use `F4` to edit the `.dag` files in FAR (configured for studio-specific
  settings) and ensure shader compatibility.
- Note that the `dynamic_simple` shader is universally available across studio
  projects and is recommended during the initial stages of asset transfer.

**Custom Properties** in 3ds Max cannot be adjusted post-transfer.
Unfortunately, importing `.dag` files may cause skin data to break. However, you
can modify the properties using `Alt`+`F4` in FAR.

```{warning}
Be cautious with the syntax: **errors in syntax will not trigger warnings, and
unsaved changes will be discarded**.
```

### Limitations

- **Bone Limit:** A maximum of 200 bones per node with skin is supported. While
  theoretically extendable to 255, current shader limitations cap the limit at
  200.
- **Triangle Limit:** A maximum of 65,534 triangles per node is allowed.

### Project-Specific Custom Object Properties

Different projects may have unique requirements for **Custom Object Properties**
to ensure proper generation of virtual assets. For example:

- **Enlisted:** Each skinned object must include the following properties:

  ```text
  animated_node:b=yes
  collidable:b=no
  massType:t="none"
  ```

  Omitting these properties will result in general errors during dynamic model
  generation.

- **CRSED:** Such requirements do not apply.

  ```{tip}
  If you have questions regarding this section, reach out to the author directly
  at {octicon}`mail;1.4em;sd-text-info` <a.vlasov@gaijin.team>.
  ```

### Animation Description in `animtree.blk`

Animations must be described in the `animtree.blk` of the corresponding block.
For War Thunder-based projects, this involves two blocks:

1. **AnimBlendNodeLeaf{} block:**

   ```text
   AnimBlendNodeLeaf{
     a2d:t="animation_file.a2d"  // Animation file name

     continuous{
       name:t="animation_name"   // Animation name
       key_start:t="start"       // Start key
       key_end:t="end"           // End key
       time:r=1                  // Playback duration
       own_timer:b=yes
       eoa_irq:b=yes
       addMoveDirH:r=-90         // Direction of unit movement
       addMoveDist:r=1.33        // Distance covered per playback time
     }
   }
   ```

2. **hub{} block:**

   ```text
   hub{
     name:t="animation_file"  // Animation name
     const:b=yes              // Indicates whether the animation loops
   }
   ```

Remember that the `animtree.blk` is also an exportable resource. To see it in
the game, you must perform a `daBuild`.

### Animation Export Description in `.blk`

The export description for animations in `.blk` format:

```text
file:t=some.a2d

opt:b=yes  // Conservative optimization
posEps:r=0.01
rotEps:r=0.2
sclEps:r=0.1
```

For `.folder.blk` files:

```text
virtual_res_blk{
  find:t="^(.*)\.a2d$"
  className:t="a2d"
  contents{
    opt:b=yes  // Conservative optimization
    posEps:r=0.01
    rotEps:r=0.2
    sclEps:r=0.1
  }
}
```


