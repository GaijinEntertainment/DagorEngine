// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gui/dag_imguiUtil.h>
#include <util/dag_string.h>

// ImGui::InputText...() with eastl::string
// Because text input needs dynamic resizing, we need to setup a callback to grow the capacity
// Source: https://github.com/ocornut/imgui/blob/master/misc/cpp/imgui_stdlib.cpp

struct InputTextCallback_UserData
{
  eastl::string *Str;
  ImGuiInputTextCallback ChainCallback;
  void *ChainCallbackUserData;
};

struct InputTextCallback_UserDataString
{
  String *Str;
  ImGuiInputTextCallback ChainCallback;
  void *ChainCallbackUserData;
};

static int InputTextCallback(ImGuiInputTextCallbackData *data)
{
  InputTextCallback_UserData *user_data = (InputTextCallback_UserData *)data->UserData;
  if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
  {
    // Resize string callback
    // If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
    eastl::string *str = user_data->Str;
    IM_ASSERT(data->Buf == str->c_str());
    str->resize(data->BufTextLen);
    data->Buf = (char *)str->c_str();
  }
  else if (user_data->ChainCallback)
  {
    // Forward to user callback, if any
    data->UserData = user_data->ChainCallbackUserData;
    return user_data->ChainCallback(data);
  }
  return 0;
}

static int InputTextCallbackString(ImGuiInputTextCallbackData *data)
{
  InputTextCallback_UserDataString *user_data = (InputTextCallback_UserDataString *)data->UserData;
  if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
  {
    // Resize string callback
    // If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
    String *str = user_data->Str;
    IM_ASSERT(data->Buf == str->c_str());
    str->resize(data->BufTextLen + 1);
    data->Buf = (char *)str->c_str();
  }
  else if (user_data->ChainCallback)
  {
    // Forward to user callback, if any
    data->UserData = user_data->ChainCallbackUserData;
    return user_data->ChainCallback(data);
  }
  return 0;
}

bool ImGuiDagor::InputText(const char *label, eastl::string *str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback,
  void *user_data)
{
  IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
  flags |= ImGuiInputTextFlags_CallbackResize;

  InputTextCallback_UserData cb_user_data;
  cb_user_data.Str = str;
  cb_user_data.ChainCallback = callback;
  cb_user_data.ChainCallbackUserData = user_data;
  return ImGui::InputText(label, (char *)str->c_str(), str->capacity() + 1, flags, InputTextCallback, &cb_user_data);
}

bool ImGuiDagor::InputText(const char *label, String *str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void *user_data)
{
  IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
  flags |= ImGuiInputTextFlags_CallbackResize;

  InputTextCallback_UserDataString cb_user_data;
  cb_user_data.Str = str;
  cb_user_data.ChainCallback = callback;
  cb_user_data.ChainCallbackUserData = user_data;
  return ImGui::InputText(label, str->str(), str->capacity(), flags, InputTextCallbackString, &cb_user_data);
}

bool ImGuiDagor::InputTextMultiline(const char *label, eastl::string *str, const ImVec2 &size, ImGuiInputTextFlags flags,
  ImGuiInputTextCallback callback, void *user_data)
{
  IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
  flags |= ImGuiInputTextFlags_CallbackResize;

  InputTextCallback_UserData cb_user_data;
  cb_user_data.Str = str;
  cb_user_data.ChainCallback = callback;
  cb_user_data.ChainCallbackUserData = user_data;
  return ImGui::InputTextMultiline(label, (char *)str->c_str(), str->capacity() + 1, size, flags, InputTextCallback, &cb_user_data);
}

bool ImGuiDagor::InputTextMultiline(const char *label, String *str, const ImVec2 &size, ImGuiInputTextFlags flags,
  ImGuiInputTextCallback callback, void *user_data)
{
  IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
  flags |= ImGuiInputTextFlags_CallbackResize;

  InputTextCallback_UserDataString cb_user_data;
  cb_user_data.Str = str;
  cb_user_data.ChainCallback = callback;
  cb_user_data.ChainCallbackUserData = user_data;
  return ImGui::InputTextMultiline(label, str->str(), str->capacity(), size, flags, InputTextCallbackString, &cb_user_data);
}

bool ImGuiDagor::InputTextWithHint(const char *label, const char *hint, eastl::string *str, ImGuiInputTextFlags flags,
  ImGuiInputTextCallback callback, void *user_data)
{
  IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
  flags |= ImGuiInputTextFlags_CallbackResize;

  InputTextCallback_UserData cb_user_data;
  cb_user_data.Str = str;
  cb_user_data.ChainCallback = callback;
  cb_user_data.ChainCallbackUserData = user_data;
  return ImGui::InputTextWithHint(label, hint, (char *)str->c_str(), str->capacity() + 1, flags, InputTextCallback, &cb_user_data);
}

bool ImGuiDagor::InputTextWithHint(const char *label, const char *hint, String *str, ImGuiInputTextFlags flags,
  ImGuiInputTextCallback callback, void *user_data)
{
  IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
  flags |= ImGuiInputTextFlags_CallbackResize;

  InputTextCallback_UserDataString cb_user_data;
  cb_user_data.Str = str;
  cb_user_data.ChainCallback = callback;
  cb_user_data.ChainCallbackUserData = user_data;
  return ImGui::InputTextWithHint(label, hint, str->str(), str->capacity(), flags, InputTextCallbackString, &cb_user_data);
}
