# dag4blend Add-on Installation

## Table of Contents

- [dag4blend Add-on Installation](#dag4blend-add-on-installation)
  - [Table of Contents](#table-of-contents)
  - [Installation](#installation)
    - [Installation Procedure](#installation-procedure)
  - [Log and Text Editors](#log-and-text-editors)
  - [Material Setup](#material-setup)
    - [Main](#main)
    - [Textures](#textures)
    - [Optional](#optional)
    - [Tools](#tools)
    - [Proxy](#proxy)
  - [Object Properties](#object-properties)
    - [Properties](#properties)
    - [Presets](#presets)
    - [Tools](#tools-1)
  - [Export](#export)
    - [Batch Export](#batch-export)
  - [Collection Properties](#collection-properties)
    - [Override Name](#override-name)
      - [Type](#type)
    - [Geometry Nodes](#geometry-nodes)
  - [Smoothing Groups](#smoothing-groups)
    - [When to use?](#when-to-use)
  - [Import](#import)
    - [Import and Textures](#import-and-textures)
    - [File▸Import](#fileimport)
    - [Batch Import](#batch-import)
      - [Additional Features](#additional-features)
  - [Tools](#tools-2)
  - [Experimental Features](#experimental-features)
    - [Bake](#bake)
    - [Composites Editor](#composites-editor)
    - [General Limitations of the Add-on Tools in Blender](#general-limitations-of-the-add-on-tools-in-blender)

## Installation

### Installation Procedure

1. Go to `Edit`&#9656;`Preferences`&#9656;`Add-ons` in Blender.
2. Click **Install...** and navigate to the `.zip` file of the add-on.
3. After installation, check the box next to the add-on in the list.

Before starting, you need to perform some initial configuration, so do not close the `Preferences` window yet:

<p align="center">
  <img alt= "Preferences" src="./image/inst_preferences.png">
</p>

The path for **ObjProps presets:** and shader list **dagorShaders.cfg:** can now be set manually. This feature ensures that updating the add-on does not overwrite any user-made changes. It is recommended to store presets outside the add-on folder.

Until the path to an existing directory is specified, the preset menu will not be displayed. For each project you  work on, you need to specify the path to the `\assets` folder.

Set the name (it can be any name as long as it is clear to the user), specify the path, and click **+ ADD Project** to add the project to the list of available projects.

> **Note!**
>
> Many interface elements have tooltips that appear when you hover the cursor over them.

Starting from version `dag4blend2.0`, projects have additional parameters available. These parameters can be found in the **Projects** panel:

<p align="center">
  <img alt= "Projects" src="./image/inst_projects.png">
</p>

- **path:** – you can edit the path after adding a project - for example, if you move resources to a different drive. You no longer need to delete and re-add the project with the new path.

- **Shading mode:** – shaders behave slightly different between projects. The add-on is designed to adapt to two main shader groups: *daNetGame-like* and *WarThunder-like* shaders.

- **Palettes** – there are default global and local palettes.

- **Experimental Features** – new tools that are functional but have some limitations. Currently, these include a *composites editor* and *texture baking tools* tools:

<p align="center">
  <img alt= "Experimental Features" src="./image/inst_exp_features.png">
</p>

The active project is set through the menu in the scene properties. You can also change palettes here without needing to go into `User Preferences` each time:

<p align="center">
  <img alt= "Scene" src="./image/inst_scene.png">
</p>

> **Note!**
>
> The add-on applies settings when these parameters are changed, but only if there is an appropriate target. Therefore, to fix the **Shading mode**, you need to re-select the project after creating at least one dagormat in the scene. For **Palettes**, re-select the project after creating a painted dagormat.
>
> By default, the global and local palettes will be simple red and green fills, and the shaders will be in *WarThunder-like* mode. To correct this, re-select the project from the dropdown after the first asset import or creation.

## Log and Text Editors

Text objects play an important role in this toolset, so it is recommended to add a text field to your layout or add a new window with just a text field if you are working with multiple monitors.

<p align="center">
  <img alt= "Text field" src="./image/log_text_field.png">
</p>

The import/export functions (and future ones) write execution information to a text  **log** field.

> **Note!**
>
> The **log** does not clear itself, so remember to clear it periodically after ensuring that there are no unchecked errors and warnings. To clear it, you can simply delete the text object. The next time you need it, the toolset will create a new one. This approach can be faster than selecting all the lines and deleting them with `Backspace`/`Delete`.

To edit object properties/dagormats/proxymats as text, the toolset creates a special text object. The name of the object is provided in the tooltip.

## Material Setup

For setup material for Dagor Engine, there is separate **dagormat** tab. It contains several subtabs that can be collapsed when not needed:

<p align="center">
  <img alt= "Dagormat" src="./image/mat_dagormat.png">
</p>

### Main

Here you can choose whether the material is two-sided, and if so, how it behaves:

<p align="center">
  <img alt= "Dagormat&#9656;Main" src="./image/mat_dagormat_main.png">
</p>

- **single_sided** – enables backface culling for triangles with this material.

- **two_sided** – each triangle is rendered from both sides. The shader is slightly heavier, but the geometry is lighter. Use this primarily for heavy geometry, such as tree crowns.

- **real_two_sided** – in the engine, a single-sided material is uses, but each triangle with this material will be duplicated and flipped to be visible from the other side. This shader is lighter, but it doubles the mesh weight. It is better used on simpler meshes.

Below is a list of **legacy** properties: *ambient*, *specular*, *diffuse*, *emissive* and *power*. Hover over them to see tooltips describing each property.

This tab also allows you to select a shader from the available options or manually enter a value. It is useful if new shaders have been added to the game but the toolset has not been updated yet.

<p align="center">
  <img alt= "Dagormat&#9656;Shader" src="./image/mat_dagormat_shader.png">
</p>

The list of shaders and their possible parameters are taken from `dagorShaders.cfg`, which by default is located at:
`C:\Users\<username>\AppData\Roaming\Blender Foundation\Blender\<version number>\scripts\addons\dag4blend`.

### Textures

This section lists the textures in use. Indices (matching those stored in `.dag` files) are available in tooltips. You can copy and paste paths from Asset Viewer or Explorer; quotes will be removed automatically.

<p align="center">
  <img alt= "Dagormat&#9656;Textures" src="./image/mat_dagormat_textures.png">
</p>

### Optional

Visual editing of shader parameters. You can manually enter values or select from a list, similar to how shader selection works.

<p align="center">
  <img alt= "Dagormat&#9656;Optional" src="./image/mat_dagormat_optional.png">
</p>

### Tools

Here you can edit the active material in text format.

<p align="center">
  <img alt= "Dagormat&#9656;Tools" src="./image/mat_dagormat_tools_1.png">
</p>

<p align="center">
  <img alt= "Dagormat&#9656;Tools" src="./image/mat_dagormat_tools_2.png">
</p>

If there is no text editing field, one will be added to the left of the viewport, otherwise, the text will be opened in the first available text field.

Search is available in two modes: only for the active material or for all materials in the scene. Switch modes by clicking the text indicating the current mode.

A correctly specified project is required for search to function properly - when working on a daNetGame asset, specify *daNetGame*.

- **Find missing...** – finds all textures and all proxymats (if they exist in the project). The texture search only affects viewport display and does not alter the material path.

- **Rebuild materials** – rebuilds the materials for the viewport.

- **Update texture paths** – finds non-existent paths and replaces them with current ones. Should be used after texture search.

- **Clear texture paths** – removes directory information, leaving only texture names.

### Proxy

The add-on now supports proxymats.

<p align="center">
  <img alt= "Dagormat&#9656;Proxy" src="./image/mat_dagormat_proxy.png">
</p>

Proxymat parameter information is taken from the `.blk` file, so all settings for them are hidden. Instead, a new tab allows you to specify the path to the proxymat folder. It can still be edited as text or temporarily unchecked as **is proxymat**.

Changes can be reverted by reloading information from the file or saved to the proxymat. Any regular material can also be converted to a proxymat by toggling the checkbox and saving it to the desired folder.

> **Note!**
>
> Proxymats in Blender use the file name, not a user-defined one. Upon import, shaders are automatically renamed if the name differs.

## Object Properties

Located in the N-panel under the `Dagor` tab, it shows parameters of the active object and is hidden if there is no active object.

<p align="center">
  <img alt= "Object Properties" src="./image/obj_prop.png">
</p>

### Properties

Visual editing allows you to add or remove parameters individually and change their values.

The UI format is determined by the `value` string. If the wrong UI option appears (e.g., `prop:r=0` gives an integer toggle, while the variable should be a float slider), enter the parameter name in the **name** field and the correct format value (e.g., `0.0` instead of `0` for a float slider). Specify the type in the name using `:`.

### Presets

Once properties are configured, you can save them as a preset for quick application to other objects.

To save the active object's properties, enter a name for the preset and click **Save preset as:**. To apply an existing preset, select it from the dropdown and click **Apply preset:**.

Since presets are simple text files, you might want to edit them in a text editor. The **open presets folder** button opens the folder containing all presets in `.txt` format, where you can add, delete, or edit them. Changes are applied immediately.

### Tools

Editing in text format within Blender, similar to material editing, requires opening the to the file specified in the tooltip by clicking **Open as text**.

**Apply...** – applies changes from the text.

**Write example** – writes an example to the text (without applying it, since you will likely need to modify the example for your specific situation).

**Transfer Attr** – copies the properties from the active object to all selected objects, saving time on manual copying.

Starting from `version 2.1.0`, the text object **props_temp** opens automatically if the internal text editor is open.

> **Note!**
>
> Incorrect values in Object Properties are recorded in a new **broken properties** field; separated by `;`, allowing manual recovery. Similarly, incorrect values when using **Apply from text** are recorded.

## Export

Like any other import-export add-on, the `.dag` exporter is available through `File`&#9656;`Export`. However, to save time during multiple re-exports in the workflow, the exporter is also available in the N-panel. The functionality is identical, so choose the option that is most convenient for you.

### Batch Export

General parameters:

- **vNormals** – saves custom normals if they have been added to the object. Without custom normals, even with the checkbox active, the export will include smoothing groups.

- **applyMods** – exports geometry with visible modifiers applied.

- **Optimize Materials** – keeps only the materials used on at least one polygon for each object.

- **Path** – the path where the file will be saved.

- **Cleanup Names** – ignore indices (.000+) during export.

> **Note!**
>
> Use **Cleanup...** only for exporting composite parts for 3D Max! It can cause issues. In a `.dag` file for the game engine, there should not be multiple objects with the same name.

- **Name** field  – context-sensitive, appearing only when exporting to a single `.dag` file. This field is hidden in other export modes.

- **Collection** field – also context-sensitive. You do not need to select a collection from the dropdown; you can simply drag the desired collection from the Outliner.

- **Limit by** – allows you to choose the export mode.

| Mode (Limit by) | Export object | Example input | Example output |
| :--- | :----: | :----: | :----: |
| Visible | export all scene contents to `Path\<Name>.dag` | <p align="center"><img alt= "Limit by&#9656;Visible" src="./image/batch_exp_visible.png"></p> | `C:\tmp\asset.dag` – contains all scene objects with custom normals |
| Sel.Joined | export selected objects to `Path\<Name>.dag` | <p align="center"><img alt= "Limit by&#9656;Sel.Joined" src="./image/batch_exp_sel_joined.png"></p> | `C:\tmp\asset.dag` – contains only selected scene objects |
| Sel.Separated | export selected objects to separate `.dag` files, where the names of selected objects are used as file names | <p align="center"><img alt= "Limit by&#9656;Sel.Separated" src="./image/batch_exp_sel_separated.png"></p> | `C:\tmp\cube.lod00.dag`, `C:\tmp\cube.lod01.dag` |
| Col.Separated | export collections from the hierarchy starting from the selected one, if there are no subcollections inside. The **exportOrphans** checkbox allows exporting objects that lie next to subcollections | <p align="center"><img alt= "Limit by&#9656;Col.Separated" src="./image/batch_exp_col_separated.png"></p> | `C:\tmp\cube.lod00.dag` – contains visible geometry and occluder inside, `C:\tmp\cube.lod01.dag` – contains visible geometry and collider inside and `C:\tmp\cube.lod02.dag`. `C:\tmp\cube_temp.dag` – will not be exported, because the **exportOrphans** checkbox is not active. If you don't specify the collection at all, then in this case the result will be identical, because the check will start with **SceneCollection**, and it contains only the **cube** collection. |
| Col.Joined | export all contents of the selected collection to a file with the name of this collection. Created mainly for Asset Viewer of assets created from several `.dag` files | <p align="center"><img alt= "Limit by&#9656;Col.Joined" src="./image/batch_exp_col_joined.png"></p> | `C:\tmp\soviet_town_building_5_floors_a_joined.lod00.dag`, containing all null lods related to this building. The collections have been linked (transfer with `Ctrl` in Outliner), i.e. they are not copies, but the same collections. The rest of the temporary lods can be prepared in the same way. It's much faster than selecting objects and typing the name manually. **Note:** since they are the same collections and not copies, do not delete them with `DEL`, but remove them with `RMB`&#9656;`Unlink`, so that you don't overwrite them. |

## Collection Properties

Located in the N-panel, similar to Object Properties. It shows properties of the active collection. Unlike Object Properties, these properties exist only within Blender and are used for internal purposes.

<p align="center">
  <img alt= "Collection Properties" src="./image/coll_prop.png">
</p>

### Override Name

Allows you to override the name with which the collection will be exported to **Col.Separated** mode. By default, overriding is disabled.

When importing `.dag` files with names longer than 63 characters (the Blender limit), the name will be recorded in this field to preserve the "overflow"  characters. You can also manually rename it – check the checkbox and enter the new name:

<p align="center">
  <img alt= "Override name: example 1" src="./image/coll_prop_override_ex_1.png">
</p>

In the given example, the contents of the collection will be exported to `C:\tmp\cube.lod00.dag` instead of `C:\tmp\Collection.dag`. However, this is not the most useful application.

A much more practical use is when you need to save multiple `.dag` files in different subfolders. Here, you can specify a subpath that will be appended to the main export path. If there is no need to rename the collection, simply end the subpath with `*`, which will be replaced by the collection name during export:

<p align="center">
  <img alt= "Override name: example 2" src="./image/coll_prop_override_ex_2.png">
</p>

In this example, the export path will be `C:\tmp\subfolder\yet_another_subfolder\cube.lod00.dag`, which is convenient for working with complex assets like buildings. For example, windows in `composit_parts\windows\*`, doors in `composit_parts\doors\*`, and so on.

Additionally, you can completely override the path – start it with the drive letter:

<p align="center">
  <img alt= "Override name: example 3" src="./image/coll_prop_override_ex_3.png">
</p>

In this case, the **Path** field will be ignored for this collection, and the file will be exported to `D:\daNetGame\develop\assets\rendinst_1lod\`.

#### Type

Used by the composite editor. When importing, it records the node type, and during export, it ensures a unique type (to avoid double interpretation in the presence of asset namesakes, typically found among composites and rendInsts). For manually created item intended for composite export, set this manually.

<p align="center">
  <img alt= "Type" src="./image/coll_prop_override_type.png">
</p>

Since it is used for composites, Dynmodel is in the list but not actually applied – it can't be arranged through composites. Therefore, the composite importer assumes all assets with `.lod**.dag` in their name are RendInsts, not Dynmodels.

### Geometry Nodes

> **Note!**
>
> Currently, geometry nodes cannot be combined with custom normals, as applying the modifier erases them. If you're placing instances using geometry nodes, don't forget to add the **Realize instances** node; otherwise, the geometry will be lost during export. Technically, before this node, they are similar to “empty” without their own geometry.

## Smoothing Groups

A bug was discovered in Blender's function for calculating smoothing groups for formats that use them. To solve this, a smoothing group editor has been added. It's available in Edit Mode when Face selection mode is active.

Upon import, objects retain smoothing groups from the original file. New objects, however, do not have smoothing groups, so you must create them with the **Init** button:

<p align="center">
  <img alt= "Smoothing groups: Init" src="./image/sm_groups_1.png">
</p>

When smoothing exist, the interface works similarly to 3D Max, but without real-time display. Partial preview is available via **Convert to Sharp Edges**, which shows only hard edges from the smoothing groups.

<p align="center">
  <img alt= "Smoothing groups" src="./image/sm_groups_2.png">
</p>

Starting with `version 2.1.0`, the **Live Update** function recalculates hard edges during smoothing group editing. This can slow down performance on heavy geometry, so it is off by default.

From the same version, you can also select polygons by smoothing group. Click the corresponding group button in the **Select by SG** subpanel.

> **Note!**
>
> Selecting by smoothing group adds to the existing selection, rather than replacing it. So, if you need to select only group 3, for example, clear the selection first.

### When to use?

If a model has areas where hard edges do not fully divide the surface into closed regions, the smoothing group calculation will be incorrect and needs manual adjustment. You may also need to adjust smoothing groups after modifying existing `.dag` file post-import if the groups were saved as an attribute.

<p align="center">
  <img alt= "Smoothing groups" src="./image/sm_groups_3.png">
</p>

## Import

Similar to export, there are two methods for import – the standard method via `File`&#9656;`Import` and the batch import via N-panel.

### Import and Textures

A `.dag` file contains not only the texture name but also its path. However, these paths are often become invalid when files are transferred. If the texture path os invalid or only the texture name is provided, it will be replaced with a UV checker texture. If a texture is used in multiple materials, updating the path in one will update it across all materials.

Texture slots are still used as in `rendinst_simple`, regardless of the chosen  shader. Nodes do not overlap but are arranged meaningfully. Images from all slots are now added to the Shader Editor, so with Node Wrangler enabled, they can be viewed by pressing `Shift`+`Ctrl`+`LMB`.

### File&#9656;Import

Parameters:

- **Optimize material slots** – removes unused slots and merges slots with the same material.

- **Import LODs** – imports all lods, not just the selected `.dag` file.

- **Import dps** – import all damage_parts (_dp).

- **Import dmgs** – import all damage versions.

- **Replace existing** – if a collection with the asset name exists in the scene, it will replace its contents with those from the `.dag` file instead of creating a duplicate with a `.001+` suffix. Useful for quickly reverting multiple changes.

<p align="center">
  <img alt= "File&#9656;Import" src="./image/file_import.png">
</p>

For bulk imports, it's smart to use batch import.

### Batch Import

This panel allows importing .dag files from a specified folder.

<p align="center">
  <img alt= "Batch Import" src="./image/batch_imp.png">
</p>

Options:

- **Search in subfolders** – checks all subdirectories for files. Use cautiously, as it can hang Blender if too many matches are found.

- **Optimize material slots** – same as in regular import.

- **Preserve Smoothing Groups** – saves smoothing groups as an attribute to ensure they don't change during export. Not recommended if you plan to modify the geometry after import.

- **Preserve paths** – saves the full path to the `.dag` file (including the file name) in Collection Property. Useful when importing file from multiple folders and needing to export them back to original locations.

- **Masks** – specifies masks for import, using `*` instead of `.*` for multiple characters, and `.` instead of `\.`. Separate masks with `;`. Spaces are ignored.

- **Excludes** – same syntax as masks, but for excluding files from import.

Example:

Masks: `“asset_a.lod0[0,2]*; asset_b_dp*”; Excludes=“*_dmg*”` imports `asset_a.lod00.dag`, `asset_a.lod02.dag`, and all lods of all `asset_b_dp`, except their `_dmg` versions.

- **Path** – the directory to search.

> **Note!**
>
> Blender will be unresponsive until the import is complete. To monitor the import process, open the console (`Window`&#9656;`Toggle system console`).

#### Additional Features

Often, you may need to import a single asset that is already open in Asset Viewer or Explorer. Instead of searching for it through `File`&#9656;`Import`, you can copy the path from Asset Viewer or Explorer (`RMB`&#9656;`Copy as path`) and paste it into **Path** field of the batch import panel. This action will automatically transfer the file name to **Masks**, and clicking **IMPORT** will load only the desired asset.

If you only have the asset's name, you can enter it in **Masks** and then use the **Apply as search path** button to set the entire project directory with subfolders as the search path.

If the path is unknown, the import will be slower by a few seconds (due to search file), but this is often faster than searching the full path manually.

<p align="center">
  <img alt= "Batch Import: Additional Features" src="./image/batch_imp_add_feat.png">
</p>

## Tools

The panel is continuously being improved. Tools are grouped into blocks that can be collapsed when not in use to avoid cluttering the interface.

<p align="center">
  <img alt= "Tools" src="./image/tools.png">
</p>

- **Optimize material slots** – merges identical material slots and removes unused ones.

- **Merge Duplicates** – merges identical materials with different indices (e.g., `Material` and `Material.001`). These duplicate materials often arise when copying objects using `ctrl+c`/`ctrl+v`, as this operation saves the selection to an external `.blend` file and then appends it to the active scene along with all its materials.

- **Find missing textures** – searches the textures without valid paths in the selected project's folder.

- **Find missing proxymats** – similar functionality for proxymats.

- **Save textures** – collects all textures with valid paths from selected objects and saves them in the `/textures/` subfolder of the export folder.

- **Preserve Sharp Edges** – marks hard edges based on Autosmooth Angle, and sets the angle to 180&deg; to ensure consistency between the model in the scene and in the `.dag` file. This function is not mandatory before exporting, as it will automatically apply to the exported geometry (keeping the original scene geometry unchanged).

- **Apply Modifiers** –  applies modifiers to the geometry. Not mandatory before export as the exporter can preserve modifiers in the original scene.

- **Clear normals** – removes custom normals from selected objects.

- **sort collections** – recursively groups scene collections by the longest matching name prefix. You can see the example on the screenshot for the [“Col.Joined” export](#batch-export).

- **pack orphans** – searches for objects not at the hierarchy's bottom and creates individual collections for them.

- **Setup Destruction** – assigns a list of typical object properties for destruction to all selected objects:

```
  animated_node:b=yes
  physObj:b=yes
  collidable:b=no
  massType:t=none
```

Creates bounding boxes (BBoxes) objects with `gi_black` material, sets them as children, and assigns collider parameters:

```
  materialName:t=[value from the material field]
  density:r=[value from the density field]
  massType:t=box
  collType:t=box
```

The BBoxes may still need to be adjusted and scaled to better fit the silhouette. In some cases, they may also need to be duplicated. However, this process still saves time in the overall setup.

## Experimental Features

### Bake

Allows baking textures from complex shaders to rendinst_simple. Commonly used for final lods or porting assets to mobile projects. Detailed in a separate article.

<p align="center">
  <img alt= "Bake" src="./image/bake.png">
</p>

### Composites Editor

The topic is extensive, so it will be covered later in a separate article.

<p align="center">
  <img alt= "Composites Editor" src="./image/comp_editor.png">
</p>

### General Limitations of the Add-on Tools in Blender

- Limitation on the number of characters in the name – names longer than 63 characters will be truncated, and when importing objects with the same name, an index `.001` is appended, reducing the maximum name length by 4 more characters. Starting from version `1.2.6`, overly long names are saved in Collection Property.

- Animations are not supported yet.