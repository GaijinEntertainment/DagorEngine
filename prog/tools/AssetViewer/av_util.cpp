#include "av_util.h"

#include <libTools/staticGeom/geomObject.h>
#include <libTools/util/strUtil.h>

#include <math/dag_mathAng.h>


#define SUN_DIRECTION_ZENITH  (PI / 4.0)
#define SUN_DIRECTION_AZIMUTH PI
#define SUN_COLOR             E3DCOLOR(255, 255, 220, 255)
#define SUN_BRIGHTNESS        2500.0
#define SKY_COLOR             E3DCOLOR(160, 180, 220, 255)
#define EARTH_COLOR           E3DCOLOR(45, 45, 55, 255)


SH3Lighting olSh3Light;


void init_geom_object_lighting()
{
  olSh3Light.clear();

  GeomObject::setLightAngles(SUN_DIRECTION_ZENITH, SUN_DIRECTION_AZIMUTH);
  GeomObject::setAmbientLight(SKY_COLOR, 1, EARTH_COLOR, 1);
  GeomObject::setDirectLight(SUN_COLOR, 1);

  Point3 dir = ::sph_ang_to_dir(Point2(SUN_DIRECTION_AZIMUTH, SUN_DIRECTION_ZENITH));
  real cosSunHalfAngle = cosf(DegToRad(2.5));

  Color4 lightColor = color4(SUN_COLOR) * SUN_BRIGHTNESS;

  ::add_hemisphere_sphharm(olSh3Light.sh, dir, cosSunHalfAngle, color3(lightColor));

  for (int i = 0; i < SPHHARM_NUM3; ++i)
    olSh3Light.sh[i] /= 8.0;

  ::rotate_sphharm(olSh3Light.sh, olSh3Light.sh, -TMatrix::IDENT);
}
