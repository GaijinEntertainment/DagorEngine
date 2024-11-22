# Introduction to Assets

## Overview

Assets refer to all types of resources used in the *Dagor Engine Tools*.

You can find a list of supported asset types in your project in the
[`application.blk`](../all-about-blk/application_blk.md) file. This file is a
key resource for configuring how the tools interact with your project.

```text
assets{
  types{}
}
```

Within `application.blk` file, the base path to the asset directory is specified
as `base:t=`.

Assets are classified into three main categories:

1. **Editor-only assets**: These are resources used exclusively by the editor,
   such as for exporting levels. Examples include materials, prefabs, spline
   classes, and others.

2. **Game-exported assets**: These resources are exported into the game
   (indicated by the `export{}` block in `application.blk`). They are stored in
   `.grp` files within the game. Typically, the level editor uses the exported
   game resources for performance reasons.

3. **Textures**: Although these are usually exported into the game, they are
   stored in `.dxp.bin` files. The editor uses a texture cache rather than the
   exported textures, allowing for real-time texture modifications.

## Asset Formats and Management

Assets for both the game and tools are stored in `.blk` files, which come in two
formats:

- **Modern format**: `<asset_name>.<className>.blk` (e.g.,
  `my_house.rendinst.blk`).
- **Legacy format**: `<asset_name>.blk`, where the asset type must be specified
  within the `.blk` file.

Each asset's `.blk` file contains all the parameters required by the exporter
for that specific resource. Each type of resource has its own build tool plugin
and dedicated code in the game.

