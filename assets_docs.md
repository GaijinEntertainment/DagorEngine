# .folder.blk and assets phylosophy


**Table of Content:**
- [General Introduction about Assets](#general-introduction-about-assets)
- [Syntax description .folder.blk](#syntax-description-folderblk)
  + [General principles](#general-principles)
  + [Asset scanning and export rules](#asset-scanning-and-export-rules)
    + [Basic parameters](#basic-parameters)
    + [virtual_res{} blocks](#virtual_res-blocks)
    + [Asset types and rules for them](#asset-types-and-rules-for-them)
         + [texture](#texture)
         + [dynmodel, rendinst](#dynmodel-rendinst)
         + [Composite dynamic models](#composite-dynamic-models)


## General Introduction about Assets

Assets is a universal name for all types of resources in Dagor tools.

> **Tip:**
> the list of supported resources in your application can be found in `application.blk`. Also, there you can find a lot of useful 
> information for configuring the tools for your project.

`assets { types {`
there is also the base path to the folder with the assets **base:t=**


**Assets are divided:**
- used only by the editor, e.g. for exporting levels. Examples: materials, prefabs, spline classes, etc.
<br></br>
- exported to the game (block export { in `application.blk` -> `assets`) stored in the game in `.grp` files.
  For speed, the location editor uses exported resources to the game.
  <br></br>  
- Textures. They are usually exported to the game, but unlike other resources, are stored in `.dxp.bin` files, and the editor uses a cache of textures, not exported textures.
So you can change textures and look for the changes immediately.


Assets for game and tools are `.blk` files of two kinds: modern kind `<asset name>.<resource type>.blk`. 
For example, `my_house.rendinst.blk`. And old kind `<asset name>.blk`, inside the `.blk` file must be specified the type of asset.

Inside the `.blk` file resource are all the parameters the exporter needs for each specific resource. 
Each resource is its own plugin for the build\tools and a separate code in the game.


Sometimes, the description is minimal – for example, for a texture, you need the texture name if it is in `.dds` format.

In some cases, for example, in Effects, it is the whole asset, and AssetViewer is its editor.

Most assets do not have tools to edit them; the main creation mechanism is editing and overloading on the fly with change tools. Unfortunately, changes to `.folder.blk` are not currently supported.

However, **in order not to create every** simple asset, like textures, skeletons, dinmodels, etc `.blk` by hand, Dagor supports a system of creating virtual assets.
This is done with a `.folder.blk` file placed in the asset folder.

In fact, 99.99% of it is enough to place textures and models in the right folders, and all the numerous rules – ranges of switching lods, rules of texture conversion, and so on and so forth will be created automatically.

> **Warning!**
> 
>Resources must be named with a unique name. In most cases, everything works if resources are named uniquely within their type, but it is desirable to have a completely unique name.

 ## Syntax description .folder.blk
     
### General principles

`.folder.blk` specifies how and where the assemblies are exported, how they are searched, and the rules for creating virtual `.res.blk`.
For assemblies, the `.res.blk` rules are applied first:

- if there are any, i.e. if there is `my_asset.dynmodel.blk`;
- the rule that creates an asset named `my_asset` will not be executed;
- the rules from `.folder.blk` next to the asset are applied;
- the rules from `.folder.blk` higher in the folder hierarchy until the directive, `don't process this file anymore, stopProcessing` is received.

To understand which rules apply to your file - for example, texture `my_texture.tga` or model `my_model.dag` - you need to start from the folder where the asset is located and look for the nearest `.folder.blk` up the hierarchy.

Read also [BLK](https://info.gaijin.lan/display/DAG/BLK).
                                                             
### Asset scanning and export rules

#### Basic parameters

- `inherit_rules:b = false`, the rules from .folder.blk are **not applied to the hierarchy**.
- `scan_assets:b= true\false` – scan assets in this folder.
- `scan_folders:b= true\false` – scan sub-folders in this folder.

>**Important:**
> it is highly recommended to use a special folder with checkboxes if you are going to make test assemblies that you want to commit and pass on to others, but not include for everyone.
   
- `exported:b=` – whether the content is exported to the game.

#### block export {}

export{

- `ddsxTexPackPrefix:t`, vgameResPackPrefix:t` – prefix before the package name;
-`package:t =` – names of additional resource packs. * means put in root pack (default value).
- `gameResPack:t`,`ddsxTexPack:t=` – names of resource/texture packs from this folder and all lower in the hierarchy (unless another rule is introduced).

Directives:
- *name_src – the name of the folder will be used as the name of the pack. Different folders with the same name will have resources in one pack;
- *path_src – relative path to the folder; 
- *name - WIP;
- *path – WIP;
- *parent – the name of the parent folder will be the name of the package.

#### virtual_res{} blocks

Assets are created here.

They are applied according to the order in which they are written in the file.
First the top one, then lower and lower, then from `.folder.blk` up the hierarchy, until the `stopProcessing:b=true` directive is encountered or all `.folder.blk` up the hierarchy run out.

##### Basic parameters

```
virtual_res_blk{
   find:t="^(.*)\.dds$"
   exclude:t="^(.*_nm)\.dds$" 
   className:t="tex"
   name:t = "low_$1" stopProcessing:b=false

contents{} }
  ```

- **find** – [regex](https://learn.microsoft.com/en-us/dotnet/standard/base-types/regular-expression-language-quick-reference) of file search rules, for which a virtual `res.blk` is created;
- **exclude** – exclude regex. Throws out the results of find;
- **className** – one of the types of assemblies in your project. See `application.blk`);
- **name** – the name of the asset. By default, it corresponds to the value in the first parentheses of the regex parameter of find (i.e. name:t="$1"). You can make any constructions from values in brackets (parameters $1 ...$9) and symbols.
- **stopProcessing** – defaults to true. This means that no more virtual res blk rules will be applied to the found file.

If you want to make many assemblies from one texture/model e.g. dinmodel, render, prefab, skeleton, character, etc., remember to set this parameter to `false` state.

All other parameters are set in the `contents{}` block and are usually specific to a particular asset.

Inside the `contents{}` block there can be *specialization* blocks by tags. The tag set is formed by all folder names in the
full path to the file not including the base path to the assemblies. For example, for develop/assets/test/warthunder/model/abc.tga the tags will be `test warthunder model`, the names of specialization blocks have the form `"tag:TTT" {}`.

Example:

```
contents { 
   hqMip:i=1
   //... 
   "tag:warthunder" {
   hqMip:i=0 
   }
  }
```

If specialization blocks are present and applicable (the full path has the specified tags), 
the contents of the blocks are merged on top of the main `contents {}` for that asset.

If multiple specialization blocks are applicable (for the full path of the asset), 
their contents are merged in the same order in which the blocks are written inside the `contents {}`.

This way you can set different properties for the same type of assemblies, which are located in 
different folders with a specific name, without copying `.folder.blk`.


As a rule, it is not necessary to specify the file that should go in the parameter of the resource – for example, the name of the texture or `.dag` file - they
are passed by default and are equal to the name of the file from which the resource is created. However, you can specify another one. For example, the file `my_dynmodel.txt` 
to create a dynmodel with a link to `dag_my_dynmodel.lod00.dag`, etc.).

Parameters common to most assets: `ddsxTexPack:t`, `gameResPack:t`, `package:t=` (see above)

`export_PC:b` (instead of PC it can be any 4cc platform: _PS4, _and, ios, *PS3, etc.* – `see application.blk`) - whether to export an asset for a given platform

### Asset types and rules for them

#### texture

A brief overview of texture asset parameters.

>**Reminder:** 
>all of them are not mandatory, and for regular diffuse textures in `.dds` format you can set nothing at all, 
and for `tga` – two parameters, `convert:b=yes`, `fmt:="DXT1|DXT5"`).

```
contents{
   convert:b=yes;fmt:t="DXT1|DXT5" mipmapType:t=mipmapGenerate mipFilter:t="filterKaiser";mipFilterAlpha:r=40;mipFilterStretch:r=2
   addrU:t="wrap";addrV:t="wrap";
   swizzleARGB:t="RAG0"
   hqMip:i=3;mqMip:i=3;lqMip:i=4
   PC{
   hqMip:i=2;mqMip:i=2;lqMip:i=3; 
   }
   gamma:r=1
   colorPadding:b=no
   // alphaCoverageThresMip9:i=200 // alphaCoverage:b=no
   // rtMipGen:b=no
   // aniFunc:t="abs";
   // anisotropy:i=8
   addrU:t="wrap";addrV:t="wrap";
```

- **convert** – convert texture. For all TGA or PSD textures, it is obligatory to specify;
- **fmt:t** – format to convert to. Values DXT1, DXT5, DXT1|DXT5 (automatically select depending on the alpha), ARGB, L8, A8L8, L16;
- **mipmapType:t=** – how to get mip levels. Valid values = `mipmapGenerate`, `mipmapUseExisting`, `mipmapNone`;
- **mipFilter** – parameters of mip levels generation (allowed values – `filterKaiser`, `filterBox`, `filterCubic`);
- **hqMip** – if not equal to =0, the texture is down sampled. `mqMip` and `lqMip` – what mip to take at medium and low-quality settings;
- **swizzleARGB** – how to swizzle the texture, the letters indicate where to write the corresponding channel. For most textures, it is not necessary to specify;
- **gamma** – gamma value for gamma-correct rendering. For normal maps and masks, – it is necessary to specify 1. By default, 2.2;
- **addr**, **addrU**, **addrV** –texture addressing. Default wrap, allowed values wrap,clamp,border; 
- **colorPadding:b=** —padding for alpha textures;
- **PC{} blocks and other 4cc platforms** – possibility to specify other values for other platforms.

#### dynmodel, rendinst

```
lod { range:r=70; fname:t="$1.lod00.dag";}
lod { range:r=200; fname:t="$1.lod01.dag";}
lod { range:r=550; fname:t="$1.lod02.dag";}
lod { range:r=4000; fname:t="$1.lod03.dag";}
ref_skeleton:t="$1_skeleton"
texref_prefix:t="low_"
all_animated:b=yes         
```

- **lod block** – LoD. parameters;
- **range:r** – range of lod;
- **fname:t** - optional parameter, by default, it is `$1.lod<number of lod block{}>.dag`;
- **texref_prefix:t=** – prefix for texture name, added to all references to textures in the model `ref_skeleton:t=` – only for dinmodels, needed for preview in assetViewer with correct skeleton;
- **all_animated:b=** – indication that all objects of this model should be considered animated, i.e. having eigenmatrices.

 #### Composite dynamic models

The essence is the construction of multiple dynamic models that use one common skeleton. 
Below are the rules for creating `*.skeleton.blk` for a composite model, 
let's see the example of a tank, to which we will add several variants of turrets and cannon:

```
name:t="tank_body.lod00.dag"

attachSubSkel {
 attach_to:t="bone_turret"
 skel_file:t="turret_a.lod00.dag"
 skel_node:t="bone_turret"
 
  attachSubSkel { 
    attach_to:t="bone_gun_a"
    skel_file:t="gun_a.lod00.dag"
    skel_node:t="bone_gun_a"
  }
  
  attachSubSkel { 
     attach_to:t="bone_gun_b"
     skel_file:t="gun_b.lod00.dag" 
     skel_node:t="bone_gun_b" 
     add_prefix:t="G1:"
  }

  attachSubSkel { 
     attach_to:t="bone_gun_c"
     skel_file:t="gun_b.lod00.dag"
     skel_node:t="bone_gun_b"
     add_prefix:t="G2:"
  } 
}

attachSubSkelc{ 
 attach_to:t="bone_turret"
  skel_file:t="turret_b.lod00.dag"
  skel_node:t="bone_turret"
  add_prefix:t="T1:"
}
```

- **name:t=** – parent name;
- **attachSubSkel** – block of an added dynamic model;
- **attach_to:t=** – node in parent skeleton to which we link additional dynamic model;
- **skel_file:t=** – child name;
- **skel_node:t=** – node in the child's skeleton, to which the child is linked to the parent's skeleton;
- **add_prefix:t=** – prefix for all child nodes.

More about what happened in `*.skeleton.blk` above. To the skeleton generated by the `tank_body` dag we added `turret_a` and `turret_b` turrets, landing them on bone_turret.
In turn, we planted two unique guns `gun_a` and `gun_b` on the `turret_a` turret on the `bone_gun_a`, `bone_gun_b`, and `bone_gun_c` nodes, but we planted `gun_b` twice on different bones.
Since we are creating one generic skeleton, obviously it cannot have nodes with the same name. 
To solve this problem, we set a different prefix for the nodes of each duplicate. The nodes of the two copies of `bone_gun_b`, which form different branches in the hierarchy, got prefixes G1 and G2.
And the nodes of `turret_b` get the prefix T1.


>**Important:**
>1. when specifying a binding node, do not consider automatically added prefixes.
>2. if a child sits with a bone on a bone of the same name, the previous bone is removed from the hierarchy, there will be no duplicate, so don't worry.
   there will be no duplicate, don't worry.

Theoretically, we can create a very long chain of dependencies, but the simpler the structure, the easier it is to work with it and before creating a new step - we need to make sure that it is necessary.

A multi-stage hierarchy would look like this, for example:

```
  name:t="papa.lod00.dag"
  
attachSubSkel {
 attach_to:t="bone_papa"
 skel_file:t="child.lod00.dag"
 skel_node:t="bone_child"
 add_prefix:t="layer01:"
 
attachSubSkel {
 attach_to:t="bone_child" 
 skel_file:t="child.lod00.dag"
 skel_node:t="bone_child" 
 add_prefix:t="layer02:"
 
      attachSubSkel { 
        attach_to:t="bone_child"
        skel_file:t="child.lod00.dag" 
        skel_node:t="bone_child" 
        add_prefix:t="layer03:"
        
          attachSubSkel { 
            attach_to:t="bone_child"
            skel_file:t="child.lod00.dag" 
            skel_node:t="bone_child" 
            add_prefix:t="layer04:"
      }
    }
  }
}
```

