# Dagor 2 Ambient Occlusion Baker

## Installation

[Install the script](installation.md) following the provided instructions.

```{important}
This script requires *3ds Max 2018* or newer version to run.
```

## Accessing the Ambient Occlusion Baker

1. Navigate to **Gaijin Tools** ▸ **Ambient Occlusion Baker**. This will open
   the main window of the **Dagor 2 AO Baker**.

2. To verify the version **(3)** of the script, go to **Gaijin Tools (1)** ▸
   **About (2)**. The **About** window will display the current version. It's
   important to check this regularly to ensure your script is up to date.

![Ambient Occlusion Baker](_images/ao_baker_01.png)

```{note}
Make sure that the plugin version is at least `1.4`.
```

## Using the Ambient Occlusion Baker

1. Open the tool panel by navigating to **Gaijin Tools** ▸ **Ambient Occlusion
   Baker**.

   ![Ambient Occlusion Baker](_images/ao_baker_02.png)

2. Click the **Bake Selected Object (1)** button to generate Ambient Occlusion
   (AO) for the selected object in the Viewport. Note that AO can only be baked
   for one object at a time.

3. The tool provides several options:
   - **AO Tint Color:** Choose the color for the AO backlight.
   - **AO Ambient Color:** Select the color for the minimum lighting of the AO.
   - **Display AO on Viewport:** Toggle the display of AO in the viewport
     (enabled by default).
   - **Transfer AO to Map Channel:** Choose which channel the AO results will be
     placed in (specific to *Dagor Engine*). By default, the AO is placed in
     channel 8.
   - **Visit toLearning Web Site:** Click to view this article.
   - **Contact with Developer:** Access the developer's web page.

4. To test the *AO Baker*, load the following test scene: [tree_AO_test_2021.max](https://drive.google.com/file/d/17sUOeN-S29WihYc2Vwcr5tq6_kDpTDpF/view?usp=drive_link).

5. Open the scene and select the object `tree_vitellaria_wide_c.lod01`.

6. For the test, configure the colors as shown below:

   ![Ambient Occlusion Baker](_images/ao_baker_03.png)

7. Leave all other settings at their default values. Click **Bake Selected
   Object** and wait for the progress bar indicating scene lighting to complete.
   The result should appear as follows:

   ![Ambient Occlusion Baker](_images/ao_baker_04.png)

8. To view the results, right-click on the Viewport and select the appropriate
   display option:

   ![Ambient Occlusion Baker](_images/ao_baker_05.png)

9. Then, change the selection from **Vertex Color** to **Map Channel Color** as
   shown below:

   ![Ambient Occlusion Baker](_images/ao_baker_06.png)

10. The AO should now be displayed correctly:

   ![Ambient Occlusion Baker](_images/ao_baker_07.png)


```{important}
- **Supported Model Types:** The script works correctly with **Edit Poly**,
  **Edit Mesh**, and **GrowFX** model types. Other model types are not
  supported, and using modifiers on the source model may cause errors when
  generating LODs or collisions.
```



