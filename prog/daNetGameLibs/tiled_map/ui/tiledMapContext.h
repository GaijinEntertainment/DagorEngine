// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_picMgr.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/updateStage.h>
#include <EASTL/hash_map.h>
#include <ecs/render/updateStageRender.h>
#include <ecs/render/updateStageRender.h>
#include <math/integer/dag_IPoint2.h>
#include <render/renderEvent.h>
#include <shaders/dag_postFxRenderer.h>
#include <sqrat.h>

namespace darg
{
class Element;
}

struct TileHandle
{
  PICTUREID picId = BAD_PICTUREID;
  TEXTUREID texId = BAD_TEXTUREID;
  d3d::SamplerHandle smpId = d3d::INVALID_SAMPLER_HANDLE;
};

// could be an `eastl::fixed_string`, but `eastl::hash_map` doesn't support it with default hash function
using QuadKey = eastl::string;
using TilesHashMap = eastl::hash_map<QuadKey, TileHandle>;

/**
 * @brief Tiled map context, it handles the zoom and position of the map, and loads required tiles
 * @note It's a singleton class
 *
 * @warning The world coordinate system is left-handed, but the cross product defined as right-handed.
 *          Should be careful with it.
 *
 * @details At this point there is no support for map rotation, so the map is always aligned with the screen.
 *
 * Tiled map supports the intermediate zoomming, this means that despite `zlevels` is an integer value (for example 5),
 * the actual zoom steps are not limited to these value. The resolution is calculated based on the
 * `worldVisibleRadius` and `worldVisibleRadiusRange` parameters. In rectangular viewports `worldVisibleRadus` `x` and `y` will differ
 * depending on the map's viewport aspect ratio. Also `worldVisibleRadiusRange` can be set either in config or via `isClampToBorder`
 * flag. The current zoom level `z` is selected as closest to the given resolution. The rendering will shrink the tiles to give a
 * required resolution.
 *
 * Existing flags:
 * - isViewCentered: setting worldPos of map to the view matrix position to follow the character.
 * - isClampToBorder: clamping worldPos and worldVisibleRadius to not go beyond the map edges.
 * - zoomToFit(Map/Border)Edges: the flag is needed for rectangular viewports. If it is active, then at maximum zoom viewport will not
 * go beyond the specified edges. If the flag is disabled, then in one of the planes viewport will go beyond the edges. Below is a
 * table for clarity.
 *
 * | map   | border | result                                                             |
 * |-------|--------|--------------------------------------------------------------------|
 * | false | false  | map will be zoomed to fill borders                                 |
 * | true  | false  | map will be zoomed to fill borders without crossing a map edge     |
 * | true  | true   | map will be zoomed to fit inside the borders without crossing them |
 * | false | true   | map will be zoomed to fit inside the borders without crossing them |
 *
 * The tiles are stored in two hash maps:
 * - tiles: the tiles for the map itself
 * - fogTiles: the tiles for the fog of war
 *
 * Both tilesets trying to keep tiles of the current zoom level. If required tile is not loaded yet, then it requested to load.
 * Load could be done sync or async. In case of async load, the requested tile trying to be substituted by the parent or children
 * tiles. When callback of async load is called, the children tiles is checked if it still required. If not, then it is removed from
 * the hash map.
 *
 * There are several coordinate systems involved in the tiled map rendering:
 * - The world coordinate system is in meters, origin and the axes fully correspond to the scene of the level.
 *   This coord system is left-handed, so from the top it looks like this:
 *    ┌───────────────┐
 *    │    ▲z         │
 *    │    │          │
 *    │    │          │
 *    │    │          │
 *    ├────┼─────────►│
 *    │    │o        x│
 *    │    │          │
 *    └────┴──────────┘
 *   oy in this case is pointing up, into the camera.
 *
 * - The map viewport coordinate system in pixels, the origin is in the center of the darg element, so in
 *   future it would be easier to implement map that rotates with camera.
 *    ┌───────┬───────┐
 *    │       │       │
 *    │       │       │
 *    │       │o      │
 *    ├───────┼──────►│
 *    │       │      x│
 *    │       │       │
 *    │       ▼y      │
 *    └───────────────┘
 *    The `mapToWorld` and `worldToMap` transformations are implemented for two coord systems above.
 *
 * - The screen coordinate system is in pixels, the origin is in the top-left corner. Used for rendering.
 *    ┌───────────────►
 *    │o             x│
 *    │               │
 *    │               │
 *    │               │
 *    │               │
 *    │               │
 *    │y              │
 *    ▼───────────────┘
 *
 * - The integer tile coordinate system is used for tile indexing, the origin is in the top-left corner.
 *   Theese indexes corresponds to the file names in quadkeys. The quadkey have following properties:
 *   1) The length of a quadkey (the number of digits) equals the zoom level of the corresponding tile.
 *   2) The quadkey of any tile starts with the quadkey of its parent tile (the containing tile at the previous level).
 *      e.g. 000 001 002 003 -- all have parent tile on the previous level called 00.
 *   3) its one dimentional index with spartial locality
 *    ┌───┬───┬───┬───┐                           ┌───┬───┬───┬───┐
 *    │0,0│1,0│2,0│...│                           │000│001│010│...│
 *    ├───┼───┼───┼───┤                           ├───┼───┼───┼───┤
 *    │0,1│1,1│2,1│...│                           │002│003│012│...│
 *    ├───┼───┼───┼───┤ <== quadkey transform ==> ├───┼───┼───┼───┤
 *    │0,2│1,2│2,2│...│                           │020│021│030│...│
 *    ├───┼───┼───┼───┤                           ├───┼───┼───┼───┤
 *    │...│...│...│...│                           │...│...│...│...│
 *    └───┴───┴───┴───┘                           └───┴───┴───┴───┘
 *   more info: https://docs.microsoft.com/en-us/bingmaps/articles/bing-maps-tile-system
 *
 * note: there is one special quadkey equal to empty string, it is corresponds to `combined.avif` file. It is used for
 *       conviniance to setup small maps.
 */
