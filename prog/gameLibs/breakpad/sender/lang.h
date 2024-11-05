// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace breakpad
{
namespace ui
{

enum TextId
{
  UI_TXT_DELIVERY_TITLE = 0,
  UI_TXT_DELIVERY_FAILED,
  UI_TXT_DELIVERY_OK,

  UI_TXT_SUBMISSION_TITLE,
  UI_TXT_SUBMISSION_QUERY,
  UI_TXT_SUBMISSION_ALLOW_LOGS,
  UI_TXT_SUBMISSION_ALLOW_EMAIL,

  UI_TXT_BTN_SEND_AND_RESTART,
  UI_TXT_BTN_SEND_AND_CLOSE,
  UI_TXT_BTN_CLOSE,
  UI_TXT_BTN_COPY_UUID,
  UI_TXT_BTN_RESTART,

  UI_TXT_TOTAL
}; // enum TextId

void set_language(const char *lang);
const char *lang(TextId id);

} // namespace ui
} // namespace breakpad
