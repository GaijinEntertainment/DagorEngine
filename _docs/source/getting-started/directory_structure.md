# Directory Structure Overview

## General Directory Structure

At the top level of the *Dagor Engine* directory structure, the root directory
is organized into four main sections:

- **[Main Sources](#main-sources):** Contain the engine core and
  associated tools.
- **[Projects](#projects):** Contain files specific to individual game
  projects.
- **[Samples](#samples-1):** Include example projects for reference.
- **[Documentation](#documentation):** Provides comprehensive guides and
  references.

Below is a tree of the general directory structure:

```
DagorEngineRootDirectory/
├── <project_name>/
│   ├── prog/
│   │   ├── ui/
│   │   ├── scripts/
│   │   └── gameBase/
│   ├── develop/
│   │   ├── assets/
│   │   ├── levels/
│   │   ├── gui(ui)/
│   │   └── gameBase/
│   ├── game/
│   ├── tools/
│   └── application.blk
├── prog/
│   ├── _jBuild/
│   ├── 1stPartyLibs/
│   ├── 3rdPartyLibs/
│   ├── dagorInclude/
│   ├── engine/
│   ├── tools/
│   ├── scripts/
│   ├── gameLibs/
│   ├── daNetGame/
│   ├── daNetGameLibs/
│   └── samples/
├── samples/
└── _docs/
```

## Main Sources

The main sources directory `prog/` contains several key parts, each serving a
specific purpose:

### `_jBuild/`

Contains common JAM file settings used to build code for different platforms.

### `1stPartyLibs/`

Contains internal libraries. These libraries have no dependencies on the engine
or other internal projects, except for `3rdPartyLibs`, making them usable
outside of Dagor Engine projects.

### `3rdPartyLibs/`

Contains external libraries with no dependencies on Gaijin code. The code within
`3rdPartyLibs/` is entirely independent.

### `dagorInclude/`

Contains essential components and modules that form the core functionality,
including 3D rendering, animation systems, debugging tools, device drivers,
special effects, resource management, and much more. May have dependencies on
`1stPartyLibs/` and `3rdPartyLibs/`.

### `engine/`

Contains the core components of the Dagor Engine, encompassing a broad range of
functionalities critical for game development. This directory includes systems
for console processing, device drivers, game resources, base GUI components,
input/output systems, and so on. Most parts are optionally used across various
applications. They may have dependencies on `1stPartyLibs/` and `3rdPartyLibs/`.

### `tools/`

Contains the source code for various Dagor Tools.

### `scripts/`

Contains common scripts, primarily *Quirrel*, which is the main dynamic
scripting language used in the Dagor Engine.

### `gameLibs/`

Contains libraries for game development. Most of these libraries are
cross-platform. Dependencies are limited to `1stPartyLibs/`, `3rdPartyLibs/`,
`engine/`, and other `gameLibs/`.

### `daNetGame/`

Contains a Work-In-Progress (WIP) Entity Component System (ECS) framework for
building cross-platform network games. This codebase is subject to change, with
specific components such as AI, weapons, player customization, and terrain
expected to be moved to `daNetGameLibs/` and potentially rewritten using
*dascript* and *ECS* principles. Dependencies should only include
`1stPartyLibs/`, `3rdPartyLibs/`, `engine/`, `daECS/`, and some `gameLibs/`. The
goal is to support a general networking solution for any physical game, with
both dedicated and player-hosted servers (excluding peer-to-peer).

### `daNetGameLibs/`

Contains libraries that assist in developing games using the *daNetGame*
framework. Dependencies can include `1stPartyLibs/`, `3rdPartyLibs/`, `engine/`,
`daNetGame/`, `daECS/`, and some `gameLibs/`.

### `samples/`

Contains sample projects for both the core engine and game libraries are stored
here.

**Library Development Principles**

Libraries should adhere to the **FIRS(T)** principles:

- *Focused*: Each library should have a single responsibility.
- *Independent*: Libraries should minimize dependencies.
- *Reusable*: Code should be designed for reuse.
- *Small*: Keep libraries concise and manageable.
- *Testable*: Ensure libraries can be easily tested.

Developers are encouraged to move code that meets these principles into
`1stPartyLibs/` or `gameLibs/`, depending on the dependencies, rather than
keeping it within individual projects. This practice helps avoid code
duplication and increases the likelihood of code review and improvement.

## Projects

### General Architecture of Projects

We follow a standard directory structure for our game projects and recommend its
use to developers:

```
<project_name>/
 ├── prog/
 │   ├── ui/
 │   ├── scripts/
 │   └── gameBase/
 ├── develop/
 │   ├── assets/
 │   ├── levels/
 │   ├── gui(ui)/
 │   └── gameBase/
 ├── game/
 ├── tools/
 └── application.blk
```

### `prog/`

Contains the game's source code, C++ files, shaders, and often configuration
files directly used by the game.

### `ui/`

Contains scripts and configurations for the game's user interface, focusing on
elements such as controls, layouts, and components.

### `scripts/`

Contains script files and related resources that define and manage various game
behaviors, events, and logic.

### `gameBase/`

Contains essential elements related to game content and virtual file system
management for projects. This directory includes:

- **`content/`**: A subdirectory where game assets and content files are stored.
  This typically includes various game resources such as textures, models, audio
  files, and other data required for the game.
- **`create_vfsroms.bat`**: A batch script used to create virtual file system
  (VFS) ROMs, which are packaged file archives that optimize the loading and
  management of game assets.
- **`mk.vromfs.blk`**: A configuration file for the VFS ROM creation process,
  specifying how files should be packaged and organized within the virtual file
  system.

### `develop/`

Contains raw data, including the asset base, locations, images, and fonts for
the interface.

### `assets/`

Assets are the building blocks of the project, used to create locations and game
objects. Key characteristics of an asset include: *name*, *type*, and
*properties*. The properties specify which raw data to use and how to compile it
into the final game data.

Examples of asset types include `tex`, `dynModel`, `rendInst`, `composit`, and
`fx`. For instance, texture (`tex`) properties might include a reference to the
source image file (typically `.tif`, `.tga`, or another standard format), the
texture format (e.g., DXT1, DXT5), and the number of mipmaps with settings for
their generation from the source image. Similarly, other asset types have unique
properties necessary for their compilation.

**Asset Base**

The asset base is created by scanning asset directories. This process is carried
out by all our tools that work with assets (*dabuild* scans and then performs
the build, *AssetViewer* scans, creates the asset tree, and allows viewing the
base, *daEditorX* scans and allows using assets from the base for editing
locations).

During scanning, two methods are used to define an asset:

1. **Explicit Asset**: Defined by a `<ASSET_NAME>.<ASSET_TYPE>.blk` file
   containing the asset's properties.
2. **Virtual Asset**: Generated based on scanning rules described in
   `.folder.blk` files.

Scanning is performed recursively, traversing all subdirectories. Each directory
may contain a `.folder.blk` file that specifies:

- Whether to scan subdirectories (default is to scan).
- Whether to scan assets in the current directory (default is to scan).
- Where and under what name to compile assets from the current and nested
  subdirectories, and whether to do so.
- `virtual_res_blk{}` blocks used to generate virtual assets for arbitrary files
  found in the directory.

### `levels/`

Contains various game levels and their corresponding assets. Each subdirectory
represents a distinct level, containing all necessary resources and
configurations to render and manage that level effectively.

### `gui(ui)/`

Contains resources and scripts for building and managing the graphical user
interface (GUI). This directory is organized to handle font generation, UI
skinning, input configurations, and overall UI structure.

### `game/`

The build output directory, including compiled code and assets. This is
essentially what is delivered to the players.

### `tools/`

Contains data for tools, including shaders used for asset building and running
editors.

- **`dsc2-*`**: Shader compilers for different platforms, converting shader code
  written in our script format + HLSL into binary format usable on various
  systems (e.g., DX11, DX12, SpirV for Windows, Metal for macOS and iOS, SpirV
  for Linux and Android).
- **`vromfsPacker-dev.exe`**: Vromfs packer (virtual containers with read-only
  files for quick loading and game usage, structurally similar to .zip
  archives). We package vromfs to deliver 3-4 files to the player instead of
  3000-4000 scattered files.
- **`daBuild-dev.exe`**: Tool for building raw asset data into game resources.
  It takes data from the asset base (`develop/` folder) and after validation,
  compiles it into a format suitable for loading by the game in the `game/`
  directory.
- **`impostorBaker-dev.exe`**: Tool for preparing special tree view textures,
  used in conjunction with `dabuild`, to optimize tree rendering in the game
  (when a complex tree model is replaced with a simple texture that looks the
  same from a certain distance).
- **`assetViewer2-dev.exe`**: Asset viewer (and sometimes editor). Reads the
  asset base, presents it as a tree (where branches are asset directories and
  leaves are the assets themselves), and allows selecting an asset to view it in
  a viewport window and its properties in a properties panel. Some asset types
  (`fx`, `fastPhys`, `composit`) can be edited and saved upon exiting the
  viewer, while others (`animChar`, `physObj`) can be visualized and debugged
  dynamically.
- **`daEditor3x-dev.exe`**: Location editor. Allows editing terrain, water
  areas, and placing assets manually, along splines (lines), and across areas
  (polygons). It can compile/export the edited location into a format suitable
  for game loading (in the `game/` directory).
- **`csvUtil2-dev.exe`**: Utility for preparing language localizations from raw
  data (e.g., CSV from Crowdin) into a format readable by the game. It performs
  various checks (presence of translations for all languages, etc.).

### `application.blk`

The file sets the configuration for the asset base and the location of data
directories for the editor. In `application.blk`, can be defined:

- A list of asset types used in the project.
- A list of asset types that can be compiled into game resources using special
  exporter plugins.
- A list of root directories from which assets are scanned (the asset base).
- A set of settings for where and how to compile assets into the game (the
  `game/` directory).

## Samples

The `samples` directory includes example projects that demonstrate the usage of
the core engine and various game libraries. These sample projects serve as
practical references, showcasing different aspects of game development with the
Dagor Engine. They are designed to illustrate best practices and provide a
starting point for developers to build their own projects. Each sample is
carefully crafted to highlight specific features and functionalities, making it
easier for developers to learn and apply the concepts in their own work.

## Documentation

The documentation directory `_docs` contains comprehensive guides, references,
and resources for using and developing with the Dagor Engine. It includes
detailed manuals, API documentation, and tutorials designed to help developers
understand the engine’s capabilities and best practices. This section is
essential for both new and experienced developers, providing valuable insights
into the architecture, features, and functions of the Dagor Engine.


