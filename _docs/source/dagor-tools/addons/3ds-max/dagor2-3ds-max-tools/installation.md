# Installation

```{important}
If you have already installed any of the Dagor 2 3ds Max Tools, you should
uninstall the previous version.

If you are installing the tools for the first time, skip the
[Uninstallation Steps](#uninstallation-steps) section.
```

## Uninstallation Steps

1. Go to **Customize User Interface**.
2. Choose **Menus** tab.
3. Delete **GaijinTools** from **Main Menu Bar**.
4. Now you can close the **Customize User Interface** window.

   <img src="_images/installation_01.png" alt="Uninstallation Steps" align="center" width="50em">

5. Close the 3ds Max.
6. Remove all `GJ_Tools\*.mcr` files from `usermarcos` directory. It should be
   located here by default:

   ```text
   C:\Users\USER_NAME\AppData\Local\Autodesk\3dsMax\YOUR_3DS_MAX_VERSION\ENU\usermacros
   ```

7. Remove all startup scripts, that you did not added yourself. Default path:

   ```text
   C:\Users\USER_NAME\AppData\Local\Autodesk\3dsMax\YOUR_3DS_MAX_VERSION\ENU\scripts\startup
   ```

## Clean Installation

For a clean installation, follow these steps:

1. Open **Customize > Configure User and Subsystem Paths**.
2. Go to **User and System** tab.
3. Select the **Additional Startup Scripts** line and click the **Modify...**
   button.
4. Specify path to `.../dagor2_toolbox/base`
5. Confirm the changes by clicking **OK**.

   <img src="_images/installation_02.png" alt="Clean Installation" align="center">

6. Restart 3ds Max. The script will automatically add the **Gaijin Tools** menu
   right after the **Help** menu.


