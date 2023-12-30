#include <consoleKeyBindings/consoleKeyBindings.h>
#include <humanInput/dag_hiKeyboard.h>
#include <humanInput/dag_hiKeybIds.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <osApiWrappers/dag_direct.h>
#include <startup/dag_inpDevClsDrv.h>
#include <util/dag_console.h>
#include <osApiWrappers/dag_spinlock.h>

namespace console_keybindings
{
OSSpinlock console_keybindings_mutex;
Tab<ConsoleKeybinding> console_keybindings;
SimpleString keybindings_file_path;


__forceinline bool check_modifier(unsigned a, unsigned b, unsigned mask)
{
  return ((a & mask) == (b & mask) || ((b & mask) == mask) && (a & mask) || ((a & mask) == mask) && (b & mask));
}


__forceinline bool check_modifiers(unsigned a, unsigned b)
{
  return check_modifier(a, b, HumanInput::KeyboardRawState::CTRL_BITS) &&
         check_modifier(a, b, HumanInput::KeyboardRawState::ALT_BITS) &&
         check_modifier(a, b, HumanInput::KeyboardRawState::SHIFT_BITS);
}


// unsafe, because not thread safe
__forceinline ConsoleKeybinding &insert_in_best_place_unsafe(unsigned modifiers)
{
  for (size_t i = 0; i < console_keybindings.size(); ++i)
    if (console_keybindings[i].modifiers < modifiers)
      return console_keybindings[insert_item_at(console_keybindings, i, ConsoleKeybinding())];
  return console_keybindings.push_back();
}


struct ModifierToken
{
  const char *modName;
  unsigned modMask;
};


static const ModifierToken modifier_tokens[] = {
  {"lctrl", HumanInput::KeyboardRawState::LCTRL_BIT},
  {"rctrl", HumanInput::KeyboardRawState::RCTRL_BIT},
  {"ctrl", HumanInput::KeyboardRawState::CTRL_BITS},
  {"lalt", HumanInput::KeyboardRawState::LALT_BIT},
  {"ralt", HumanInput::KeyboardRawState::RALT_BIT},
  {"alt", HumanInput::KeyboardRawState::ALT_BITS},
  {"lshift", HumanInput::KeyboardRawState::LSHIFT_BIT},
  {"rshift", HumanInput::KeyboardRawState::RSHIFT_BIT},
  {"shift", HumanInput::KeyboardRawState::SHIFT_BITS},
};


static const char *find_next_delimiter(const char *str)
{
  for (; *str; str++)
    if (strchr("+-_ ", *str))
      return str;
  return nullptr;
}


static int is_real_keyboard_present()
{
  for (int k = 0; k < global_cls_drv_kbd->getDeviceCount(); k++)
  {
    HumanInput::IGenKeyboard *kbd = global_cls_drv_kbd->getDevice(k);
    if (kbd && kbd->getKeyName(HumanInput::DKEY_F1) != nullptr)
      return true;
  }
  return false;
}


static int get_key_code(const char *key_name)
{
  for (int k = 0; k < global_cls_drv_kbd->getDeviceCount(); k++)
  {
    HumanInput::IGenKeyboard *kbd = global_cls_drv_kbd->getDevice(k);
    if (kbd)
    {
      if (kbd->getKeyName(HumanInput::DKEY_F1) == nullptr) // only real keyboards
        continue;

      for (int i = 0; i < kbd->getKeyCount(); i++)
      {
        String underscoredKeyName(kbd->getKeyName(i));
        for (int j = 0; j < underscoredKeyName.length(); j++)
          if (underscoredKeyName[j] == ' ')
            underscoredKeyName[j] = '_';

        underscoredKeyName.toLower();
        if (!strcmp(key_name, underscoredKeyName.str()))
        {
          return i;
        }
      }
    }
  }

  return -1;
}


static void list_keys()
{
  if (!global_cls_drv_kbd)
    return;

  for (int k = 0; k < global_cls_drv_kbd->getDeviceCount(); k++)
  {
    HumanInput::IGenKeyboard *kbd = global_cls_drv_kbd->getDevice(k);
    if (kbd)
    {
      if (kbd->getKeyName(HumanInput::DKEY_F1) == nullptr) // only real keyboards
        continue;

      console::print("Keys of keyboard #%d:", k);

      for (int i = 0; i < kbd->getKeyCount(); i++)
      {
        String underscoredKeyName(kbd->getKeyName(i));
        for (int j = 0; j < underscoredKeyName.length(); j++)
          if (underscoredKeyName[j] == ' ')
            underscoredKeyName[j] = '_';

        if (underscoredKeyName.empty())
          continue;

        underscoredKeyName.toLower();
        console::print("%s", underscoredKeyName.str());
      }
    }
  }
}


static bool parse_shortcut(const char *key_comb, const char *error_prefix, unsigned &result_modifiers, int &result_key_code)
{
  result_modifiers = 0;
  result_key_code = -1;

  String keyComb(key_comb);

  keyComb.toLower();

  const char *cur = keyComb.str();
  const char *next = find_next_delimiter(cur);
  while (next)
  {
    if (next == cur)
      break;

    bool found = false;
    for (const ModifierToken &token : modifier_tokens)
      if (!strncmp(cur, token.modName, next - cur) && strlen(token.modName) == next - cur)
      {
        if (result_modifiers & token.modMask)
        {
          console::error("%s %s - duplicate modifier '%s'", error_prefix, key_comb, token.modName);
          return false;
        }
        result_modifiers |= token.modMask;
        found = true;
        break;
      }

    if (!found)
    {
      // if we didn't find a modifier, maybe we hit some strange key which we wanted to treat as a modifier (e.g. num_1)
      result_key_code = get_key_code(cur);
      if (result_key_code != -1)
        break;

      console::error("%s %s - invalid modifier '%.*s'", error_prefix, key_comb, next - cur, cur);
      return false;
    }

    cur = next + 1;
    next = find_next_delimiter(cur);
  }


  const char *keyName = cur;
  if (!keyName || !strlen(keyName))
  {
    console::error("%s %s - key name is empty", error_prefix, key_comb);
    return false;
  }

  if (result_key_code == -1) // we might have found the key while checking for modifiers, so let's not do the search twice
    result_key_code = get_key_code(keyName);

  if (result_key_code == -1)
  {
    console::error("%s %s - invalid key name", error_prefix, key_comb);
    return false;
  }

  return true;
}


bool bind(const char *key_comb, const char *command)
{
  if (!global_cls_drv_kbd || !global_cls_drv_kbd->getDeviceCount())
    return false;

  if (!key_comb || !command || !*key_comb || !*command)
  {
    console::error("console.bind - invalid arguments, shortcut or command is empty");
    return false;
  }

  if (!is_real_keyboard_present())
    return false;

  int keyCode = -1;
  unsigned modifiers = 0;
  if (!parse_shortcut(key_comb, "console.bind", modifiers, keyCode))
    return false;

  String keyComb(key_comb);
  keyComb.toLower();

  {
    OSSpinlockScopedLock lock(&console_keybindings_mutex);
    int index = -1;
    for (int i = console_keybindings.size() - 1; i >= 0; i--)
      if (console_keybindings[i].keyCode == keyCode && check_modifiers(modifiers, console_keybindings[i].modifiers))
      {
        index = i;
        break;
      }

    if (index != -1 && console_keybindings[index].command != command)
      console::warning("console.bind %s - shortcut already bound to '%s' and will be replaced with '%s'", key_comb,
        console_keybindings[index].command.str(), command);

    ConsoleKeybinding &binding = (index == -1) ? insert_in_best_place_unsafe(modifiers) : console_keybindings[index];
    binding.command = command;
    binding.keyCode = keyCode;
    binding.modifiers = modifiers;
    binding.keyCombo = keyComb;
  }
  return true;
}


bool unbind(const char *key_comb)
{
  if (!global_cls_drv_kbd || !global_cls_drv_kbd->getDeviceCount())
    return false;

  if (!key_comb || !*key_comb)
  {
    console::error("console.unbind - invalid arguments, shortcut is empty");
    return false;
  }

  int keyCode = -1;
  unsigned modifiers = 0;
  if (!parse_shortcut(key_comb, "console.unbind", modifiers, keyCode))
    return false;

  {
    bool found = false;
    OSSpinlockScopedLock lock(&console_keybindings_mutex);
    for (int i = console_keybindings.size() - 1; i >= 0; i--)
      if (console_keybindings[i].keyCode == keyCode && check_modifiers(modifiers, console_keybindings[i].modifiers))
      {
        console_keybindings.erase(console_keybindings.begin() + i);
        found = true;
      }

    if (found)
      return true;
  }

  console::warning("console.unbind %s - shotrcut not bound", key_comb);
  return false;
}


const char *get_command_by_key_code(int key_code, unsigned modifiers)
{
  OSSpinlockScopedLock lock(&console_keybindings_mutex);
  for (const auto &keybinding : console_keybindings)
    if (keybinding.keyCode == key_code && check_modifiers(modifiers, keybinding.modifiers))
      return keybinding.command.str();

  return nullptr;
}


void clear()
{
  OSSpinlockScopedLock lock(&console_keybindings_mutex);
  clear_and_shrink(console_keybindings);
}


void set_binds_file_path(const String &path) { keybindings_file_path = path; }


void load_binds_from_file()
{
  DataBlock consoleBinds;
  if (dd_file_exist(keybindings_file_path))
  {
    dblk::load(consoleBinds, keybindings_file_path, dblk::ReadFlag::ROBUST);
    for (int i = 0; i < consoleBinds.blockCount(); ++i)
    {
      const DataBlock *bind = consoleBinds.getBlock(i);
      const char *key = bind->getStr("key", nullptr);
      const char *command = bind->getStr("command", nullptr);
      if (key && command)
        if (!console_keybindings::bind(key, command))
          console::error("Failed to load console binding '%s' from file '%s'", key, keybindings_file_path.str());
    }
  }
}


void save_binds_to_file()
{
  OSSpinlockScopedLock lock(&console_keybindings_mutex);
  DataBlock consoleBinds;
  for (const auto &binding : console_keybindings)
  {
    DataBlock *bind = consoleBinds.addNewBlock("bind");
    bind->setStr("key", binding.keyCombo);
    bind->setStr("command", binding.command);
  }
  dblk::save_to_text_file(consoleBinds, keybindings_file_path);
}
} // namespace console_keybindings


