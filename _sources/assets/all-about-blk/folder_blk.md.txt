# .folder.blk

## .blk File Types

The [`.blk` files](../../dagor-tools/blk/blk.md#blk-file-format) interacting
with *assets* (all types of resources in *Dagor Engine*) can be categorized
into:

- resource `.<className>.blk` file types – the `.blk` files that process the
  asset itself (extract collision, animation, create destruction, etc.) to
  create a game resource of a certain class. A list of supported resources in
  your project can be found in `application.blk`.
- `composit.blk` – the `.blk` files that describe the handling of the final game
  resource (composites).

We will focus on `.<className>.blk` files in more detail.

```{seealso}
For more information on composite files, see [.composit.blk](./composit_blk.md).
```

After exporting from *3ds Max*, we usually obtain a `.dag` file or a `.tif` file
in the case of textures. These files are inert and cannot be used without the
engine's tools, and the engine needs instructions on how to handle them.

This is where `.<className>.blk` files come into play. They define the
processing parameters for assets, turning them into "virtual" resources. From a
single `.dag` file, we can generate an level of detail (LOD), collision,
skeleton, and more, provided the necessary objects and properties are included.

The `.<className>.blk` files must be named according to the final resource name
in the game (in the [*Asset
Viewer*](../../dagor-tools/asset-viewer/asset-viewer/asset_viewer.md)) and must
contain the asset type (`<className>`) in the filename, as it contains semantics
that are used later in config interpretation.

For example:

- `hangar_watch_tower_d.rendinst.blk` – game resource `hangar_watch_tower_d`
  (rendinst),
- `hangar_watch_tower_d.blk` – same as above, but it can be a rendinst or prefab
  (defined within the file),
- `hangar_watch_tower_d_collision.collision.blk` – game resource
  `hangar_watch_tower_d_collision` (collision),
- `airfield_workshop_a_overlay.tex.blk` – game resource
  `airfield_workshop_a_overlay` (texture).

To avoid manually creating a `.blk` file for every simple asset, such as `.dag`,
textures, skeletons, animations, dynamic models, etc., *Dagor* supports a highly
flexible system for creating virtual assets.

`.folder.blk` is a type of resource `.<className>.blk` files that process assets
in batches.

The `.folder.blk` file places in the assets directory and in most cases, it is
sufficient to simply place textures and models in the appropriate directories.
All numerous rules, such as LOD switching distances, texture conversion rules,
etc., will be created automatically.

```{note}
Resources must be uniquely named. In most cases, everything works fine if
resources are uniquely named within their type, but it is preferable to have a
fully unique name.
```

## Syntax of .folder.blk

### General Principles

The `.folder.blk` file defines how and where assets are exported, how they are
found, and the rules for creating virtual resource `.<className>.blk` files.

For assets, the `.<className>.blk` rules are always applied first if they exist
(i.e., if `my_asset.dynmodel.blk` exists, the rule for creating an asset named
`my_asset` will not be executed). Afterward, the rules from the `.folder.blk`
file next to the asset are applied, followed by the rules from `.folder.blk`
files higher in the directory hierarchy until a `stopProcessing:b=true`
directive is encountered.

To understand which rules apply to your file (e.g., `my_texture.tif` or
`my_model.dag`), start from the directory where the asset is located and look
for the nearest `.folder.blk` file up the hierarchy.

```{seealso}
For more information, see
[.blk File Format](../../dagor-tools/blk/blk.md#blk-file-format).
```

## Asset Scanning and Export Rules

### Basic parameters

- `inherit_rules:b=true\false` – apply the rules from `.folder.blk` higher up
  the hierarchy.
- `scan_assets:b=true\false` – scan assets in this directory.
- `scan_folders:b=true\false` – scan subdirectories in this directory.
- `exported:b=true\false` – whether the content is exported to the game.

  ```{important}
  It is highly recommended to use a special directory with checkboxes if you are
  going to make test assets that you want to commit and pass on to others, but
  not include for everyone.
  ```

#### Export{} Block

The export parameters can be specified either directly through names or via
[special keywords](#special-keywords).

##### Basic Parameters

- `ddsxTexPack:t=""` – defines the name or texture packs for the current
   directory and all nested directories, unless overridden by another rule,
   possible values: <pack_name>, special keyword.

- `gameResPack:t=""` – defines the name of resource packs for the current
   directory and all nested directories, unless overridden by another rule,
   possible values: <pack_name>, special keyword.

   **Examples:**

   ```
   ddsxTexPack:t="combat_suits.dxp.bin"
   gameResPack:t="combat_suits.grp"
   ```

   ```
   ddsxTexPack:t="*name_src"
   gameResPack:t="*name_src"
   ```

##### Optional Parameters

- `ddsxTexPackPrefix:t=""` or `gameResPackPrefix:t=""` – defines the prefix
  before the package name, possible values: prefix.

  **Example:**

   ```
   ddsxTexPackPrefix:t="aircrafts/"
   ```

- `package:t=""` – defines the name of additional resource packs, possible
  values: <pack_name> or `*` – means put in root pack (default value).

  **Examples:**

   ```
   package:t="outer_space"
   ```

   ```
   package:t="*"
   ```

##### Special Keywords

- `*name_src` – the name of the directory containing the **asset** will be used
  as the pack name.
- `*path_src` – the path to the directory containing the **asset** will be used
  as the pack name.
- `*name` – the name of the directory containing the `.folder.blk` file will be
  used as the pack name.
- `*path` – the path to the directory containing the `.folder.blk` file
  will be used as the pack name.
- `*parent` – the name of the parent directory will be used as the pack name.

##### Explanations

1. For example, for this file structure:

```
  dir_1
  ├── .folder.blk
  └── dir_2
      └── asset
```

- `*name_src`, `*path_src`: the `dir_2` name (or `dir_1/dir_2` path) will be
  used as the pack name.
- `*name`, `*path`: the `dir_1` name (or `dir_1` path) will be used as the
  pack name.
- `*parent`: the `dir_1` name will be used as the pack name.

2. This is how the `*parent` key works:

```cpp
    if (p[0] != '*')
    {
      if (p[0] == '/')
        return p + 1;
      if (!prefix[0])
        return p;
      buf.printf(260, "%s%s", prefix, p);
      return buf;
    }

    if (stricmp(p, "*parent") == 0)
    {
      fidx = mgr.getFolder(fidx).parentIdx;
      if (fidx != -1)
      {
        p = mgr.getFolder(fidx).exportProps.getStr(paramName, def_val);
        continue;
      }
      p = NULL;
      break;
    }
    else if (stricmp(p, "*name") == 0)
    {
      buf.printf(260, "%s%s%s", prefix, mgr.getFolder(fidx).folderName.str(), pack_suffix);
      return buf;
    }
    else if (stricmp(p, "*name_src") == 0)
    {
      buf.printf(260, "%s%s%s", prefix, mgr.getFolder(src_fidx).folderName.str(), pack_suffix);
      return buf;
    }
    else if (stricmp(p, "*path") == 0)
    {
      makeRelFolderPath(buf, mgr, fidx);
      buf2.printf(260, "%s%s%s", prefix, buf.str(), pack_suffix);
      return buf2;
    }
    else if (stricmp(p, "*path_src") == 0)
    {
      makeRelFolderPath(buf, mgr, src_fidx);
      buf2.printf(260, "%s%s%s", prefix, buf.str(), pack_suffix);
      return buf2;
    }
```

#### Virtual Resource Blocks (virtual_res{})

Virtual resource blocks (`virtual_res{}`) are used to create assets. They are
applied sequentially from top to bottom within a `.folder.blk` file, and then
from `.folder.blk` files higher in the directory hierarchy, until a
`stopProcessing:b=true` directive is encountered or all `.folder.blk` files in
the hierarchy are processed.

```
virtual_res_blk{
  find:t="^(.*)\.dds$"
  exclude:t="^(.*_nm)\.dds$"
  className:t="tex"
  name:t="low_$1"
  stopProcessing:b=false
  contents{}
}
```

##### Key Parameters

- **find**: A regular expression to locate files for which the virtual resource
  `.<className>.blk` will be created.
- **exclude**: A regular expression for excluding files that match the `find`
  pattern.
- **className**: Specifies the type of asset in your project.
- **name**: The asset's name. By default, this corresponds to the first
  capturing group in the `find` pattern (`name:t="$1"`). You can construct the
  name using values from capturing groups ($1 ... $9) and other characters.
- **stopProcessing**: Defaults to `true`, meaning no further `virtual res blk`
  rules will be applied to the matched file. Set to `false` if you want to
  create multiple assets (e.g., dynamic model, render instance, prefab,
  skeleton, character) from one texture/model.

All other parameters are specified within the `contents{}` block and are
typically specific to the asset type.

#### Specialized Blocks: content{}, tag{}

Within the `contents{}` block, you can include specialization blocks based on
*tags*. Tags are generated from all directory names in the full path to the file
(excluding the base asset path).

For example, for `develop/assets/test/warthunder/model/abc.tif`, the tags would
be `test`, `warthunder`, and `model`. Specialization block names follow the
format `tag:TTT{}`, such as:

```
contents{
  hqMip:i=1
  //...
  "tag:warthunder"{
    hqMip:i=0
  }
}
```

If applicable specialization blocks are present (i.e., the full path contains
the specified tags), their contents are merged into the main `contents{}` for
the asset. If multiple specialization blocks apply, their contents are merged in
the order they appear within the `contents{}` block.

This approach allows for different properties to be set for similar assets
located in various directories with specific names, without duplicating the
`.folder.blk`.

#### Additional Details

Typically, virtual assets do not need to specify the file name for the resource
parameter (such as the texture name or `.dag` file). These are passed by default
and equal to the name of the file from which the resource is created. However,
you can specify a different file if needed (e.g., creating a dynamic model from
`my_dynmodel.txt` with a reference to `dag_my_dynmodel.lod00.dag`).

#### Common Parameters for Most Assets

- `ddsxTexPack:t`, `gameResPack:t`, `package:t=`: See
  [above](#basic-parameters-1) for details.
- `export_PC:b`: Whether to export the asset for the specified platform
  (substitute `PC` with any 4-character platform code: `_PS4`, `_and`, `ios`,
  `PS3`, etc.).

## Using Regexp Rules

[Regular expressions](https://en.wikipedia.org/wiki/Regular_expression) are used
in `.folder.blk` for resource generation, which starts with internal
subdirectories, not external ones.

### Regular Expression Patterns

1. **LOD Files**
   ```
   find:t="^(.*)\.lod00\.dag$"
   ```
2. **Texture and Mask Files**
   ```
   find:t="^((.*_mask)|(.*_tex_m))\.tif$"
   ```

### Regular Expression Symbols Explained

- `^` : Asserts the position at the start of the line.
- `.` : Matches any single character except for line terminators.
- `*` : Matches the preceding element zero or more times.
- `\` : Escapes the following character, treating it as a literal.
- `$` : Asserts the position at the end of the line.

### Processing Control

- `stopProcessing:b=false` : Indicates whether to continue processing or
  reprocess the already found and processed items.

### Examples

#### Rendering and Prefab Creation

To generate renderings from `.lod00.dag` files and create prefabs from them,
use:

```
find:t="^(.*)\.lod00\.dag$"
```

#### Texture Processing

To find and process all files matching `"^(.*_n\.tif$"` and convert them to
`.dds` format with swizzling, then process all remaining `.tif` files:

```
virtual_res_blk{
  find:t="^(.*_n(_dmg|_expl|_inside)?)\.tif$"
  swizzleARGB:t="RAG0"
}

virtual_res_blk{
  find:t="^(.*)\.tif$"
  convert:b=yes; fmt:t="DXT1|DXT5"
}
```

These examples demonstrate how to use regular expressions to find specific file
types and perform various actions on them within the `.folder.blk` framework.


