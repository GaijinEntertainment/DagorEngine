# Lens Flare

## What this lib does

This lib adds lens flares really similar to what Unity3D is capable of. Features in a nutshell:
* Configure lens flares in ECS (configuration is documented here: lens_flare/templates/lens_flare.template.blk)
* Add lens flare provider light sources in ECS (reference the lens flare config by name)
  * This can be added to the sun
  * And omni- and spotlights (these are restricted to a single global lens flare config. Two different lights can't have two different flares)
* Draw the lens flare components:
  * Draw regular shapes with optional rounding on the vertices
  * Apply gradient and texture to the shapes
  * Use the light source's brightness and color
* Occlusion/attenuation is calculated from
  * Solid objects (using gbuf depth)
  * Volumetric fog
  * Sun only: Transparent effects (based on fom shadows)
  * Dynamic lights: IES textures
  * Spotlights: angular attenuation
  * Any point light: Distance attenuation (uses a formula without radius limitation)

## How it works (Overview)

The logic is implemented inside a single renderer class: LensFlareRenderer

This is a singleton, managed by an entity (template: 'lens_flare_renderer')

The renderer is marked dirty whenever the lens flare configuration entities are modified. This normally doesn't happen during the game, but this allows real-time editing of the configs.

When the renderer is marked dirty, it queries all lens flare configurations and fully recreates the necessary resources.

### Rendering

The rendering happens in two phases: prepare and render. Both are done from FG nodes.

The preparation phase queries lens flare providers (light sources), and computes their visibility, position on the screen and current intensity.

The rendering phase issues the draws calls. All flare components (shapes) are applied to the render target additively.

Currently, the RT is a separate texture at half resolution. It is later applied in the postfx shader. The shader uses pre-exposure and outputs linear values. It could be directly applied to the frame between the TAA/TSR resolution and the postfx shader (but that's used as a history for the TAA/TSR).

## Implementation details

### Preparation

The preparation phase is divided into two parts:

#### Pre-culling

Pre-culling treats light sources as single points are runs all checks accordingly. The depth test is done based on downsampled far depth. The mip level is selected based on the occlusion area used later.
This step is done separately for manual and dynamic lights:

##### Manual flares

These flares come from manually provided sources, meaning that the C++ code knows all data of the light source (like the sun) and it is provided there.

These light sources are collected into a buffer, grouped by the flare config id. A single compute shader pass is dispatched to prepare/cull them at once.

##### Dynamic lights

These lights are pre-culled by the lights manager and are available in a buffer on the GPU, along with their masks (which contains a bit to mark lights to use flares).

The compute shader is dispatched two times: one for the omnilights and one for the spotlights (if such lights are present in the view frustum and a lens flare config is available for them). The shader checks all visible lights, but culls those whose mask doesn't contain the lens flare bit.

#### Occlusion test

The occlusion check uses a compute shader on **only** the pre-culled instances and tests depth in an area of 16x16 around the light. The flares intensity is scaled proportional to the number of pixels that passed the depth test.

### Vertex buffers

The flare components (shapes) are grouped by a few parameters:
* Texture
* Roundness: Sharp, rounded, circle

All groups are rendered with a separate draw call. This is because, on one hand the texture is problematic if I need to specify an array of differently sized textures in a single draw call, on the other hand the different shapes can be optimized separately using shader variants. The rounded shape is the slowest, and the sharp is the fastest.

When the renderer initializes, it creates vertex and index buffers for the flares. There is a single vb and a single ib in the renderer, but all flare configs get their separate regions inside the buffers. Within a flare config, each flare component group (based on texture and roundness) gets its own region too.

Every flare component (shape) has its own region inside the vb and ib, because that's how the shader knows the flare component's id => retrieves the data based on it.

Each shape uses a triangle fan as geometry: apart from the outer vertices, there is a central vertex and all triangle are formed between the central and two neighbouring outer vertices. The triangles go around, but the first/last vertex is not the same vertex! It's created two times in the same location. These construction details are important, since they enable some optimizations:
* Linear distance from edges is calculated by the interpolator
* Closest outer vertex to a pixel is calculated by interpolating the vertex id (interpolation would fail between the 0 and sideCount-1, that's why first vertex != last vertex)

### Roundness

I give this feature a separate section here, because it's not trivial to do efficiently.

This logic doesn't apply to sharp shapes and circles. Those are optimized.

The roundness parameter in the config defines what percentage of the length of the sides of the shape is rounded down.

Based on the side count and the roundness parameter, we can calculate a **rounding circle** for each vertex. The position of this circle can be expressed as a normalized distance: where it is between the central and the outer vertex (*rounding circle pos = lerp(center, outerVertex, normalizedDistance)*).

The pixel shader needs to be able to:
* Discard pixels outside the rounding circle, but only at the vertices
* Calculate the distance from the edge of the shape

#### Identifying the rounding circle close to a pixel

All outer vertices store their vertexId. This value goes from \[0\.\.sideCount\]. Pos\[sideCount\] = pos\[0\]. This is the repeated vertex. These are all outer vertices. The central vertex must use 0 as vertexId.

The vertexId is passed to the pixel shader, and it's linearly interpolated. If we treat the interpolated vertexId together with the normalized distance from the edge (1-distance), we get an 1D homogeneous coordinate. Dividing by the distance, we get the interpolated vertexId between the two outer vertices, ignoring the center: we are projecting the vertexId to the edge.

At this point:
* round(projected vertexId) = id of the closest vertex
* abs(round(projected vertexId) - projected vertexId) = distance from the closest outer vertex projected to the edge of the shape => can be compared with the roundness

#### Early exit logic

1) If the normalized distance from the edge is too great, rounding circle won't affect the pixel
2) Comparing the projected vertexId (the above calculation) with the roundness, we can skip further pixels

As a result, the pixels that are not skipped are confined into a shape bounded by:
* the rounding circle's center
* the points, where the rounding circle touches the edges of the shape

#### Rounding

The vertex positions are stored in a separate buffer. These allow the calculation of the closest rounding circle without relying on expensive trigonometric operations. The buffers are small and the shader reads them in a very cache-efficient way.

The rest of the logic is quite simple.