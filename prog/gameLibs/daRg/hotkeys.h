#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_tab.h>
#include <util/dag_fastStrMap.h>
#include <util/dag_string.h>
#include <sqrat.h>

#include <daRg/dag_inputIds.h>
#include <daRg/dag_guiScene.h>
#include <humanInput/dag_hiXInputMappings.h>


namespace darg
{


struct IGuiScene;
class Element;
class StringKeys;


class HotkeyCombo
{
public:
  bool updateOnCombo();
  bool updateOnEvent(IGuiScene *scene, Element *elem, const char *event_id, int id_str_len, bool pressed); //< return true if processed
  void triggerOnCombo(IGuiScene *scene, Element *elem);

public:
  Tab<HotkeyButton> buttons;
  String eventName;
  bool isPressed = false;

  enum TriggerFront
  {
    TF_PRESS,
    TF_RELEASE
  };

  TriggerFront triggerFront = TF_PRESS;
  Sqrat::Function handler;
  Sqrat::Object description;
  Sqrat::Object soundKey;
  bool inputPassive = false;
  bool ignoreConsumerCallback = false;
};


class Hotkeys
{
public:
  enum
  {
    DEV_ID_SHIFT = 8,
    BTN_ID_MASK = 0xFF
  };
  typedef int32_t EncodedKey;

  Hotkeys();
  void loadCombos(const StringKeys *csk, const Sqrat::Table &comp_desc, const Sqrat::Array &hotkeys_data,
    dag::Vector<eastl::unique_ptr<HotkeyCombo>> &out) const;

  const char *getButtonName(HotkeyButton btn) const;
  HotkeyButton resolveButton(const char *name) const;

  static void collectButtonNames(FastStrMapT<EncodedKey> &name_map);

private:
  static void collectKeyboardKeyNames(FastStrMapT<EncodedKey> &name_map);
  static void collectJoystickButtonNames(FastStrMapT<EncodedKey> &name_map);
  static void collectMouseButtonNames(FastStrMapT<EncodedKey> &name_map);

  void parseButtonsString(const char *str, Tab<Tab<HotkeyButton>> &out_buttons, Tab<String> &out_events,
    HotkeyCombo::TriggerFront &out_front) const;

private:
  FastStrMapT<EncodedKey> btnNameMap;
};


} // namespace darg
