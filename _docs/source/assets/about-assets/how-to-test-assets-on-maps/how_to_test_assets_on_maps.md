# How to Test Assets on Maps (Basic Workflow)

This guide provides essential steps for testing assets on maps. Note that there
are differences between *War Thunder* and *daNetGame*-based projects.

## Build Changes in Assets

If you've made changes to an asset, open it in [*Asset
Viewer*](../../../dagor-tools/asset-viewer/asset-viewer/asset_viewer.md) and
build the corresponding components:

- **Material changes:** Build the asset group (grp)

  <img src="_images/how_to_test_assets_on_maps_01.jpg" width="50%" align="center" class="bg-primary">

  <br>

- **Collision changes:** Build the collision group (grp)

  <img src="_images/how_to_test_assets_on_maps_02.jpg" width="50%" align="center" class="bg-primary">

  <br>

- **Texture changes:** Build the texture bin

  <img src="_images/how_to_test_assets_on_maps_03.jpg" width="50%" align="center" class="bg-primary">

  <br>

- **Multiple changes or uncertainty:** Build the entire folder (this may take
  longer depending on the number of assets and packs in the folder/subfolders)

  <img src="_images/how_to_test_assets_on_maps_04.jpg" width="50%" align="center" class="bg-primary">

  <br>

## Export the Map with the Asset

1. **Launch [*daEditor*](../../../dagor-tools/daeditor/daeditor/daeditor.md)**
   and open the map you want to modify. In the **Landscape** tab, place the
   asset using the **Create entity** tool.

   <img src="_images/how_to_test_assets_on_maps_05.jpg" width="80%" align="center" class="bg-primary">

   <br>

2. **Export the map** via **Project ▸ Export to game (PC format)**.

   <img src="_images/how_to_test_assets_on_maps_06.jpg" width="40%" align="center" class="bg-primary">

   <br>

3. **Confirm all dialog boxes** and choose the place to export level. A
   successful export will replace the current map file.

   <img src="_images/how_to_test_assets_on_maps_07.jpg" width="40%" align="center" class="bg-primary">

   <br>

## Launching the Level

The method for launching the level varies by project.

### daNetGame-based projects

The following steps related to *daNetGame*-based projects are described using
the *Enlisted* project as an example.

To see your map in the game, you should launch the scene that includes your map,
which is a `.blk` file. There are two main approaches:

**Using the Launcher:**

1. Run `/enlisted/game/z_enlisted_launcher.bat`
2. Select the correct mission from the dropdown (the one with your exported
   map). Customize mode, graphics settings, etc., if needed.
3. Click **Play** to launch the game.

**Manually:**

1. Create a `.bat` file in the `/enlisted/game/` directory.
2. Open it with a text editor (e.g., *Notepad*, *Notepad++*, *VSCode*).
3. Add the following line:

   ```
   @start win64\\enlisted-dev.exe -scene:gamedata/scenes/<your_scene_path>.blk -config:disableMenu:b=yes
   ```

4. Save and run the `.bat` file to launch your scene.

While the launcher is the preferred method, manual execution is useful for
testing new scenes that haven't been added to the launcher yet.

### Enlisted CDK

1. Launch `/Enlisted/modsEditor.bat`
2. Select your scene. Choose the team, mode, and other settings using the
   wrench icon.
3. Click **Load** or double-click the scene name to open it.

### War Thunder

In *War Thunder*, you will use the *Mission Editor*, which *daNetGame*-projects
do not have. You will have to create your mission using the following steps:

1. **Select Vehicle Type** from the dropdown:
   - **armada:** Aircraft
   - **tankModels:** Tanks
   - **ships:** Ships
   (Others are non-playable.)

   Place the unit on the map using the **Create Unit** button, then click the
   map to position it.

   <img src="_images/how_to_test_assets_on_maps_08.jpg" width="98%" align="center" class="bg-primary">

   <br>

   ```{note}
   Aircraft will appear in the air, while tanks and ships will be placed on
   land.
   ```

2. **Change Vehicle Properties:** If the wrong vehicle type is selected, modify
   it in the **Properties** panel (press `P` after selecting the unit).

   <img src="_images/how_to_test_assets_on_maps_09.jpg" width="98%" align="center" class="bg-primary">

   <br>


   After that you will have to choose:
   - Vehicle model
   - Weapon type (you may set to `default`)
   - Bullets (usually `HE` or `default`)
   - Bullets count

   It can be done in the same unit's **Properties** panel:

   <img src="_images/how_to_test_assets_on_maps_10.jpg" width="32%" align="center" class="bg-primary">

   <br>

3. **Configure Mission Properties:**

   - Find your unit's army number (different parties - different armies in pvp)
     in unit's **Properties** panel.

     <img src="_images/how_to_test_assets_on_maps_11.jpg" width="32%" align="center" class="bg-primary">

     <br>

   - Assign the unit's army number to **Team A** in **Mission Settings**
     panel (deselect unit and press `P` button).

     <img src="_images/how_to_test_assets_on_maps_12.jpg" width="32%" align="center" class="bg-primary">

     <br>

   - Specify your unit in **wing** inset.

     <img src="_images/how_to_test_assets_on_maps_13.jpg" width="32%" class="bg-primary">
     <img src="_images/how_to_test_assets_on_maps_14.jpg" width="32%" class="bg-primary">
     <img src="_images/how_to_test_assets_on_maps_15.jpg" width="32%" class="bg-primary">

     <br>

   In the same **Mission Settings** panel:

   - **name:** give the name to your mission;
   - **type:** select **singleMission**;
   - **level:** choose the location where the mission will start (otherwise, your vehicle
     will spawn on default water location by default).

     <img src="_images/how_to_test_assets_on_maps_16.jpg" width="32%" align="center" class="bg-primary">

     <br>

4. **Save the Mission:** Save your mission in
   `<project_name>/develop/gameBase/gameData/missions`.

   <img src="_images/how_to_test_assets_on_maps_17.jpg" width="98%" align="center" class="bg-primary">

   <br>

   ```{important}
   Be sure to save after every change – there is no autosave.
   ```

5. **Play the Mission:** Once ready, click the **Play** button.

   <img src="_images/how_to_test_assets_on_maps_18.jpg" width="98%" align="center" class="bg-primary">


