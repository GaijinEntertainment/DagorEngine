#pragma once

#include <daRg/dag_guiScene.h>
#include "textLayout.h"
#include <EASTL/string.h>

namespace StdGuiRender
{
class GuiContext;
}


namespace darg
{

class SceneStatus
{
public:
  void reset();
  void appendErrorText(const char *msg);
  void dismissShownError();
  void toggleShowDetails();
  void setSceneErrorRenderMode(SceneErrorRenderMode mode);
  void renderError(StdGuiRender::GuiContext *ctx);

public:
  bool sceneIsBroken = false;
  bool renderErrorFlag = false;
  int errorCount = 0;
  int errorBeginRenderTimeMsec = 0;

  textlayout::FormattedText errorMsg;
  eastl::string shortErrorString;

  SceneErrorRenderMode errorRenderMode = SEI_DEFAULT;
  SceneErrorRenderMode minimalErrorRenderMode = SEI_DEFAULT;
};


} // namespace darg
