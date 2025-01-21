## How to Build: Environment
Requirements for building and using the Dagor Engine toolkit: Windows 10 (x64), 16 GB of RAM, 200 GB of HDD/SSD space.

* Install Git: https://git-scm.com/download/win
* Install 7-Zip: https://www.7-zip.org/
* Install Python 3
* If you plan to use the FMOD sound library, also install FMOD Studio SDK 2.02.15

Create a project folder at the root of any drive (the folder name should not contain spaces or non-Latin characters).
```
md X:\develop && cd X:\develop
```

Clone the Dagor Engine source code and samples:
```
git clone https://github.com/GaijinEntertainment/DagorEngine.git
cd DagorEngine
```

Run the `make_devtools.py` script. This script will download, install, and configure the build toolkit. You should provide the path to the build toolkit folder as an argument, and the script will create this folder if it doesn't exist.

```
python3 make_devtools.py X:\develop\devtools
```

If the script is not run as an administrator, installers of certain programs may request permission for installation, which you should grant. If you plan to use plugins for 3ds Max, press 'Y' when the script asks if you want to install the 3ds Max SDK. The script will also ask to add the path X:\develop\devtools to the PATH environment variable and set the GDEVTOOL variable to point to this folder.

After the script completes its work, the X:\develop\devtools folder will be configured with the following SDKs and tools:

* Agility.SDK.1.614.1 - DirectX 12 Agility SDK
* astcenc-4.6.1 - Adaptive Scalable Texture Compression (ASTC) Encoder
* DXC-1.7.2207 - DirectX Compiler
* FidelityFX_SC - a library for image quality enhancement
* fmod-studio-2.xx.xx [optional] - FMOD sound library
* ispc-v1.23.0-windows - Implicit SPMD Program Compiler
* LLVM-15.0.7 - C/C++ compiler and libraries (Clang)
* max2024.sdk - 3ds Max 2024 SDK
* nasm - netwide assembler ver 2.x
* openxr-1.0.27 - library for AR/VR
* vc2019_16.11.34 - C/C++ compiler and libraries (MSVC)
* vc2022_17.9.5 - C/C++ compiler and libraries (MSVC)
* win.sdk.100 - Windows 10 SDK
* win.sdk.81 - Windows 8.1 SDK
* ducible.exe - a tool to make builds of Portable Executables (PEs) and PDBs reproducible
* pdbdump.exe - a tool for dumping the content of PDB files
* jam.exe - a small build tool that can be used as a replacement for Make

Restart the command line console to make the new environment variables available.

## How to Build: Prebuilt Binaries