class TiledMapContext
{
public:
  TiledMapContext();
  ~TiledMapContext();
  TiledMapContext &operator=(TiledMapContext &&other) = default;

  static void freeAllPictures();

  static QuadKey tileXYToQuadKey(int tileX, int tileY, int zoom);
  static IPoint2 quadKeyToTileXY(const QuadKey &quadKey, int zoom);

  inline int getViewportWidth() const { return viewportWidth; }
  inline int getViewportHeight() const { return viewportHeight; }
  void setViewportSize(int width, int height);

  inline Point2 getVisibleRadiusRange() const { return worldVisibleRadiusRange; }
  inline float getVisibleRadiusWidth() const { return worldVisibleRadius.x; }
  inline float getVisibleRadiusHeight() const { return worldVisibleRadius.y; }
  inline float getVisibleRadius() const { return min(worldVisibleRadius.x, worldVisibleRadius.y); }
  float setVisibleRadius(float r);

  inline Point3 getWorldPos() const { return worldPos; }
  void setWorldPos(const Point3 &pos);
  void setWorldBorder(Point2 leftTop, Point2 rightBottom, bool clampToWorld);
  void resetWorldBorder();

  const TMatrix &getCurViewItm() const { return curViewItm; }

  inline float getTileWorldWidth(int zlvl)
  {
    if (zlvl < 0 || zlvl > zlevels)
      return 0.0;
    return tileWorldWidths[zlvl];
  }
  inline float getTileResolution(int zlvl)
  {
    if (zlvl < 0 || zlvl > zlevels)
      return 0.0;
    return tileResolutions[zlvl];
  }

  inline bool getViewCentered() { return isViewCentered; }
  inline void setViewCentered(bool value) { isViewCentered = value; }

  void setup(Sqrat::Object cfg);
  bool isViewInitialized() const { return viewportResolution > 0; }

  void calcItmFromView(TMatrix &itm) const;
  void calcTmFromView(TMatrix &tm) const;
  Point3 mapToWorld(const Point2 &mapPos) const;
  Point2 worldToMap(const Point3 &pos) const;
  Point2 worldToTc(const Point3 &pos) const;
  Point2 worldToTc(const Point2 &pos) const;

  static TiledMapContext *get_from_element(const darg::Element *elem);

  eastl::string getFogOfWarBase64() const;
  inline void toggleFogOfWar() { fogOfWarEnabled = !fogOfWarEnabled; }

private:
  void dispatchTiles(const eastl::vector<QuadKey> &requiredTiles,
    TilesHashMap &tilesHashMap,
    eastl::string prefix,
    PictureManager::async_load_done_cb_t cb);

