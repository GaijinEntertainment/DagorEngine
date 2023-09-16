#include <supp/dag_android_native_app_glue.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <humanInput/dag_hiCreate.h>

extern int32_t dagor_android_handle_motion_event(struct android_app *app, int event_source, void *event);
extern int32_t dagor_android_handle_key_event(struct android_app *app, void *event);

static int32_t dagor_android_handle_input(struct android_app *app, AInputEvent *event)
{
  android_app *state = (android_app *)win32_get_instance();
  if (!state)
    return 0;

  int src = AInputEvent_getSource(event);

  int ime_status = HumanInput::getScreenKeyboardStatus_android();
  if (ime_status == 1)
    return 0;
  switch (AInputEvent_getType(event))
  {
    case AINPUT_EVENT_TYPE_MOTION: return ime_status == 2 ? 0 : dagor_android_handle_motion_event(app, src, event);
    case AINPUT_EVENT_TYPE_KEY: return dagor_android_handle_key_event(app, event);
  }
  return 0;
}

namespace android
{
jobject get_activity_class(void *activity) { return ((ANativeActivity *)activity)->clazz; }

JavaVM *get_java_vm(void *activity) { return ((ANativeActivity *)activity)->vm; }

void activity_finish(void *activity) { ANativeActivity_finish((ANativeActivity *)activity); }

void activity_setWindowFlags(void *activity, uint32_t values, uint32_t mask)
{
  ANativeActivity_setWindowFlags((ANativeActivity *)activity, values, mask);
}

void init_input_handler(struct android_app *app) { app->onInputEvent = dagor_android_handle_input; }

void process_system_messages(struct android_app *) {}

const char *app_command_to_string(int32_t cmd)
{
  switch (cmd)
  {
    case APP_CMD_INPUT_CHANGED: return "APP_CMD_INPUT_CHANGED";
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
    default: return "unknown";
  }
  return "unknown";
}

namespace input
{
void enable_axis(int32_t) {}
} // namespace input

namespace motion_event
{
int32_t get_device_id(void *event) { return AInputEvent_getDeviceId((AInputEvent *)event); }

int32_t get_action(void *event) { return AMotionEvent_getAction((AInputEvent *)event); }

int32_t get_pointer_count(void *event) { return AMotionEvent_getPointerCount((AInputEvent *)event); }

int32_t get_pointer_id(void *event, int32_t pointer_index) { return AMotionEvent_getPointerId((AInputEvent *)event, pointer_index); }

float get_x(void *event, int32_t pointer_index) { return AMotionEvent_getX((AInputEvent *)event, pointer_index); }

float get_y(void *event, int32_t pointer_index) { return AMotionEvent_getY((AInputEvent *)event, pointer_index); }

float get_axis_value(void *event, int32_t axis, size_t pointer_index)
{
  return AMotionEvent_getAxisValue((AInputEvent *)event, axis, pointer_index);
}
} // namespace motion_event

namespace key_event
{
int32_t get_action(void *event) { return AKeyEvent_getAction((AInputEvent *)event); }

int32_t get_key_code(void *event) { return AKeyEvent_getKeyCode((AInputEvent *)event); }

int32_t get_meta_state(void *event) { return AKeyEvent_getMetaState((AInputEvent *)event); }
} // namespace key_event
} // namespace android