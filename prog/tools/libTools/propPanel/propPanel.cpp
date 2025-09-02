// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <propPanel/propPanel.h>
#include <propPanel/imguiHelper.h>
#include "imageHelper.h"
#include "messageQueueInternal.h"
#include "tooltipHelper.h"

namespace PropPanel
{

void release()
{
  image_helper.release();
  message_queue.release();
}

void after_new_frame()
{
  ImguiHelper::afterNewFrame();
  tooltip_helper.afterNewFrame();
}

void before_end_frame() { tooltip_helper.beforeEndFrame(); }

IconId load_icon(const char *filename) { return image_helper.loadIcon(filename); }

ImTextureID get_im_texture_id_from_icon_id(IconId icon_id) { return image_helper.getImTextureIdFromIconId(icon_id); }

void reload_all_icons() { image_helper.reloadAllIcons(); }

void set_previous_imgui_control_tooltip(const void *control, const char *text, const char *text_end)
{
  tooltip_helper.setPreviousImguiControlTooltip(control, text, text_end);
}

} // namespace PropPanel