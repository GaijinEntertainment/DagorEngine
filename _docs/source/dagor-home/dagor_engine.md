## Introduction

The *Dagor Engine* is an open-source game engine by
[*Gaijin*](https://gaijinent.com/), continuously updated since 2002 for
high-performance gaming and realistic graphics.

## Platform Support

Supports *Windows PC*, *Linux*, *iOS*, *Android*, *Nintendo Switch*,
*PlayStation 4*, *PlayStation 5*, *Xbox One*, *Xbox Series X/S*.

## Games Created with the Dagor Engine

Powers games with detailed physics and large-scale environments, including:

- [**War Thunder**](https://warthunder.com/): Vehicular combat game with air,
  land, and sea battles across historical eras.
- [**Enlisted**](https://enlisted.net/): Squad-based WWII first-person shooter
  with large-scale battles.
- [**War Thunder Mobile**](https://www.wtmobile.com/): Mobile adaptation of *War
  Thunder* for iOS and Android.

## Key Features

- **Graphics Engine**: Supports *DirectX 12*, *Vulkan*, and *Metal* APIs.

- **Physics Engine**: *Dagor* supports third party open source industry
standard solutions for physics simulation: [**Jolt
Physics**](https://github.com/jrouwe/JoltPhysics) (preferable) and [**Bullet
Physics**](https://github.com/bulletphysics).

- **DaGI (Dagor Global Illumination)**

  - Optimized for efficient global illumination on huge dynamic scenes with low
    memory usage (less than 1ms on PS4!), and with raytracing support for modern
    hardware.
  - Simulates indirect lighting and light bouncing for realistic scenes:
    - [GDC talk: Scalable Real-Time Ray Traced Global Illumination for Large Scenes](https://enlisted.net/en/news/show/25-gdc-talk-scalable-real-time-ray-traced-global-illumination-for-large-scenes-en/#!/)
    - [Dagor Engine 5.0: light and shadows: Global Illumination, contact shadows, shadows on effects](https://warthunder.com/en/news/5338-development-dagor-engine-5-0-light-and-shadows-global-illumination-contact-shadows-shadows-on-effects-en)

- **[Darg (Dagor Reactive GUI)](../api-references/quirrel-modules/quirrel-modules/darg_framework/index.rst)**

  - Reactive UI framework based on
    [*Quirrel*](../api-references/quirrel-modules/dargbox/index.rst) scripting.
  - Supports stateful components and modular development with native code
    integration.

- [**Dagor ECS**](../api-references/dagor-ecs/index.rst)

  - Data-driven Entity Component System for modular game development.
  - Manages entities through templates, supporting parallel processing and cache
    efficiency.

- **[Render Framegraph](../api-references/dagor-render/index/daBFG.rst)**

  - Directed acyclic graph for defining frame rendering sequences.
  - Allows flexible integration of game-specific and engine features.

- **[daNetGame Framework](../danetgame-framework/index.rst)**

  - Modular platform for comprehensive game development.
  - Integrates core systems like rendering, physics, networking, and audio.

- **[Daslang](https://daslang.io/)**

  - Scripting language for interacting with core systems like rendering and
    physics.
  - Enables dynamic content management and engine functionality extension.

- **[DSHL (Dagor Shader Language)](../api-references/dagor-dshl/index.rst)**

  - Specialized language for shader creation and management.
  - Supports advanced features like conditionals, custom data types, and
    hardware-specific optimizations.

- [**Toolset**](../dagor-tools/index.rst)

  - [**Dagor Editor**](../dagor-tools/daeditor/daeditor/daeditor.md): Level
    editor with terrain editing, prefab placement, and plugin support.
  - [**Asset
    Viewer**](../dagor-tools/asset-viewer/asset-viewer/asset_viewer.md):
    Resource editor and viewer for special effects, particle systems, and
    animations.
  - [**Impostor Baker**](../dagor-tools/impostor-baker/impostor_baker.md): Tool
    for optimizing rendering with simplified 3D object representations.
  - [**Importers and Exporters**](../dagor-tools/addons/index.rst): Integration
    with *3ds Max*, *Maya*, and *Blender*.

- **Networking Subsystem**

  - Scalable, low-latency architecture for multiplayer games.
  - Supports TCP and UDP protocols, with built-in tools for debugging, lag
    compensation, and data compression.

- **Audio Engine**

  - Uses FMOD library for 3D sound, DSP effects, and multi-channel support.

- **Animation System**

  - Supports skeletal and procedural animation, blending, morphing, and various
    controllers, including Inverse Kinematics, additive animations and motion
    matching.


