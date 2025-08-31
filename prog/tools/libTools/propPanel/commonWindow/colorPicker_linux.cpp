// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_color.h>
#include <math/dag_e3dColor.h>
#include <osApiWrappers/dag_miscApi.h>
#include <util/dag_string.h>
#include <workCycle/dag_workCycle.h>

#include <EASTL/optional.h>

#include <dbus/dbus.h>

namespace PropPanel
{

static eastl::optional<E3DCOLOR> get_color_from_variant(DBusMessageIter &parent_iter)
{
  if (dbus_message_iter_get_arg_type(&parent_iter) != DBUS_TYPE_VARIANT)
    return {};

  DBusMessageIter iter;
  dbus_message_iter_recurse(&parent_iter, &iter);
  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRUCT)
    return {};

  DBusMessageIter structIter;
  dbus_message_iter_recurse(&iter, &structIter);

  if (dbus_message_iter_get_arg_type(&structIter) != DBUS_TYPE_DOUBLE)
    return {};
  double r = 0.0;
  dbus_message_iter_get_basic(&structIter, &r);

  dbus_message_iter_next(&structIter);
  if (dbus_message_iter_get_arg_type(&structIter) != DBUS_TYPE_DOUBLE)
    return {};
  double g = 0.0;
  dbus_message_iter_get_basic(&structIter, &g);

  dbus_message_iter_next(&structIter);
  if (dbus_message_iter_get_arg_type(&structIter) != DBUS_TYPE_DOUBLE)
    return {};
  double b = 0.0;
  dbus_message_iter_get_basic(&structIter, &b);

  return e3dcolor(Color4(r, g, b));
}

static eastl::optional<E3DCOLOR> get_color_from_array(DBusMessageIter &iter)
{
  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY)
    return {};

  DBusMessageIter dictIter;
  dbus_message_iter_recurse(&iter, &dictIter);

  while (dbus_message_iter_get_arg_type(&dictIter) == DBUS_TYPE_DICT_ENTRY)
  {
    DBusMessageIter entry;
    dbus_message_iter_recurse(&dictIter, &entry);
    if (dbus_message_iter_get_arg_type(&entry) != DBUS_TYPE_STRING)
      return {};

    const char *key;
    dbus_message_iter_get_basic(&entry, &key);
    if (strcmp(key, "color") == 0)
    {
      dbus_message_iter_next(&entry);
      if (dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_VARIANT)
        return get_color_from_variant(entry);
      else
        return {};
    }

    dbus_message_iter_next(&dictIter);
  }

  return {};
}

static eastl::optional<E3DCOLOR> wait_for_reply(DBusConnection *connection)
{
  while (true)
  {
    dagor_idle_cycle(false);
    dbus_connection_read_write(connection, 100);

    DBusMessage *msg = dbus_connection_pop_message(connection);
    if (!msg)
    {
      sleep_msec(100);
      continue;
    }

    if (dbus_message_is_signal(msg, "org.freedesktop.portal.Request", "Response"))
    {
      DBusMessageIter iter;
      dbus_message_iter_init(msg, &iter);

      uint32_t response_code = 1;
      if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_UINT32)
        dbus_message_iter_get_basic(&iter, &response_code);
      dbus_message_iter_next(&iter);

      eastl::optional<E3DCOLOR> color;
      if (response_code == 0) // Success, the request is carried out.
        color = get_color_from_array(iter);

      dbus_message_unref(msg);
      return color;
    }

    dbus_message_unref(msg);
  }
}

static eastl::optional<E3DCOLOR> show_linux_color_picker_internal(DBusConnection *connection, DBusError &error)
{
  DBusMessage *msg = dbus_message_new_method_call("org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
    "org.freedesktop.portal.Screenshot", "PickColor");
  if (!msg)
    return {};

  DBusMessageIter args;
  dbus_message_iter_init_append(msg, &args);
  const char *parentWindow = "";
  dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &parentWindow);

  DBusMessageIter dict;
  dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &dict);
  dbus_message_iter_close_container(&args, &dict);

  DBusMessage *reply = dbus_connection_send_with_reply_and_block(connection, msg, DBUS_TIMEOUT_USE_DEFAULT, &error);
  dbus_message_unref(msg);

  if (dbus_error_is_set(&error))
  {
    logerr("Failed to call org.freedesktop.portal.Screenshot.PickColor D-BUS method. Error: %s", error.message);
    dbus_error_free(&error);
    return {};
  }

  if (!dbus_message_iter_init(reply, &args))
  {
    dbus_message_unref(reply);
    return {};
  }

  if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_OBJECT_PATH)
  {
    dbus_message_unref(reply);
    return {};
  }

  char *path = nullptr;
  dbus_message_iter_get_basic(&args, &path);
  dbus_message_unref(reply);

  if (path == nullptr || *path == 0)
    return {};

  String matchRule(0, "type='signal',interface='org.freedesktop.portal.Request',member='Response',path='%s'", path);
  dbus_bus_add_match(connection, matchRule, &error);
  dbus_connection_flush(connection);

  return wait_for_reply(connection);
}

eastl::optional<E3DCOLOR> show_linux_color_picker()
{
  DBusError error;
  dbus_error_init(&error);

  DBusConnection *connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
  if (dbus_error_is_set(&error))
  {
    logerr("Failed to open D-BUS connection. Error: %s", error.message);
    dbus_error_free(&error);
    return {};
  }

  const eastl::optional<E3DCOLOR> color = show_linux_color_picker_internal(connection, error);

  dbus_connection_unref(connection);
  return color;
}

} // namespace PropPanel