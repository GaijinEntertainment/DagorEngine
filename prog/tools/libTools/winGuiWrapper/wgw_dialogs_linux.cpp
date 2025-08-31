// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_debug.h>
#include <libTools/util/daKernel.h>
#include <libTools/util/strUtil.h>
#include <osApiWrappers/dag_messageBox.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_threads.h>
#include <util/dag_string.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <workCycle/dag_workCycle.h>

#include <EASTL/atomic.h>

#include <stdio.h>

namespace wingw
{

static const char *NATIVE_MODAL_DIALOG_EVENTS_VAR_NAME = "wingw::dialog_events";

void set_native_modal_dialog_events(INativeModalDialogEventHandler *events)
{
  dakernel::set_named_pointer(NATIVE_MODAL_DIALOG_EVENTS_VAR_NAME, events);
}

static INativeModalDialogEventHandler *get_native_modal_dialog_events()
{
  return (INativeModalDialogEventHandler *)dakernel::get_named_pointer(NATIVE_MODAL_DIALOG_EVENTS_VAR_NAME);
}

static int message_box_internal(int flags, const char *caption, const char *text)
{
  int osFlags = 0;
  switch (flags & 15)
  {
    case MBS_OKCANCEL: osFlags = GUI_MB_OK_CANCEL; break;
    case MBS_YESNOCANCEL: osFlags = GUI_MB_YES_NO_CANCEL; break;
    case MBS_YESNO: osFlags = GUI_MB_YES_NO; break;
    case MBS_ABORTRETRYIGNORE: osFlags = GUI_MB_ABORT_RETRY_IGNORE; break;

    case MBS_OK:
    default: osFlags = GUI_MB_OK; break;
  }

  switch (flags & (15 << 4))
  {
    case MBS_HAND: osFlags |= GUI_MB_ICON_WARNING; break;
    case MBS_QUEST: osFlags |= GUI_MB_ICON_QUESTION; break;
    case MBS_EXCL: osFlags |= GUI_MB_ICON_ERROR; break;

    case MBS_INFO:
    default: osFlags |= GUI_MB_ICON_INFORMATION; break;
  }

  const int result = os_message_box(text, caption, osFlags);
  switch (flags & 15)
  {
    case MBS_YESNOCANCEL:
      if (result == GUI_MB_BUTTON_1)
        return MB_ID_YES;
      return result == GUI_MB_BUTTON_2 ? MB_ID_NO : MB_ID_CANCEL;

    case MBS_ABORTRETRYIGNORE:
      if (result == GUI_MB_BUTTON_1)
        return MB_ID_ABORT;
      return result == GUI_MB_BUTTON_2 ? MB_ID_RETRY : MB_ID_IGNORE;

    case MBS_YESNO: return result == GUI_MB_BUTTON_1 ? MB_ID_YES : MB_ID_NO;
    case MBS_OKCANCEL: return result == GUI_MB_BUTTON_1 ? MB_ID_OK : MB_ID_CANCEL;
    case MBS_OK: return MB_ID_OK;
    default: return MB_ID_CANCEL;
  }
}

int message_box(int flags, const char *caption, const char *text, const DagorSafeArg *arg, int anum)
{
  if (INativeModalDialogEventHandler *eventHandler = get_native_modal_dialog_events())
    eventHandler->beforeModalDialogShown();

  String formattedText;
  formattedText.vprintf(2048, text, arg, anum);
  const int result = message_box_internal(flags, caption, formattedText);

  if (INativeModalDialogEventHandler *eventHandler = get_native_modal_dialog_events())
    eventHandler->afterModalDialogShown();

  return result;
}

static String run_zenity_internal(dag::ConstSpan<String> arguments)
{
  if (system("command -v zenity > /dev/null") != 0)
  {
    message_box(GUI_MB_OK, "Error", "zenity cannot be found. Please install it.\n\nIt is required for the file selector dialogs.");
    logerr("zenity cannot be found.");
    return String();
  }

  String command("zenity");
  for (const String &argument : arguments)
  {
    command += ' ';

    // Put the argument into single quotes. Escape single quotes with closing the quote, adding backslash escaped quote,
    // and re-opening the quote.
    command += "'";
    command += String(argument).replaceAll("'", "'\\''");
    command += "'";
  }

  FILE *pipe = popen(command, "r");
  if (!pipe)
  {
    logerr("Failed to run command: %s", command.c_str());
    return String();
  }

  char buffer[1024];
  String output;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    output += buffer;

  const int result = pclose(pipe);
  if (WEXITSTATUS(result) != 0)
    return String();

  if (output.length() > 0 && output[output.length() - 1] == '\n')
    output.chop(1);

  trim(output);
  return output;
}

// Run zenity in a thread to prevent the main window to appear frozen to Mutter.
// Fl_Native_File_Chooser crashed..., so using zenity instead.
// TODO: tools Linux porting: use XDG Desktop Portal's FileChooser instead.
static String run_zenity(dag::ConstSpan<String> arguments)
{
  eastl::atomic<bool> finished = false;
  String result;

  execute_in_new_thread(
    [&](auto) {
      result = run_zenity_internal(arguments);
      finished = true;
    },
    "zenity runner");

  while (!finished)
  {
    dagor_idle_cycle(false);
    sleep_msec(50);
  }

  return result;
}

struct FileFilter
{
  String title;
  String extension;
};

// Example filter: "Squirrel files (*.nut)|*.nut|All files (*.*)|*.*"
// The returned extension is guaranteed to be non-empty.
static dag::Vector<FileFilter> split_extension_filters(const char *filter)
{
  dag::Vector<FileFilter> fileFilters;

  while (filter)
  {
    const char *filterSeparator = strchr(filter, '|');
    if (!filterSeparator)
      break;

    FileFilter fileFilter;
    fileFilter.title.append(filter, filterSeparator - filter);

    filter = filterSeparator + 1;
    filterSeparator = strchr(filter, '|');
    if (filterSeparator)
    {
      fileFilter.extension.append(filter, filterSeparator - filter);
      filter = filterSeparator + 1;
    }
    else
    {
      fileFilter.extension = filter;
      filter = nullptr;
    }

    trim(fileFilter.extension);
    if (!fileFilter.extension.empty())
      fileFilters.push_back(fileFilter);
  }

  return fileFilters;
}

static String file_selector_dialog(void *hwnd, const char caption[], const char filter[], const char def_ext[], const char init_path[],
  const char init_fn[], bool save_dialog)
{
  dag::Vector<String> arguments;
  arguments.emplace_back("--file-selection");

  String captionString(caption);
  trim(captionString);
  if (!captionString.empty())
  {
    arguments.emplace_back("--title");
    arguments.emplace_back(captionString);
  }

  String initialPathAndName(init_path);
  trim(initialPathAndName);

  String initialName(init_fn);
  trim(initialName);
  if (!initialPathAndName.empty() && !initialName.empty())
    initialPathAndName += "/";
  initialPathAndName += initialName;

  if (!initialPathAndName.empty())
  {
    arguments.emplace_back("--filename");
    arguments.emplace_back(initialPathAndName);
  }

  const dag::Vector<FileFilter> fileFilters = split_extension_filters(filter);
  for (const FileFilter &fileFilter : fileFilters)
  {
    const char *filterTitle = fileFilter.title.empty() ? fileFilter.extension : fileFilter.title;
    arguments.emplace_back(0, "--file-filter=%s|%s", filterTitle, fileFilter.extension);
  }

  if (save_dialog)
  {
    arguments.emplace_back("--save");
    arguments.emplace_back("--confirm-overwrite");
  }

  return run_zenity(arguments);
}

String file_open_dlg(void *hwnd, const char caption[], const char filter[], const char def_ext[], const char init_path[],
  const char init_fn[])
{
  if (INativeModalDialogEventHandler *eventHandler = get_native_modal_dialog_events())
    eventHandler->beforeModalDialogShown();

  const String result = file_selector_dialog(hwnd, caption, filter, def_ext, init_path, init_fn, /*save_dialog = */ false);

  if (INativeModalDialogEventHandler *eventHandler = get_native_modal_dialog_events())
    eventHandler->afterModalDialogShown();

  return result;
}

String file_save_dlg(void *hwnd, const char caption[], const char filter[], const char def_ext[], const char init_path[],
  const char init_fn[])
{
  if (INativeModalDialogEventHandler *eventHandler = get_native_modal_dialog_events())
    eventHandler->beforeModalDialogShown();

  const String result = file_selector_dialog(hwnd, caption, filter, def_ext, init_path, init_fn, /*save_dialog = */ true);

  if (INativeModalDialogEventHandler *eventHandler = get_native_modal_dialog_events())
    eventHandler->afterModalDialogShown();

  return result;
}

static String dir_select_dlg_internal(void *hwnd, const char caption[], const char init_path[])
{
  dag::Vector<String> arguments;
  arguments.emplace_back("--file-selection");
  arguments.emplace_back("--directory");

  String captionString(caption);
  trim(captionString);
  if (!captionString.empty())
  {
    arguments.emplace_back("--title");
    arguments.emplace_back(captionString);
  }

  String initialPath(init_path);
  trim(initialPath);
  if (!initialPath.empty())
  {
    arguments.emplace_back("--filename");
    arguments.emplace_back(init_path);
  }

  return run_zenity(arguments);
}

String dir_select_dlg(void *hwnd, const char caption[], const char init_path[])
{
  if (INativeModalDialogEventHandler *eventHandler = get_native_modal_dialog_events())
    eventHandler->beforeModalDialogShown();

  const String result = dir_select_dlg_internal(hwnd, caption, init_path);

  if (INativeModalDialogEventHandler *eventHandler = get_native_modal_dialog_events())
    eventHandler->afterModalDialogShown();

  return result;
}

} // namespace wingw