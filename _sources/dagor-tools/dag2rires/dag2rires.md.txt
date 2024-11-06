# Overview

The *dag2riRes* tool is used for the following purposes:

- Converting `.dag` files into `.composit.blk` files.
- Generating multiple `.dag` files from objects within a single `.dag` file.

The tool uses two key files:

- `dag2riRes-dev.exe`
- `dag2riRes-dev.pdb`

# Usage Recommendation

For ease of use, it is recommended to create a separate directory where you can
copy the tool. Avoid placing this directory within the project resources, as
temporary files can accumulate there, potentially disrupting the build process
and generating errors in the editor logs. While these issues are usually minor
and can be resolved by routine cleanup, in cases like research or iterative
testing, where frequent exporting, testing, and deleting is necessary, it is
better to keep the tool in a dedicated directory. In the created directory,
duplicate the `*.exe` and `*.pdb` files.

# Generating a .composit.blk File

To generate a `.composit.blk` file:

1. In *3ds Max*/*Blender*, either create a new object if it is not yet available
   in the asset database, or import it from an existing `.dag` file.
2. Name the object either after the original `.dag` file from which you imported
   the geometry or after the `.dag` file you want to generate from the object
   using *dag2riRes*.
3. Add a three-digit suffix such as `_000`.
4. If you assign the suffix `_origin` to any object in the scene, its pivot
   point will be used as the zero-coordinate in the resulting composite file.

   ```{note}
   If the `.dag` file being processed with *dag2riRes* lacks a node named
   `*_origin`, the pivot in the resulting composite file will not match the
   pivot of the original `.dag`, and will be offset instead.
   ```
5. It is recommended to clone similar geometry in the scene as a reference (or
   instance).
6. Export the `.dag` scene to the directory containing the *dag2riRes* tool.
7. The composite file will be generated in the root directory, next to the
   executable file.

For more detailed instructions on creating `.composit.blk` files, please refer
to the relevant documentation.

See more details about composite files
[here](../../assets/all-about-blk/composit_blk.md).

# Rules for Generating Individual .dag Files

When generating individual `.dag` files:

- `.dag` files will be created based on similar names. For a set of similarly
  named objects in the scene, only one `.dag` file will be generated based on
  the first object in the numerical order. For example, from the objects
  `test_geometry_001`, `test_geometry_127`, and `test_geometry_origin`, only one
  `.dag` file will be created, named `test_geometry`, based on the first object
  (`test_geometry_001`).
- `.dag` files will automatically be assigned a `.lod00` suffix; no other
  suffixes are supported.
- The `.dag` files will be located in the `\simple_dags\p_RendInst` directory.
- `.rendinst.blk` files will be generated alongside the `.dag` files, but they
  are generally not useful.

# Running the dag2riRes

To run `dag2riRes-dev.exe`, it is recommended to use *FAR Manager*. Upon
launching, the log will display a brief instruction:

![Running the dag2riRes](_images/dag2rires_01.png)

Enter the command in the command line. At a minimum, you need to specify the
name of the `.dag` file you are processing and the target directory. For
example:

```
dag2riRes-dev.exe -s:test.dag -d:simple_dags
```

All other parameters are optional.