In some cases, the description within the `.blk` file is minimal. For example, a
texture asset might only require the texture name (if it's in `.dds` format). In
other cases, like effects, the entire asset is defined within the `.blk` file,
and the [*Asset
Viewer*](../../dagor-tools/asset-viewer/asset-viewer/asset_viewer.md) acts as
its editor.

Most assets do not have dedicated editing tools. The primary method for creating
these assets is through manual editing and hot-reloading using the tools.

However, to avoid manually creating `.blk` files for every simple asset type
(e.g., textures, skeletons, dynamic models), *Dagor* supports a highly flexible
system for creating virtual assets - through a
[`.folder.blk`](../all-about-blk/folder_blk.md) file placed within the asset
directory.

In practice, simply placing textures and models into the appropriate directories
is sufficient in 99.99% of cases. Numerous rules, such as LOD (Level of Detail)
switching distances and texture conversion rules, will be generated
automatically.

```{note}
All resource names must be **unique**. While the system generally functions if
resources are unique within their type, it is advisable to ensure globally
unique names.

```

## Asset Types and Their Rules

### Texture Assets

A brief overview of texture asset parameters (all are optional, and for standard
diffuse `.dds` textures, nothing may need to be specified. For `.tga` files, two
parameters are typically required: `convert:b=yes`, `fmt:="DXT1|DXT5"`):

```text
contents{
  convert:b=yes; fmt:t="DXT1|DXT5"
  mipmapType:t=mipmapGenerate
  mipFilter:t="filterKaiser"; mipFilterAlpha:r=40; mipFilterStretch:r=2
  addrU:t="wrap"; addrV:t="wrap";
  swizzleARGB:t="RAG0"
  hqMip:i=3; mqMip:i=3; lqMip:i=4
  PC{
    hqMip:i=2; mqMip:i=2; lqMip:i=3;
  }
  gamma:r=1
  colorPadding:b=no
  // alphaCoverageThresMip9:i=200
  // alphaCoverage:b=no
  // rtMipGen:b=no
  // aniFunc:t="abs";
  // anisotropy:i=8
  addrU:t="wrap"; addrV:t="wrap";
}
```

- `convert`: Converts the texture (mandatory for all `.tga` or `.psd` textures).
- `fmt:t`: Specifies the format to convert to (options include `DXT1`, `DXT5`,
  `ARGB`, `L8`, `A8L8`, `L16`).
- `mipmapType:t`: Method for generating mipmaps (`mipmapGenerate`,
  `mipmapUseExisting`, `mipmapNone`).
- `mipFilter`: Specifies the filter used to generate mipmaps (`filterKaiser`,
  `filterBox`, `filterCubic`).
- `hqMip`, `mqMip`, `lqMip`: Specifies which mip level to use at high, medium,
  and low quality settings.
- `swizzleARGB`: Specifies how to swizzle the texture channels. Typically not
  needed.
- `gamma`: Gamma correction value (`1` for normal maps and masks, default is
  `2.2`).
- `addrU`, `addrV`: Texture addressing mode (`wrap`, `clamp`, `border`).

### Dynamic Models and Rendering Instances

```text
lod{
  range:r=70; fname:t="$1.lod00.dag";
}
lod{
  range:r=200; fname:t="$1.lod01.dag";
}
lod{
  range:r=550; fname:t="$1.lod02.dag";
}
lod{
  range:r=4000; fname:t="$1.lod03.dag";
}
ref_skeleton:t="$1_skeleton"
texref_prefix:t="low_"
all_animated:b=yes
```

- `lod`: LOD parameters.
  - `range:r`: LOD distance.
  - `fname:t`: Optional filename, defaults to `$1.lod<lod_number>.dag`.
- `texref_prefix:t`: Prefix added to all texture references in the model.
- `ref_skeleton:t`: Reference skeleton for dynamic models, necessary for correct
  preview in [*Asset
  Viewer*](../../dagor-tools/asset-viewer/asset-viewer/asset_viewer.md).
- `all_animated:b`: Indicates that all objects in the model should be
  considered animated, i.e., having their own matrices.

### Composite Dynamic Models

Composite dynamic models are structures composed of multiple dynamic models that
share a common skeleton.

Below are the rules for creating a
[`.skeleton.blk`](../all-about-blk/skeleton_blk.md) file for a composite model,
using a tank with several turret and gun options as an example:

```text
name:t="tank_body.lod00.dag"
attachSubSkel{
  attach_to:t="bone_turret"
  skel_file:t="turret_a.lod00.dag"
  skel_node:t="bone_turret"
  attachSubSkel{
    attach_to:t="bone_gun_a"
    skel_file:t="gun_a.lod00.dag"
    skel_node:t="bone_gun_a"
  }
  attachSubSkel{
    attach_to:t="bone_gun_b"
    skel_file:t="gun_b.lod00.dag"
    skel_node:t="bone_gun_b"
    add_prefix:t="G1:"
  }
  attachSubSkel{
    attach_to:t="bone_gun_c"
    skel_file:t="gun_b.lod00.dag"
    skel_node:t="bone_gun_b"
    add_prefix:t="G2:"
  }
}
attachSubSkel{
  attach_to:t="bone_turret"
  skel_file:t="turret_b.lod00.dag"
  skel_node:t="bone_turret"
  add_prefix:t="T1:"
}
```

- `name:t`: Parent model name.
- `attachSubSkel{}`: Block for adding a dynamic model.
  - `attach_to:t`: Node in the parent skeleton to which the additional dynamic
    model is linked.
  - `skel_file:t`: Child model file name.
  - `skel_node:t`: Node in the child model's skeleton to link with the parent.
  - `add_prefix:t`: Prefix for all child model nodes.

In the `.skeleton.blk` file above, the skeleton generated from the `tank_body`
model is extended with the turrets `turret_a` and `turret_b`, which are attached
to the `bone_turret`. On `turret_a`, two unique guns, `gun_a` and `gun_b`, are
attached to `bone_gun_a`, `bone_gun_b`, and `bone_gun_c`. However, `gun_b` is
attached twice to different bones.

Since we are creating a single shared skeleton, it is critical to avoid
duplicate node names. To address this, a unique prefix is added to the nodes of
each duplicate. The two copies of `bone_gun_b`, forming different branches in
the hierarchy, received prefixes `G1` and `G2`. Similarly, the nodes in
`turret_b` were prefixed with `T1`.

```{important}
- When specifying the attachment node, do not include automatically added
prefixes.
- If a child model is attached to a node with the same name as one of its
bones, the previous node is removed from the hierarchy. There won't be a
duplicate – don't worry.
```

While theoretically, you can create a very deep hierarchy of dependencies,
simpler structures are easier to manage. Always evaluate the necessity of adding
new levels before doing that.

A multi-level hierarchy might look like this:

```text
name:t="papa.lod00.dag"
attachSubSkel{
  attach_to:t="bone_papa"
  skel_file:t="child.lod00.dag"
  skel_node:t="bone_child"
  add_prefix:t="layer01:"
  attachSubSkel{
    attach_to:t="bone_child"
    skel_file:t="child.lod00.dag"
    skel_node:t="bone_child"
    add_prefix:t="layer02:"
    attachSubSkel{
      attach_to:t="bone_child"
      skel_file:t="child.lod00.dag"
      skel_node:t="bone_child"
      add_prefix:t="layer03:"
      attachSubSkel{
        attach_to:t="bone_child"
        skel_file:t="child.lod00.dag"
        skel_node:t="bone_child"
        add_prefix:t="layer04:"
      }
    }
  }
}
```

This hierarchy demonstrates a parent model (`papa.lod00.dag`) with multiple
layers of child models attached, each with its own prefix to ensure uniqueness
and proper hierarchy management.


