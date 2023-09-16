#include <consoleKeyBindings/consoleKeyBindings.h>
#include <humanInput/dag_hiKeyboard.h>
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

__forceinline bool is_mods_valid(unsigned checked_mods, unsigned mods)
{
  unsigned u = checked_mods & mods;
  return ((mods & HumanInput::KeyboardRawState::CTRL_BITS) != 0 ? u & HumanInput::KeyboardRawState::CTRL_BITS : 1) &&
         ((mods & HumanInput::KeyboardRawState::ALT_BITS) != 0 ? u & HumanInput::KeyboardRawState::ALT_BITS : 1) &&
         ((mods & HumanInput::KeyboardRawState::SHIFT_BITS) != 0 ? u & HumanInput::KeyboardRawState::SHIFT_BITS : 1);
}

// unsafe, because not thread safe
__forceinline ConsoleKeybinding &insert_in_best_place_unsafe(unsigned modifiers)
{
  for (size_t i = 0; i < console_keybindings.size(); ++i)
    if (console_keybindings[i].modifiers < modifiers)
      return console_keybindings[insert_item_at(console_keybindings, i, ConsoleKeybinding())];
  return console_keybindings.push_back();
}

bool bind(const char *key_comb, const char *command)
{
  G_ASSERT(key_comb);
  String keyComb(key_comb);
  if (!global_cls_drv_kbd || !global_cls_drv_kbd->getDeviceCount())
    return false;

  keyComb.toLower();
  HumanInput::IGenKeyboard *kbd = global_cls_drv_kbd->getDevice(0);

  unsigned modifiers = 0;
  int shift = 0;
  if (strstr(keyComb.str(), "lctrl"))
  {
    modifiers |= HumanInput::KeyboardRawState::LCTRL_BIT;
    shift += 6;
  }
  else if (strstr(keyComb.str(), "rctrl"))
  {
    modifiers |= HumanInput::KeyboardRawState::RCTRL_BIT;
    shift += 6;
  }
  else if (strstr(keyComb.str(), "ctrl"))
  {
    modifiers |= HumanInput::KeyboardRawState::CTRL_BITS;
    shift += 5;
  }
  if (strstr(keyComb.str(), "lalt"))
  {
    modifiers |= HumanInput::KeyboardRawState::LALT_BIT;
    shift += 5;
  }
  else if (strstr(keyComb.str(), "ralt"))
  {
    modifiers |= HumanInput::KeyboardRawState::RALT_BIT;
    shift += 5;
  }
  else if (strstr(keyComb.str(), "alt"))
  {
    modifiers |= HumanInput::KeyboardRawState::ALT_BITS;
    shift += 4;
  }
  if (strstr(keyComb.str(), "lshift"))
  {
    modifiers |= HumanInput::KeyboardRawState::LSHIFT_BIT;
    shift += 7;
  }
  else if (strstr(keyComb.str(), "rshift"))
  {
    modifiers |= HumanInput::KeyboardRawState::RSHIFT_BIT;
    shift += 7;
  }
  else if (strstr(keyComb.str(), "shift"))
  {
    modifiers |= HumanInput::KeyboardRawState::SHIFT_BITS;
    shift += 6;
  }

  const char *keyName = keyComb.str();
  if (modifiers)
  {
    for (int i = shift - 1, len = keyComb.length(); i < len; ++i)
      if (strchr("+-_ ", keyComb[i]))
      {
        keyName += (i + 1);
        break;
      }
  }
  if (!keyName || !strlen(keyName))
    return false;

  int keyCode = -1;
  for (int i = 0; i < kbd->getKeyCount(); i++)
  {
    String underscoredKeyName(kbd->getKeyName(i));
    for (int j = 0; j < underscoredKeyName.length(); j++)
      if (underscoredKeyName[j] == ' ')
        underscoredKeyName[j] = '_';

    underscoredKeyName.toLower();
    if (!strcmp(keyName, underscoredKeyName.str()))
    {
      keyCode = i;
      break;
    }
  }
  {
    OSSpinlockScopedLock lock(&console_keybindings_mutex);
    int index = -1;
    for (int i = console_keybindings.size() - 1; i >= 0; i--)
      if (console_keybindings[i].keyCode == keyCode && is_mods_valid(modifiers, console_keybindings[i].modifiers))
      {
        index = i;
        break;
      }

    ConsoleKeybinding &binding = (index == -1) ? insert_in_best_place_unsafe(modifiers) : console_keybindings[index];
    binding.command = command;
    binding.keyCode = keyCode;
    binding.modifiers = modifiers;
    binding.keyCombo = keyComb;
  }
  return true;
}

const char *get_command_by_key_code(int key_code, unsigned modifiers)
{
  OSSpinlockScopedLock lock(&console_keybindings_mutex);
  for (const auto &keybinding : console_keybindings)
    if (keybinding.keyCode == key_code && is_mods_valid(modifiers, keybinding.modifiers))
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
          logerr("Binding <%s> error. Check key name.", key);
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
  found = console::collector_cmp(argv[0], argc, "consoleKeybindings.bind", 3, 3, "", "<key> \"<command>\"");
  if (found == 0)
    found = console::collector_cmp(argv[0], argc, "console.bind", 3, 3, "", "<key> \"<command>\"");
  if (found != 0)
  {
    if (found == -1)
    {
      console::print("Usage: %s <key> \"<command>\" to call a console command on key press", argv[0]);
      console::print("Usage: use '+' to add moddifiers like 'rshift+lctrl+k' for the key. Possible moddifiers: ctrl, shift, alt (and "
                     "l, r versions)");
      return true;
    }

    const char *command = argv[2];
    size_t commandLen = strlen(command);
    String clearedCommand =
      commandLen > 2 && command[0] == '"' && command[commandLen - 1] == '"' ? String(&command[1], commandLen - 2) : String(command);

    if (console_keybindings::bind(argv[1], clearedCommand.c_str()))
      console::print_d("Key '%s' bound for '%s'", argv[1], clearedCommand.c_str());
    else
      logerr("Binding <%s> error. Check key name.", argv[1]);

    return true;
  }
  CONSOLE_CHECK_NAME_EX("console", "binds_save", 1, 1, "Save all current binds to be auto loaded on next launch", "")
  {
    console_keybindings::save_binds_to_file();
    return true;
  }
  CONSOLE_CHECK_NAME_EX("console", "binds_list", 1, 1, "List all current binds", "")
  {
    OSSpinlockScopedLock lock(&console_keybindings::console_keybindings_mutex);
    for (const auto &bind : console_keybindings::console_keybindings)
      console::print_d("Key '%s' bound for '%s'", bind.keyCombo, bind.command);
    return true;
  }

  return false;
}

REGISTER_CONSOLE_HANDLER(consoleKeybindings_console_handler);
