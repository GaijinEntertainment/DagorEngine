# Vromfs

## What is Vromfs?

*Vromfs* is a type of virtual file system used by Dagor games.

Files and directories are compressed into a single file with the extension
`.vromfs.bin`. This file can be both compressed and cryptographically signed
during creation.

Files are created and managed using `vromfsPacker-dev.exe`, which can also
unpack them. To use it, run the command:

```
vromfsPacker-dev.exe some_config.blk
```

The configuration file in [`.blk` format](../blk/blk.md),
contains rules for compression, specifying what to compress, the source and
destination paths of directories, compression settings, signing, etc.

You can open a `.vromfs.bin` file to inspect which directories and files are
compressed within each vrom file and identify their source paths. It is also
possible to decompress a vrom file manually.

Using vroms protects data from modification and significantly reduces load
times.

## Important Tips and Hints

- **Potential Issues:** Using the flag has drawbacks. For example, the
  disappearance of a file might go unnoticed because it would still be found in
  the vrom.

- **daNetGame Approach:** *daNetGame-based* games use a different concept.
  All add-ons and vroms are mounted to the game via a list in `settings.blk`,
  which also provides the source of the vrom. The flag
  `debug{useAddonVromSrc:b=yes}` tells the game to ignore vroms and instead
  mount the provided directories.

- **Advantages of Vroms:** Vroms are small and can hold vital information, such
  as scripts and game data, which allows updates or patches without a full patch
  (particularly important for consoles or phones where quick updates are not
  feasible). Consequently, vroms are updated in all games. In *WarThunder*, this
  is done via a special service that also synchronizes vroms between dedicated
  servers and clients. In *daNetGame-based* games, the game downloads these
  files at startup (only on consoles). Future updates may include downloading
  vroms before joining queues or in other scenarios.

