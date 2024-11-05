// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "lang.h"

#include <string.h>


namespace breakpad
{
namespace ui
{

struct Language
{
  const char *id;
  const char *texts[UI_TXT_TOTAL];
}; // struct Language

static int cur_lang_id = 0;
Language languages[] = {
  {"en-US", {": Crash delivery report", "Could not deliver report: ", "Report has been sent successfully.\nCrash ID is ",

              ": Crash report",
              "We are sorry, but something went wrong.\n\nPlease help us track down and fix this crash by sending\nan automatically "
              "generated report.",
              "Include log files with submission. I understand that logs may contain some personal information such as my system and "
              "user name.",
              "I agree to be contacted by email if additional information will help fix this crash.",

              "Send and Restart", "Send and Close", "Close", "Copy Crash ID", "Close and Restart"}},

  {"ru-RU", {": отчёт об ошибке", "Не удалось отправить отчёт: ", "Отчёт отправлен успешно.\nИдентификатор отчёта - ",

              ": отчёт об ошибке",
              "К сожалению, что-то пошло не так.\n\nПожалуйста, помогите нам разобраться в проблеме, отправив\nавтоматически "
              "сгенерированный отчёт.",
              "Отправить журнал выполнения. Я понимаю, что журнал может содержать персональную информацию, например, имя пользователя "
              "или идентификатор системы.",
              "Я согласен, чтобы со мной связались по email, если это поможет исправить проблему.",

              "Отправить и перезапустить", "Отправить и закрыть", "Закрыть", "Копировать идентификатор", "Закрыть и перезапустить"}},

  {"zh-CN", {": 发送错误报告", "无法发送错误报告: ", "成功发送错误报告。\n报告编号为 ",

              ": 错误报告", "非常抱歉，游戏程序崩溃了。\n\n请发送自动生成的错误报告帮助我们定位和解决游戏的错误问题。",
              "错误报告将包含如电脑配置或游戏昵称之类的个人信息，请知悉", "我同意，如果必要会接受官方邮件联系。",

              "发送并重启", "发送并关闭", "关闭", "复制错误报告编号", "关闭并重启"}},
};

void set_language(const char *lang)
{
  size_t numLangs = sizeof(languages) / sizeof(languages[0]);
  for (cur_lang_id = 0; cur_lang_id < numLangs; ++cur_lang_id)
    if (!strcmp(languages[cur_lang_id].id, lang))
      break;

  if (cur_lang_id == numLangs)
    cur_lang_id = 0;
}

const char *lang(TextId id) { return (0 < id && id < UI_TXT_TOTAL) ? languages[cur_lang_id].texts[id] : ""; }

} // namespace ui
} // namespace breakpad
