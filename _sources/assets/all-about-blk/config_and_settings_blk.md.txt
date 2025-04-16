# .config.blk and .settings.blk

Dagor-based games use a primary configuration file named `.settings.blk`
(referred to as `dgs_settings` in the game code) for fundamental settings. For a
detailed description of the file format, see
[.blk File Format](../../dagor-tools/blk/blk.md#blk-file-format) section.

## Purpose of .settings.blk

`.settings.blk` stores essential settings such as:

- Resolution
- Debug settings
- Video mode
- Sound settings

In general, any settings required before the game is fully initialized are found
here.

## Configuration Management

1. **Automatic Patching**: The engine automatically attempts to patch
   `.settings.blk` with `.config.blk`. When the game modifies settings, it saves
   them to `.config.blk`, which is then applied to `.settings.blk` upon loading.

2. **Allowed Overrides**: The permissible overrides are specified within the
   `__allowedConfigOverrides{}` block in `.settings.blk`. In non-release builds,
   all settings are allowed to be overridden.

3. **Command Line Modifications**: Any property that can be overridden may also
   be modified via the command line. For example, running `game.exe
   -config:debug/watchdog:b=no` is equivalent to adding `debug{watchdog:b=no}`
   to `.config.blk`.

## Game-Specific Adjustments

In certain games, the `.settings.blk` and `.config.blk` files are named
`<game_name>.settings.blk` and `<game_name>.config.blk`, respectively. The
`<game_name>` is provided via the `-game:<game_name>` command line argument or
specified in the code.

These mechanisms ensure a flexible and robust configuration management system
to meet different game requirements and development stages.

