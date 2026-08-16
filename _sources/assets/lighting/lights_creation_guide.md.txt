# Lights Creation Guide

## Main Point

The key rule when working with lighting is to carefully check the source
placement results. The radius of light sources should be sufficient to
illuminate the necessary parts of a room or area, but not exceed reasonable
limits. For instance, it’s inappropriate to use a light source designed for a
small guardhouse to illuminate a large hangar.

## Light Source

Templates for light sources are located in the `lights.blk` file within the
`<project_name>/prog` directory. For example, in *daNetGame*-based projects, it
can be found here:

`<project_name>/prog/gameBase/content/common/gamedata/templates/lights.blk`

Details about light source settings are discussed in the [Lights](./lights.md)
article.

**Key points:**

- For each new visual model, create and configure a new light source, including
  the visual model's name in the light source name (along with functional
  parameters such as light radius if needed).
- Prefer to use **spot** lights over **omni** lights.
- Light sources can be adjusted via the mission editor (`F12` in-game). These
  changes won’t be saved but can be previewed immediately.
- To change the time of day, use the browser-based tool [Var
  Editor](http://127.0.0.1:23456/editvar).
- Balance brightness and maximum radius when configuring light sources.
- If creating an **omni** light for outdoor use (outside an `envi_probe`),
  ensure the `light__use_box` parameter is disabled; otherwise, the light won’t
  be rendered outside.
- Avoid activating shadows if possible.
- If shadows are enabled, they should only apply to static objects. Dynamic
  objects may have a unique light source if necessary.
- Always check shadows in-game, as enabled shadows may degrade the scene in
  certain situations, depending on the light source's position relative to the
  visual model.
- If the light source's visual model can be destroyed, add the parameter
  `destroyable_with_rendinst:tag` to the light source template.
- If the light source is only active during nighttime (e.g., a street lamp), add
  the parameter `light__nightly:b=yes` to the template.

### IES Textures

Details on creating IES textures are covered in the [Photometric
Lights](./photometric_lights.md) article. Don’t hesitate to generate multiple
IES textures. Name them according to the light source and/or visual model.

### Flickering Source

For more information on flickering light sources, refer to the [Flickering
Lights](./flickering_lights.md) article.

To correctly set up a flickering light source, the following entities must be
created:

1. A flickering light source.

   ```{note}
   This is not the same as a light source; refer to the
   [article](./flickering_lights.md) for details.
   ```

2. A dynamic visual model with an animation controller.
3. A template linking the animChar of the dynamic model to the flickering
   source. Don’t forget to specify destructibility if the dynamic model can be
   destroyed.
4. A gameObj template linking the animChar of the dynamic model to the
   flickering source.
5. A light source.
6. A template linking the light source to the flickering source.
7. A gameObj template linking the light source to the flickering source.
8. A composite file combining the two gameObjects: one for linking the
   flickering source to the dynamic model and the other for linking the
   flickering source to the light source.

## Visual Model

This refers to the geometric model of the light source, such as a lamp,
spotlight, torch, or any other visual representation of a light source.

Consider whether the light source requires flickering. For emergency lights,
torches, and most lamps (since many can have flickering variants), create a
dynamic model with an emissive surface and, if necessary, a render instance
base. The logic behind separating the visual model into render instances and
dynamic models, as well as their preparation, is detailed in the [Flickering
Lights](./flickering_lights.md) article.

### Emission Strength

There are no exact values for emission strength since it depends on factors like
the albedo, emission mask, and the size of the emissive surface. Configure the
emission strength individually, keeping in mind that lower values are generally
better. Excessive emission can disrupt the camera's adaptation and negatively
impact gameplay.

More details on configuring emission can be found in [this
article](../shaders/dng-shaders/rendinst_emissive.md).

## GameObj

```{seealso}
For more information, see
[gameObjects](../about-assets/gameobjects/gameobjects.md).
```

Create a gameObj for both the light source and the dynamic visual model. In
*daNetGame*-based projects, for example, they are located here:

`<project_name>/develop/assets/common/gameRes/gameObjects`

For convenience, ensure to define a reference model in the gameObj. For example:

`ref_dynmodel:t="lamppost_d_glass_flicker_dynmodel"`

This is necessary to display the geometry in the
[*daEditor*](../../dagor-tools/daeditor/daeditor/daeditor.md).

## Restriction Box

The *Restriction Box* is a gameObj used to cut off light. It’s required for
light sources that don’t cast shadows, to prevent light from passing through
walls. Typically, it’s not included in the composite file with the light source,
but rather placed manually at the location or added to the composite file for
the entire building.

The object is named:

`omni_light_restriction_box.gameObj.blk`

## Composite

[Composite](../all-about-blk/composit_blk.md) file can use the same visual model
but have different light sources. In such cases, make sure to include the light
source functionality in the composite file name. Always specify the radius in
the names of composites containing light sources, e.g.:

`factory_lamp_a_flicker_10m_cmp.composit.blk`

A composite containing a light source may consist of:

- **RendInst** – static base, which can be destructible and include destruction
  logic.
- **GameObj** template describing the light source or linking the light source
  to the flickering source.
- **GameObj** template using a dynamic model, in case the light source has a
  flickering variant.
- **Restriction Box** for cutting off light, though it is often placed
  separately and not included in the composite with the light source.

## Placing Light Sources

Remember to account for the light radius when placing light sources. The same
light source cannot be used to illuminate both a small room and a large hall.
This would either leave the hall under-illuminated or cause the small room to be
over-lit. Excessive brightness not only breaks visual immersion but also
disrupts gameplay.


