# Packages

## How It Works

The rules that define which packages exist in the game and their dependencies
are specified in the
[`application.blk`](../../assets/all-about-blk/application_blk.md) file within
the `packages{}` block.

Example:

```
pkg_dev{  // package name
  PC:b=yes; iOS:b=no; and:b=no;  // platforms
  destSuffix:t="/../content.hq"  // if the package should be placed in content.hq (needed for HQ versions)
  "PC~~dedicatedServer":b=yes    // required on dedicated servers (HQ versions are not needed, standard ones are)
  allow_patch:b=false   // should patching be allowed (only disabled for pkg_dev, everything in production is patched)
  always_commit:b=true  // should the rebuild commit changes
  dependencies{  // dependencies
    "*" {base:b=yes;}  // base package
    pkg_cockpits {base:b=yes;}  // cockpit assets
  }
}
```

The `dependencies{}` block indicates that Package A, which specifies a
dependency on Package B, relies on Package B. However, Package B does not depend
on Package A.

## How to Add a New Package

To add a new package, define the new `pkg` in `application.blk`, modify the
necessary [`folder.blk`](../../assets/all-about-blk/folder_blk.md) files (export
settings for assets), run the [local build](resource_building.md#local-build),
and ensure that the dependencies are correct and no errors occur.

It’s not necessary to rebuild the [vromfs](vromfs.md) files after defining a new
package.

For the game to recognize the package content, you must specify it in the
settings file
[`settings.blk`](../../assets/all-about-blk/config_and_settings_blk.md) located
at `engine_root\<project_name>\develop\gameBase\_pc\settings.blk`. Add the
package paths in both the `addons{}` and `addons_no_check{}` blocks.


