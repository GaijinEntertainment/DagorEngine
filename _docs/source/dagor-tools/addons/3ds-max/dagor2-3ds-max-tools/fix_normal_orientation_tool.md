# Dagor 2 Fix Normal Orientation Tool

## Installation

[Install the script](installation.md) following the provided instructions.

```{important}
This script requires 3ds Max 2013 or newer version to run.
```

## Accessing the Fix Normal Orientation Tool

1. Navigate to **Gaijin Tools** {bdg-dark-line}`1` **> Fix Normal
   Orientation...** {bdg-dark-line}`2`. This will open the main window of the
   Dagor 2 Fix Normals Orientation script {bdg-dark-line}`3`.

2. To verify the version of the script, go to **Gaijin Tools**
   {bdg-dark-line}`1` **> About** {bdg-dark-line}`4`. The **About** window will
   display the current version. It's important to check this regularly to ensure
   your script is up to date.

   <img src="_images/fix_normal_orient_01.png" alt="Fix Normal Orientation Tool" align="center" width="">


```{note}
Make sure that the plugin version is at least `1.4`.
```

## Using the Fix Normal Orientation Tool

After exporting objects, you may encounter issues with normals in
[daEditor](../../../daeditor/daeditor/daeditor.md), as shown in the screenshot
below:

<img src="_images/fix_normal_orient_02.png" alt="Fix Normal Orientation Tool" align="center" width="50em">

The Fix Normal Orientation Tool is designed to resolve these issues in most
cases. To correct the normals:

1. In the **Viewport**, select the objects with incorrect normals.
2. Click the **Fix Normals from Selection** button.

This should correct the normals for the selected objects. In most cases, the
issue is resolved upon re-export. See the screenshot below for an example of the
corrected normals:

<img src="_images/fix_normal_orient_03.png" alt="Fix Normal Orientation Tool" align="center" width="50em">


