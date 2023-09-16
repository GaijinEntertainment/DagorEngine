#include <EASTL/vector_map.h>

namespace dafx_ex
{
enum
{
  RTAG_LOWRES = 0,
  RTAG_HIGHRES = 1,
  RTAG_DISTORTION = 2,
  RTAG_TRANSMITTANCE = 3,
  RTAG_THERMAL = 4,
  RTAG_WATER_PROJ = 5,
  RTAG_VOL_THICKNESS = 6,
  RTAG_VOL_DEPTH = 7,
  RTAG_VOL_WBOIT = 8,
  RTAG_FOM = 9,
  RTAG_UNDERWATER = 10,
};

static const char *renderTags[] = {"lowres", "highres", "distortion", "transmittance", "thermal", "water_proj", "vol_thickness",
  "vol_depth", "vol_wboit", "fom", "underwater"};

enum TransformType
{
  TRANSFORM_DEFAULT = 0,
  TRANSFORM_WORLD_SPACE = 1,
  TRANSFORM_LOCAL_SPACE = 2,
};

struct SystemInfo
{
  int rflags = 0;
  int sflags = 0;
  int maxInstances = 0;
  int playerReserved = 0;
  float spawnRangeLimit = 0;
  float onePointNumber = 0;
  float onePointRadius = 0;
  TransformType transformType = TRANSFORM_DEFAULT;

  enum ValueType
  {
    VAL_FLAGS = 0,
    VAL_TM,
    VAL_RADIUS_MIN,
    VAL_RADIUS_MAX,
    VAL_COLOR_MIN,
    VAL_COLOR_MAX,
    VAL_COLOR_MUL,
    VAL_ROTATION_MIN,
    VAL_ROTATION_MAX,
    VAL_ROTATION_VEL_MIN,
    VAL_ROTATION_VEL_MAX,
    VAL_MASS,
    VAL_DRAG_COEFF,
    VAL_DRAG_TO_RAD,
    VAL_LOCAL_GRAVITY_VEC,
    VAL_VELOCITY_START_MIN,
    VAL_VELOCITY_START_MAX,
    VAL_VELOCITY_START_VEC3,
    VAL_VELOCITY_START_ADD,
    VAL_VELOCITY_ADD_MIN,
    VAL_VELOCITY_ADD_MAX,
    VAL_VELOCITY_ADD_VEC3,
    VAL_LIGHT_POS,
    VAL_LIGHT_COLOR,
    VAL_LIGHT_RADIUS,
  };
  eastl::vector_map<int, int> valueOffsets;
};
} // namespace dafx_ex