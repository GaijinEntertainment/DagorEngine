#define MAX_NUM_CONTROL_POINTS 1024

struct PhotometryControlPoint
{
  float2 coefficients; // light_intensity = coefficients.x + theta * coefficients.y
  float theta; // [0, pi]
  float lightIntensity; // [0, inf)
};