  void updateVisibleTiles();

  int calcZoomLevel(float visibleRadius);
  float calcVisibleRadius(int zoom);

  void setWorldBorderInner(Point2 leftTop, Point2 rightBottom, bool clampToWorld);
  void setVisibleRadiusInner(float r);
  void setViewportSizeInner(int width, int height);

  inline void clampToWorldBorder()
  {
    clampVisibleRadiusRangeToWorldBorder();
    setVisibleRadiusInner(getVisibleRadius());
    clampPosToWorldBorder();
  }
  void clampVisibleRadiusRangeToWorldBorder();
  void clampPosToWorldBorder();

public:
  eastl::string tilesPath;
  TilesHashMap tiles;

  bool canLoadTiles = true;

  TMatrix curViewItm = TMatrix::IDENT;
  float perspWk = 0.0;

  // all `world*` members are in meters
  Point2 worldLeftTop = Point2(0, 0);
  Point2 worldRightBottom = Point2(0, 0);
  Point2 worldSize = Point2(0, 0);
  Point2 worldBorderLeftTop = Point2(0, 0);
  Point2 worldBorderRightBottom = Point2(0, 0);
  Point2 worldBorderSize = Point2(0, 0);
  Point3 worldPos = Point3(0, 0, 0);                            // center of the map
  Point3 lastWorldPosAfterVisibleTilesUpdate = Point3(0, 0, 0); // Last world pos after updateVisibleTiles()
  Point2 worldVisibleRadius = Point2(0, 0);                     // Radius of the visible area
  Point2 worldVisibleRadiusRange = Point2(0, 1000);             // (min, max);
  Point2 worldVisibleRadiusRangeDefault = Point2(0, 1000);      // (min, max);
  float northAngle = 0;                                         // [rad]

  int viewportWidth = 0;        // [px]
  int viewportHeight = 0;       // [px]
  float viewportResolution = 0; // [m/px]
  float viewportRatioLargerToSmaller = 1.0;

  int zlevels = 0;                      // total zoom levels
  int z = 0;                            // current zoom level range[0..zlevels]
  int tileWidth;                        // [px] tile original width
  eastl::vector<float> tileWorldWidths; // [m] tile width in world (Depends on zoom level. To get the required value use
                                        // `zoom level-1` index. Because the array starts from `0` and the initial zoom level is `1`)
  eastl::vector<float> tileResolutions; // [m/px] tile resolution on screen (Depends on zoom level. To get the required value use
                                        // `zoom level-1` index. Because the array starts from `0` and the initial zoom level is `1`)

  bool isViewCentered = false;
  bool isClampToBorder = false;
  bool zoomToFitMapEdges = false;
  bool zoomToFitBorderEdges = false;

  TilesHashMap fogTiles;
  UniqueTex fogOfWarTex;
  d3d::SamplerHandle fogOfWarSampler;
  UniqueBuf fogOfWarBitsetSb;
  PostFxRenderer fogOfWarShader = PostFxRenderer("fog_of_war");
  bool fogOfWarEnabled = false;
  float fogOfWarResolution = 0.0f;
  Point2 fogOfWarLeftTop = Point2(0, 0);
  Point2 fogOfWarRightBottom = Point2(0, 0);

  int tileLoadGeneration = 0; // incremented on freeAllPictures to invalidate in-flight async requests

  // track the previous data to avoid unnecessary updates in the fog of war
  int fogOfWarDataGen = 0;
  int fogOfWarPrevDataGen = -1;
  float fogOfWarPrevVisibleRadius = -1.0f;
  Point3 fogOfWarPrevWorldPos = Point3(-1.0f, -1.0f, -1.0f);
};

void tiled_map_on_render_ui(const RenderEventUI &evt);
void tiled_map_fog_of_war_update_data(
  const UpdateStageInfoBeforeRender &evt, const ecs::IntList &fog_of_war__data, const int fog_of_war__dataGen);
void tiled_map_fog_of_war_before_render(const UpdateStageInfoBeforeRender &evt);
void tiled_map_fog_of_war_after_reset();

eastl::vector<uint32_t> tiled_map_fog_of_war_get_data();
void tiled_map_fog_of_war_set_data(const eastl::vector<uint32_t> &data);
