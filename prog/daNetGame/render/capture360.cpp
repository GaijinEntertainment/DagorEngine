// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "capture360.h"

#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_commands.h>
#include <math/integer/dag_IPoint2.h>
#include <osApiWrappers/dag_miscApi.h>
#include <ecs/camera/getActiveCameraSetup.h>
#include <util/dag_string.h>
#include <util/dag_console.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_assert.h>
#include <shaders/dag_shaderVar.h>
#include "camera/sceneCam.h"
#include "video360/video360.h"
#include "textureUtil/textureUtil.h"
#include "renderer.h"
#include "screencap.h"

static const int face_indices[] = {4, 5, 2, 3, 1, 0};
static const char *face_names[] = {"face_1_+X", "face_2_-X", "face_3_+Y", "face_4_-Y", "face_5_+Z", "face_6_-Z"};

static int get_360_cube_face_index(int faceId)
{
  if (faceId < 0 || faceId >= countof(face_indices))
    return -1;
  return face_indices[faceId];
}

static const char *get_360_cube_face_filename(int index)
{
  if (index < 0 || index >= countof(face_names))
    return "";
  return face_names[index];
}

static void saveAll()
{
  logdbg("Saving 360 capture screenshots...");

  int cubemapSize = screencap::get_video360()->getCubeSize();

  DagorDateTime time;
  ::get_local_time(&time);
  String dir(256, "shot360_%04d.%02d.%02d_%02d.%02d.%02d", time.year, time.month, time.day, time.hour, time.minute, time.second);

  dag::Span<UniqueTex> sourceFaces = screencap::get_video360()->getFaces();
  UniqueTex faces[6];

  // Render scene to cube faces
  for (int faceId = 0; faceId < 6; faceId++)
  {
    TEXTUREID targetFrameId = sourceFaces[faceId].getTexId();
    // Copy cube face result to export later
    int cubeFaceIndex = get_360_cube_face_index(faceId);
    String texName(256, "cubeface360_%d", cubeFaceIndex);
    faces[cubeFaceIndex] = dag::create_tex(nullptr, cubemapSize, cubemapSize, TEXCF_SRGBREAD | TEXCF_RTARGET, 1, texName);
    if (cubeFaceIndex == 2)
      texture_util::rotate_texture(targetFrameId, faces[cubeFaceIndex].getBaseTex(), texture_util::TextureRotation::_90_CW);
    else if (cubeFaceIndex == 3)
      texture_util::rotate_texture(targetFrameId, faces[cubeFaceIndex].getBaseTex(), texture_util::TextureRotation::_90_CCW);
    else
      texture_util::rotate_texture(targetFrameId, faces[cubeFaceIndex].getBaseTex(), texture_util::TextureRotation::_0);
  }

  // Export separate images non-rotated
  for (int i = 0; i < 6; i++)
  {
    String cubeFaceFileNameOverride(256, "%s/%s", dir, get_360_cube_face_filename(i));
    screencap::make_screenshot(faces[i], cubeFaceFileNameOverride);
  }


  if (cubemapSize * 6 <= 16384)
  {
    // Export cube faces stitched together as a single cubemap image
    {
      Tab<BaseTexture *> faceTextures;
      for (int i = 0; i < 6; i++)
        faceTextures.push_back(faces[i].getBaseTex());
      UniqueTex stitchedTex =
        dag::create_tex(nullptr, cubemapSize * 6, cubemapSize, TEXCF_SRGBREAD | TEXCF_RTARGET, 1, "stitched_horizontal");
      texture_util::stitch_textures_horizontal(faceTextures, stitchedTex.getTex2D());
      String cubeFileNameOverride(256, "%s/%s", dir, "cube");
      screencap::make_screenshot(stitchedTex, cubeFileNameOverride);
    }

    // Export cube faces stitched together as a single cubemap image, WT-style, so AV accepts it
    {
      Tab<BaseTexture *> faceTextures;
      for (int i = 0; i < 6; i++)
        faceTextures.push_back(faces[i].getBaseTex());
      UniqueTex stitchedTex =
        dag::create_tex(nullptr, cubemapSize, cubemapSize * 6, TEXCF_SRGBREAD | TEXCF_RTARGET, 1, "stitched_vertical");
      texture_util::stitch_textures_vertical(faceTextures, stitchedTex.getTex2D());
      String cubeFileNameOverride(256, "%s/%s", dir, "cubemap"); // AV needs *cubemap as name
      screencap::make_screenshot(stitchedTex, cubeFileNameOverride, true);
    }
  }
  else
    console::print_d(
      "Requested cubemap size %d is too big to make the stitched textures! Can't allocate larger than 16384x16384 texture.",
      cubemapSize);

  // Export panorama image
  {
    SCOPE_RENDER_TARGET;
    UniqueTex panorama =
      dag::create_tex(nullptr, cubemapSize * 2, cubemapSize, TEXCF_SRGBREAD | TEXCF_RTARGET, 1, "screenshot360_panoramic");
    d3d::set_render_target(panorama.getTex2D(), 0);

    screencap::get_video360()->renderSphericalProjection();

    String panoramicFileNameOverride(256, "%s/%s", dir, "panoramic");
    screencap::make_screenshot(panorama, panoramicFileNameOverride);
  }

  const char *screenshotsDir = ::dgs_get_settings()->getBlockByNameEx("screenshots")->getStr("dir", "Screenshots");
  console::print_d("360 capture saved: %s/%s", screenshotsDir, dir);
}

bool capture360::is_360_capturing_in_progress() { return screencap::get_video360(); }

void capture360::update_capture(ManagedTexView tex)
{
  screencap::get_video360()->update(tex);
  if (screencap::get_video360()->isFinished())
  {
    saveAll();
    screencap::cleanup_video360();
  }
}
