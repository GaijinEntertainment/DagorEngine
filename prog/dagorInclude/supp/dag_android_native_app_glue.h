//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#if !NDEBUG
#define ANDROID_APP_GLUE_NO_NDEBUG 1
#endif

#if ANDROID_APP_GLUE_NO_NDEBUG
#define NDEBUG 1
#endif

#include <android_native_app_glue.h>
#include <EASTL/array.h>

namespace android
{
using OnInputCallback = void (*)(const AInputEvent *);

constexpr int MAX_INPUT_CALLBACK_NUM = 1;
extern OnInputCallback input_callbacks[MAX_INPUT_CALLBACK_NUM];

struct JoystickEvent
{
  int deviceId;
  eastl::array<float, 48> axisValues;
};

jobject get_activity_class(void *activity);
JavaVM *get_java_vm(void *activity);

void activity_finish(void *activity);
void activity_setWindowFlags(void *activity, uint32_t values, uint32_t mask);

bool add_input_callback(OnInputCallback callback);
bool del_input_callback(OnInputCallback callback);

const char *app_command_to_string(int32_t cmd);

namespace input
{
void enable_axis(int32_t axis);
}

namespace motion_event
{
int32_t get_device_id(void *event);
int32_t get_action(void *event);
int32_t get_pointer_count(void *event);
int32_t get_pointer_id(void *event, int32_t pointer_index);
float get_x(void *event, int32_t pointer_index);
float get_y(void *event, int32_t pointer_index);
float get_axis_value(void *event, int32_t axis, size_t pointer_index);
} // namespace motion_event

namespace key_event
{
int32_t get_action(void *event);
int32_t get_key_code(void *event);
int32_t get_meta_state(void *event);
} // namespace key_event
} // namespace android

#if ANDROID_APP_GLUE_NO_NDEBUG
#undef NDEBUG
#undef ANDROID_APP_GLUE_NO_NDEBUG
#endif