You will need to download and extract additional binary files from the repository [https://github.com/GaijinEntertainment/DagorEngine/releases](https://github.com/GaijinEntertainment/DagorEngine/releases) into the X:\develop\DagorEngine folder:

* [tools-base.7z](https://dagorenginedata.cdn.gaijin.net/rel-fea0a2b3ae5acdb25e088e57a8fb1f77d5ba3e1d/tools-base.7z) - contains initial data for tools
* [samples-base.7z](https://dagorenginedata.cdn.gaijin.net/rel-fea0a2b3ae5acdb25e088e57a8fb1f77d5ba3e1d/samples-base.7z) - contains initial assets that will be compiled into binary files that will be loaded the game
* [tools-prebuilt-windows-x86_64.7z](https://dagorenginedata.cdn.gaijin.net/rel-fea0a2b3ae5acdb25e088e57a8fb1f77d5ba3e1d/tools-prebuilt-windows-x86_64.7z),
  [tools-prebuilt-linux-x86_64.tar.gz](https://dagorenginedata.cdn.gaijin.net/rel-fea0a2b3ae5acdb25e088e57a8fb1f77d5ba3e1d/tools-prebuilt-linux-x86_64.tar.gz),
  [tools-prebuilt-macOS.tar.gz](https://dagorenginedata.cdn.gaijin.net/rel-fea0a2b3ae5acdb25e088e57a8fb1f77d5ba3e1d/tools-prebuilt-macOS.tar.gz) - contains the prebuilt engine toolkit

The directory structure should look like this:
```
X:\develop\DagorEngine\tools\...

X:\develop\DagorEngine\samples\skiesSample\game
                              \skiesSample\develop
                              \skiesSample\prog

X:\develop\DagorEngine\samples\testGI\game
                              \testGI\develop
                              \testGI\prog
```

* prog - game source code
* develop - initial assets
* game - directory where assets are placed after building and game executable files are located

## How to Build: Build from Source Code

First unpack [tools-base.7z](https://dagorenginedata.cdn.gaijin.net/rel-fea0a2b3ae5acdb25e088e57a8fb1f77d5ba3e1d/tools-base.7z) to DagorEngine root to get mandatory binary files in their place.

If you are going to run samples unpack [samples-base.7z](https://dagorenginedata.cdn.gaijin.net/rel-fea0a2b3ae5acdb25e088e57a8fb1f77d5ba3e1d/samples-base.7z) to DagorEngine root (if you plan to only build EXE and shaders these binary data are not mandatory).

To build and run OuterSpace sample project unpack also [outerSpace-devsrc.7z](https://dagorenginedata.cdn.gaijin.net/rel-fea0a2b3ae5acdb25e088e57a8fb1f77d5ba3e1d/outerSpace-devsrc.7z) to DagorEngine root.

Then run `build_all.py` in DagorEngine root.

This builds the entire project toolkit from the source code. This process may take a considerable amount of time.
After tools are built and ready to be used script builds game and UI resources using daBuild and other utilities.

You can use direct build commands instead of using Python script.<br>
For example we will build "skiesSample" sample:<br>
* **To build code**, navigate to the `X:\develop\DagorEngine\samples\skiesSample\prog` folder and run the `jam` command (it builds `jamfile` script found in that folder).<br>After building the executable file will be placed in the `skiesSample\game` folder.<br>
* **To build shaders**, navigate to the `X:\develop\DagorEngine\samples\skiesSample\prog\shaders` folder and run any of `compile_shaders_*` scripts.<br>After building the shader-dump file will be placed in the `skiesSample\game\compiledShaders` folder.<br>
* **To build resources**, navigate to the `X:\develop\DagorEngine\samples\skiesSample\develop` folder and run the `dabuild.cmd` script.<br>After building the game resources will be placed in the `skiesSample\game\res` folder.<br>

You can also build everything for **Outer Space** sample game project or **daNetGame-based Scene Viewer** with their own scripts:<br>
* `X:\develop\DagorEngine\outerSpace\prog\build.py`<br>
* `X:\develop\DagorEngine\samples\dngSceneViewer\prog\build.py`<br>

Just be aware that these scripts use DagorEngine tools so they must be built beforehand or downloaded as prebuilt archive as described earlier.

`build*.py` script may accept optional parameters to limit components to be built:
`code`, `shaders`, `assets`, `vromfs`, `gui`.
If no parameters passed script builds all components.

`build*.py` script may accept optional parameters to force target architecture to be built, e.g. `arch:x86_64`, `arch:arm64`, `arch:x86`.
If none is specified code is built to default architecture (depends on host OS and jamfile settings).
Not all code applies target architecture now (only **Outer Space** and **daNetGame-based Scene Viewer**).

`build_all.py` script in addition to components accepts optional list of projects to be built:
`project:dagorTools`, `project:physTest`, `project:skiesSample`, `project:testGI`, `project:outerSpace`, `project:dngSceneViewer`.
If none is passed script builds all projects.

Example: `build_all.py project:dngSceneViewer code shaders` will build only code and shaders for **daNetGame-based Scene Viewer** project.

### Basic dagor samples

* Offline scene viewer : **East District**<br>
  [Code](https://github.com/GaijinEntertainment/DagorEngine/tree/main/samples/dngSceneViewer/prog) and built scene data [east_district-dagor.tar.gz](https://dagorenginedata.cdn.gaijin.net/rel-fea0a2b3ae5acdb25e088e57a8fb1f77d5ba3e1d/east_district-dagor.p1.tar.gz) to be unpacked to DagorEngine root<br>
  **east_district-dagor.p1.tar.gz** also contains prebuilt viewer app (for windows, macOS and linux) to run sample at once<br>
  [Demos of a new Gaijin’s game showcase Dagor Engine power](https://gaijinent.com/news/demos-of-a-new-gaijins-game-showcase-dagor-engine-power)<br>
  [East District review on YouTube](https://youtu.be/miABl6aekBA)
* Multiplayer sample: **Outer Space**<br>
  [Code](https://github.com/GaijinEntertainment/DagorEngine/tree/main/outerSpace/prog) and source (develop) files [outerSpace-devsrc.7z](https://dagorenginedata.cdn.gaijin.net/rel-fea0a2b3ae5acdb25e088e57a8fb1f77d5ba3e1d/outerSpace-devsrc.7z) to be unpacked to DagorEngine root<br>
  Prebuilt game (executables, shaders, vromfs, gameres) is available as [outerSpace-prebuilt-fullsrc.tar.gz](https://dagorenginedata.cdn.gaijin.net/rel-fea0a2b3ae5acdb25e088e57a8fb1f77d5ba3e1d/outerSpace-prebuilt-fullsrc.tar.gz)

### Documentation
  Automatically generated [Dagor Documentation](https://gaijinentertainment.github.io/DagorEngine/) contains general architecture description, API reference, tutorials and manuals.<br>
  It is not complete yet and will be extended.

## Open-source roadmap

We are going to open-source more parts of our Engine and tools.
These are general and broad plans for next year, can be changed.

### Documentation

* how to work with dagor assets
* daNetGame framework
