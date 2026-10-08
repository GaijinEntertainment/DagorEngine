# Dagor Fix Normal Orientation Tool

## Installation

[Install the script](installation.md) following the provided instructions.

```{admonition} 3ds Max Version Requirement
:class: warning

This script requires **3ds Max 2013 or later**.
```

## Accessing Fix Normal Orientation Tool

1. Navigate to **Gaijin Tools** {cnum}`1` **> Fix Normal Orientation...**
   {cnum}`2`. This will open the main window of the Dagor Fix Normals
   Orientation script {cnum}`3`.

2. To verify the version of the script, go to **Gaijin Tools** {cnum}`1` **>
   About** {cnum}`4`. The **About** window will display the current version.
   It's important to check this regularly to ensure your script is up to date.

   ```{eval-rst}
   .. image:: _images/fix_normal_orient_01.png
      :alt: Fix Normal Orientation Tool
      :width: 50em
      :align: center
   ```

   ```{admonition} Plugin Version Requirement
   :class: warning

   Requires plugin version **1.4 or higher**.
   ```

## Using Fix Normal Orientation Tool

After exporting objects, you may encounter issues with normals in
[daEditor](../../../daeditor/daeditor/daeditor.md), as shown in the screenshot
below:

```{eval-rst}
.. image:: _images/fix_normal_orient_02.png
   :alt: Fix Normal Orientation Tool
   :width: 50em
   :align: center
```

The Fix Normal Orientation Tool is designed to resolve these issues in most
cases. To correct the normals:

1. In the Viewport, select the objects with incorrect normals.

2. Click the **Fix Normals from Selection** button.

This should correct the normals for the selected objects. In most cases, the
issue is resolved upon re-export. See the screenshot below for an example of the
corrected normals:

```{eval-rst}
.. image:: _images/fix_normal_orient_03.png
   :alt: Fix Normal Orientation Tool
   :width: 50em
   :align: center
```

