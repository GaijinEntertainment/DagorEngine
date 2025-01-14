# Vromfs

## What is Vromfs?

*Vromfs* is a type of virtual file system used by Dagor games.

Files and directories are compressed into a single file with the extension
`.vromfs.bin`. This file can be both compressed and cryptographically signed
during creation.

Files are created and managed using `vromfsPacker-dev.exe`, which can also
unpack them. To use it, run the command:

```text
vromfsPacker-dev.exe some_config.blk
```

The configuration file in `.blk` format, contains rules for compression,
specifying what to compress, the source and destination paths of directories,
compression settings, signing, etc.

```{seealso}
For more information, see
[.blk File Format](../blk/blk.md).
```

You can open a `.vromfs.bin` file to inspect which directories and files are
compressed within each vrom file and identify their source paths. It is also
possible to decompress a vrom file manually.

Using vromfs protects data from modification and significantly reduces load
times.

## Important Tips and Hints

- **Potential Issues:** using the flag has drawbacks. For example, the
  disappearance of a file might go unnoticed because it would still be found in
  the vromf.

- **daNetGame Approach:** daNetGame-based games use a different concept. All
  add-ons and vromfs are mounted to the game via a list in `settings.blk`, which
  also provides the source of the vromf. The flag `debug{useAddonVromSrc:b=yes}`
  tells the game to ignore vromfs and instead mount the provided directories.

  ```{seealso}
  For more information, see
  [settings.blk](../../assets/all-about-blk/config_and_settings_blk.md).
  ```

- **Advantages of Vromfs:** vromfs are small and can hold vital information,
  such as scripts and game data, which allows updates or patches without a full
  patch (particularly important for consoles or phones where quick updates are
  not feasible). Consequently, vromfs are updated in all games. In War Thunder,
  this is done via a special service that also synchronizes vromfs between
  dedicated servers and clients. In daNetGame-based games, the game downloads
  these files at startup (only on consoles). Future updates may include
  downloading vromfs before joining queues or in other scenarios.


