#include "av_util.h"

#include <libTools/staticGeom/geomObject.h>
#include <libTools/util/strUtil.h>

#define SUN_DIRECTION_ZENITH  (PI / 4.0)
#define SUN_DIRECTION_AZIMUTH PI
#define SUN_COLOR             E3DCOLOR(255, 255, 220, 255)
#define SKY_COLOR             E3DCOLOR(160, 180, 220, 255)
#define EARTH_COLOR           E3DCOLOR(45, 45, 55, 255)


void init_geom_object_lighting()
{
  GeomObject::setLightAngles(SUN_DIRECTION_ZENITH, SUN_DIRECTION_AZIMUTH);
  GeomObject::setAmbientLight(SKY_COLOR, 1, EARTH_COLOR, 1);
  GeomObject::setDirectLight(SUN_COLOR, 1);
}
