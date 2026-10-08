# Dagor Custom Groups Export Tool

## Installation

The Dagor Custom Groups Export Tool is a specialized script designed to prepare
scenes for export, automating the process of saving multiple groups of objects
with predefined properties into the DAG format. This script is not included in
the Dagor Tool set, as it is tailored for a specific mode of object export.

To install the script, simply drag and drop the `dagor_export_custom_groups.ms`
file {cnum}`0` located at `.../dagor_toolbox/base` into the active Viewport of
3ds Max {cnum}`1` (green marks on the picture below). This action will open the
Dagor Custom Batch Exporter script window.

```{admonition} 3ds Max Version Requirement
:class: warning

- This script requires **3ds Max 2013 or later**.
- Ensure that Dagor Plugins are installed before using this script.
```

```{eval-rst}
.. image:: _images/custom_group_exp_01.png
   :alt: Custom Groups Export Tool
   :width: 70em
   :align: center
```

## Using Custom Groups Export Tool

The script provides several configurable settings:

- **Move to [0,0,0]** {cnum}`2`: moves selected objects to the coordinate center
  before export. After export, objects are returned to their original positions.
- **Reset XForm** {cnum}`3`: automatically resets the XForm on selected objects
  before export.
- **Convert to editable mesh** {cnum}`4`: converts selected objects to
  **Editable Mesh** before export.
- **Merge All Nodes** {cnum}`5`: merges all selected objects within groups into
  a single object and assigns the group name to this new object. If enabled, the
  log will only display a general summary for the group. If disabled, the log
  will detail which specific objects have degenerate triangles.

```{note}
These settings only apply during export. Once the export is complete, the scene
will revert to its previous state.
```

### Export Settings

- **Format** {cnum}`6`: selects the export format. The script is designed
  exclusively for the Dagor Engine, so only the DAG format is available.
- **Path** {cnum}`7`: displays previously saved export paths. These paths are
  automatically added when you select an export path using button **... Export
  PATH** {cnum}`8`.
- If the checkbox **Export to Max File Location** {cnum}`10` is enabled, list
  {cnum}`7` and button {cnum}`8` become unavailable. This option allows you to
  export to the directory where the current `.max` scene is saved.
- **Max Folder** {cnum}`9`: opens the directory containing the current `.max`
  scene in Windows Explorer.
- **Show prompt before export** {cnum}`11`: enables the display of logs after
  export. Any errors or warnings encountered during export will be shown.

### Naming Conventions

- **Change Names** {cnum}`12`: enables the addition of prefixes and suffixes to
  the names of exported objects. You can specify the prefix and suffix in text
  fields {cnum}`14` and {cnum}`13`, respectively. For example, if the object
  name is `OBJ_01`, by default, only the suffix `.lod00` will be applied,
  resulting in the final file name `OBJ_01.lod00.dag`.

### Collision Settings

- **Collision Settings** block allows you to export collision objects based on a
  prefix specified in text field {cnum}`16`. To enable this function, enable the
  **Add Collision** checkbox {cnum}`15`. The script will search for collision
  objects in the scene that match the prefix (default: `Cls_`) specified in
  field {cnum}`16`. If found, these objects will be included in the export.
  Otherwise, they will be exported as normal objects.

### Additional Controls

- **Export Selection** {cnum}`17`: initiates the export process. Ensure that at
  least one object with triangles is selected in the scene before proceeding.
- **Open Local Documentation** {cnum}`18`: opens a browser window with this
  documentation.
- **Contact with Developer** {cnum}`19`: displays the contact information for
  the script developer.

