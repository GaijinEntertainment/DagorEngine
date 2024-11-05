// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <image/dag_texPixel.h>
#include <image/tinyexr-1.0.8/tinyexr.h>
#include <image/dag_exr.h>
#include <debug/dag_assert.h>
#include <EASTL/fixed_vector.h>
#include <generic/dag_enumerate.h>
#include <util/dag_globDef.h>

bool save_exr(const char *filename, uint8_t **pixels, int width, int height, int planes, int stride, const char **plane_names,
  const char *comments)
{
  constexpr int MAX_PLANES = 3;
  G_ASSERT(planes <= MAX_PLANES);

  EXRHeader header;
  InitEXRHeader(&header);

  EXRImage image;
  InitEXRImage(&image);

  image.num_channels = planes;
  image.images = pixels;
  image.width = width;
  image.height = height;

  eastl::fixed_vector<EXRChannelInfo, MAX_PLANES> channel_infos(planes);
  for (auto [i, ci] : enumerate(channel_infos))
    strcpy(ci.name, plane_names[i]);

  header.num_channels = channel_infos.size();
  header.channels = channel_infos.data();

  eastl::fixed_vector<int, MAX_PLANES> pixel_types(planes, TINYEXR_PIXELTYPE_HALF);
  header.pixel_types = header.requested_pixel_types = pixel_types.data();

  header.compression_type = TINYEXR_COMPRESSIONTYPE_PIZ;

  // const of `comments` is cast away, however implementation never modifies the string
  // in EXIF length is described in 2-bytes, resulting in a limit of 2^16, the same limit is used here as well
  EXRAttribute commentAttribute{
    "comments", "string", (uint8_t *)comments, static_cast<int>(strnlen(comments, eastl::numeric_limits<uint16_t>::max()))};
  header.custom_attributes = &commentAttribute;
  header.num_custom_attributes = 1;

  const char *error = nullptr;
  int retVal = SaveEXRImageToFile(&image, &header, filename, &error);
  if (retVal != TINYEXR_SUCCESS)
  {
    logerr("Failed to save EXR image to %s, error: %s", filename, error);
    FreeEXRErrorMessage(error);
    return false;
  }

  return true;
}