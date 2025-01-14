struct PuddleInfo
{
  float3 pos;
  uint starttime__strength;
  uint2 normal_fadetime;
  uint matrix_frame_incident_type;
  uint rotate_size;
};

static const int SECONDS_TO_FADE = 4;
static const int BITS_FOR_TYPE = 3;
static const int BITS_FOR_INCIDENT_ANGLE = 3;
static const float FADE_TIME_TICK = 0.1f;
static const float FADE_TIME_TICK_INV = 1 / FADE_TIME_TICK;
static const int MATRIX_MASK = 0x3FFF; //bit No 30 is unused
static const int LANDSCAPE_SHIFT = 31;