#include <render/debugTonemapOverlay.h>
#include <3d/dag_drv3d.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_overrideStates.h>
#include <EASTL/string.h>
#include <util/dag_console.h>

static const char explanation_text[] = "render.show_tonemap r g b [maxIntensity]\n"
                                       "The input color is scaled from 0 to maxIntensity on the x axis, then it's tonemapped.\n"
                                       "The resulting color is compared to the y axis linear scaling of the input color.\n"
                                       "The brigther the graph is, the closer these two colors are.\n"
                                       "The color represents the absolute change in components relative to each other.\n"
                                       "For example, if it's red, the R component changed more than G and B.\n"
                                       "maxIntensity is optional, by default it's 2, to account for >1 colors.\n";

static int renderSizeVarId = -1;
static int tonemapColorVarId = -1;
static int maxIntensityVarId = -1;

DebugTonemapOverlay::DebugTonemapOverlay() : targetSize(1024, 768), tonemapColor(0.0f, 0.0f, 0.0f)
{
  renderer.init("debug_tonemap_overlay", NULL, false);
  renderSizeVarId = get_shader_variable_id("tonemap_render_size", true);
  tonemapColorVarId = get_shader_variable_id("tonemap_debug_color", true);
  maxIntensityVarId = get_shader_variable_id("tonemap_max_intensity", true);
  shaders::OverrideState state;
  state.set(shaders::OverrideState::SCISSOR_ENABLED);
  scissor = shaders::overrides::create(state);
}

DebugTonemapOverlay::~DebugTonemapOverlay() {}

String DebugTonemapOverlay::processConsoleCmd(const char *argv[], int argc)
{
  if (!renderer.getMat())
    return String("'debug_tonemap_overlay' shader is unavailable");

  if (argc < 4)
  {
    bool showExplanation = !(isEnabled && argc == 1);
    isEnabled = false;
    return showExplanation ? String(explanation_text) : String("render.show_tonemap is off");
  }
  else
  {
    isEnabled = true;
    float r = atof(argv[1]);
    float g = atof(argv[2]);
    float b = atof(argv[3]);
    tonemapColor = Color4(r, g, b);
    if (argc == 5)
    {
      float mi = atof(argv[4]);
      if (mi > 0.0f)
        maxIntensity = mi;
    }
    return String(0, "tonemap debug color (%f, %f, %f) with max intensity %f", r, g, b, maxIntensity);
  }
}

void DebugTonemapOverlay::setTargetSize(const Point2 &target_size) { targetSize = target_size; }

void DebugTonemapOverlay::render()
{
  if (!isEnabled)
    return;

  if (renderSizeVarId == -1 || tonemapColorVarId == -1 || maxIntensityVarId == -1)
  {
    console::print_d("show_tonemap unavailable, shader vars not found");
    isEnabled = false;
    return;
  }

  Point2 s = Point2(GRAPH_SIZE / targetSize.x, GRAPH_SIZE / targetSize.y);
  ShaderGlobal::set_color4(renderSizeVarId, Color4(s.x, s.y, s.x - 1.f, 1.f - s.y));
  ShaderGlobal::set_color4(tonemapColorVarId, tonemapColor);
  ShaderGlobal::set_real(maxIntensityVarId, maxIntensity);

  shaders::overrides::set(scissor);
  d3d::setscissor(0, 0, GRAPH_SIZE, GRAPH_SIZE);
  renderer.render();
  shaders::overrides::reset();
}