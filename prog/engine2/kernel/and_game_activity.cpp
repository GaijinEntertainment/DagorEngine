#include <supp/dag_android_native_app_glue.h>
#include <humanInput/dag_hiCreate.h>

#undef NDEBUG
#define NDEBUG 1

#include "game-activity/GameActivity.cpp"
#include "game-activity/GameActivityEvents.cpp"

extern int32_t dagor_android_handle_motion_event(struct android_app *app, int event_source, void *event);
extern int32_t dagor_android_handle_key_event(struct android_app *app, void *event);

namespace android
{
jobject get_activity_class(void *activity) { return ((GameActivity *)activity)->javaGameActivity; }

JavaVM *get_java_vm(void *activity) { return ((GameActivity *)activity)->vm; }

void activity_finish(void *activity) { GameActivity_finish((GameActivity *)activity); }

void activity_setWindowFlags(void *activity, uint32_t values, uint32_t mask)
{
  GameActivity_setWindowFlags((GameActivity *)activity, values, mask);
}

void init_input_handler(struct android_app *app) { android_app_set_motion_event_filter(app, NULL); }

void process_system_messages(struct android_app *app)
{
  struct android_input_buffer *ib = android_app_swap_input_buffers(app);
  if (!ib)
    return;

  const int imeStatus = HumanInput::getScreenKeyboardStatus_android();
  if (imeStatus != 1)
  {
    if (imeStatus != 2 && ib->motionEventsCount)
      for (int i = 0; i < ib->motionEventsCount; i++)
        dagor_android_handle_motion_event(app, ib->motionEvents[i].source, &ib->motionEvents[i]);

    if (ib->keyEventsCount)
      for (int i = 0; i < ib->keyEventsCount; i++)
        dagor_android_handle_key_event(app, &ib->keyEvents[i]);
  }

  if (ib->motionEventsCount)
    android_app_clear_motion_events(ib);

  if (ib->keyEventsCount)
    android_app_clear_key_events(ib);
}

const char *app_command_to_string(int32_t cmd)
{
  switch (cmd)
  {
    case UNUSED_APP_CMD_INPUT_CHANGED: return "UNUSED_APP_CMD_INPUT_CHANGED";
    case APP_CMD_INIT_WINDOW: return "APP_CMD_INIT_WINDOW";
    case APP_CMD_TERM_WINDOW: return "APP_CMD_TERM_WINDOW";
    case APP_CMD_WINDOW_RESIZED: return "APP_CMD_WINDOW_RESIZED";
    case APP_CMD_WINDOW_REDRAW_NEEDED: return "APP_CMD_WINDOW_REDRAW_NEEDED";
    case APP_CMD_CONTENT_RECT_CHANGED: return "APP_CMD_CONTENT_RECT_CHANGED";
    case APP_CMD_GAINED_FOCUS: return "APP_CMD_GAINED_FOCUS";
    case APP_CMD_LOST_FOCUS: return "APP_CMD_LOST_FOCUS";
    case APP_CMD_CONFIG_CHANGED: return "APP_CMD_CONFIG_CHANGED";
    case APP_CMD_LOW_MEMORY: return "APP_CMD_LOW_MEMORY";
    case APP_CMD_START: return "APP_CMD_START";
    case APP_CMD_RESUME: return "APP_CMD_RESUME";
    case APP_CMD_SAVE_STATE: return "APP_CMD_SAVE_STATE";
    case APP_CMD_PAUSE: return "APP_CMD_PAUSE";
    case APP_CMD_STOP: return "APP_CMD_STOP";
    case APP_CMD_DESTROY: return "APP_CMD_DESTROY";
    case APP_CMD_WINDOW_INSETS_CHANGED: return "APP_CMD_WINDOW_INSETS_CHANGED";
    default: return "unknown";
  }
  return "unknown";
}

namespace input
{
void enable_axis(int32_t axis) { GameActivityPointerAxes_enableAxis(axis); }
} // namespace input

namespace motion_event
{
int32_t get_device_id(void *event) { return ((GameActivityMotionEvent *)event)->deviceId; }

int32_t get_action(void *event) { return ((GameActivityMotionEvent *)event)->action; }

int32_t get_pointer_count(void *event) { return ((GameActivityMotionEvent *)event)->pointerCount; }

int32_t get_pointer_id(void *event, int32_t pointer_index) { return ((GameActivityMotionEvent *)event)->pointers[pointer_index].id; }

float get_x(void *event, int32_t pointer_index)
{
  return GameActivityPointerAxes_getX(&((GameActivityMotionEvent *)event)->pointers[pointer_index]);
}

float get_y(void *event, int32_t pointer_index)
{
  return GameActivityPointerAxes_getY(&((GameActivityMotionEvent *)event)->pointers[pointer_index]);
}

float get_axis_value(void *event, int32_t axis, size_t pointer_index)
{
  return GameActivityPointerAxes_getAxisValue(&((GameActivityMotionEvent *)event)->pointers[pointer_index], axis);
}
} // namespace motion_event

namespace key_event
{
int32_t get_action(void *event) { return ((GameActivityKeyEvent *)event)->action; }

int32_t get_key_code(void *event) { return ((GameActivityKeyEvent *)event)->keyCode; }

int32_t get_meta_state(void *event) { return ((GameActivityKeyEvent *)event)->metaState; }
} // namespace key_event
} // namespace android