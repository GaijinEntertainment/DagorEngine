#include <render/genericLUT/colorGradingLUT.h>

ColorGradingLUT::ColorGradingLUT(bool)
{
  lutBuilder.init("graded_color_lut", "render_graded_color_lut", "compute_graded_color_lut", GenericTonemapLUT::HDROutput::HDR, 33);
}