static bool consoleKeybindings_console_handler(const char *argv[], int argc)
{
  int found = 0;
  found = console::collector_cmp(argv[0], argc, "consoleKeybindings.bind", 3, 3, "", "<shortcut> \"<command>\"");
  if (found == 0)
    found = console::collector_cmp(argv[0], argc, "console.bind", 3, 3, "", "<shortcut> \"<command>\"");
  if (found != 0)
  {
    if (found == -1)
    {
      console::print("Usage: %s <shortcut> \"<command>\" to call a console command on key/shortcut press", argv[0]);
      console::print(
        "Usage: use '+' to add modifiers like 'rshift+lctrl+k' for the shortcut. Possible moddifiers: ctrl, shift, alt (and "
        "l, r versions)");
      return true;
    }

    const char *command = argv[2];
    size_t commandLen = strlen(command);
    String clearedCommand =
      commandLen > 2 && command[0] == '"' && command[commandLen - 1] == '"' ? String(&command[1], commandLen - 2) : String(command);

    if (console_keybindings::bind(argv[1], clearedCommand.c_str()))
      console::print_d("Shortcut '%s' bound for '%s'", argv[1], clearedCommand.c_str());

    return true;
  }
  CONSOLE_CHECK_NAME_EX("console", "binds_save", 1, 1, "Save all current binds to be auto loaded on next launch", "")
  {
    console_keybindings::save_binds_to_file();
    return true;
  }
  CONSOLE_CHECK_NAME_EX("console", "bind_list_keys", 1, 1, "Print all possible key names for bindings", "")
  {
    console_keybindings::list_keys();
    return true;
  }
  CONSOLE_CHECK_NAME_EX("console", "unbind", 2, 2, "Remove binding", "<shortcut>")
  {
    if (console_keybindings::unbind(argv[1]))
      console::print_d("Shortcut '%s' unbound", argv[1]);
    return true;
  }
  CONSOLE_CHECK_NAME_EX("console", "binds_list", 1, 1, "List all current binds", "")
  {
    OSSpinlockScopedLock lock(&console_keybindings::console_keybindings_mutex);
    for (const auto &bind : console_keybindings::console_keybindings)
      console::print_d("Shortcut '%s' bound for '%s'", bind.keyCombo, bind.command);
    return true;
  }

  return false;
}

REGISTER_CONSOLE_HANDLER(consoleKeybindings_console_handler);
