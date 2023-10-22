#include <cstdint>

#include <render/emissionColorMaps.h>

#include <3d/dag_drv3d.h>
#include <3d/dag_lockSbuffer.h>
#include <math/dag_Point4.h>
#include <math/dag_Point3.h>

static const uint32_t EMISSION_COLOR_MAP_POINTS = 256;
static const uint32_t EMISSION_COLOR_HUE_BITS = 6;
static const uint32_t EMISSION_COLOR_REPLACE_ENC = (1 << EMISSION_COLOR_HUE_BITS) - 1;

EmissionColorMaps::EmissionColorMaps()
{
  decodeMap = dag::buffers::create_persistent_sr_structured(sizeof(Point4), EMISSION_COLOR_MAP_POINTS, "emission_decode_color_map");
  decodeMap.setVar();
  upload();
}

void EmissionColorMaps::onReset() { upload(); }

static inline Point4 hsv_to_rgb(const Point3 &c)
{
  Point3 K(1.0f, 2.0f / 3.0f, 1.0f / 3.0f);
  Point3 p = abs(frac(K + c.x) * 6.0f - 3.0f);
  float lerpFactor = clamp(c.y, 0.0f, 1.0f);
  return Point4::xyz0((clamp(p - K.x, 0.0f, 1.0f) * lerpFactor + K.x * (1.0f - lerpFactor)) * c.z);
}

void EmissionColorMaps::upload()
{
  if (auto bufData = lock_sbuffer<Point4>(decodeMap.getBuf(), 0, EMISSION_COLOR_MAP_POINTS, VBLOCK_WRITEONLY))
  {
    for (uint32_t i = 0; i < EMISSION_COLOR_MAP_POINTS; ++i)
    {
      bufData[i] = Point4(1, 1, 1, 0);
      if (i == EMISSION_COLOR_REPLACE_ENC)
        continue;
      float saturation = ((float)(i >> EMISSION_COLOR_HUE_BITS) + 1) * 0.25;
      float hue = (float)(i & EMISSION_COLOR_REPLACE_ENC) / EMISSION_COLOR_REPLACE_ENC;
      bufData[i] = hsv_to_rgb(Point3(hue, saturation, 1.0f));
    }
  }
}
