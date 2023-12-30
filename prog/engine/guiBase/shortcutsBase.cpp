#include <gui/dag_shortcuts.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_sort.h>
#include <generic/dag_smallTab.h>
#include <util/dag_globDef.h>
#include <util/dag_fastIntList.h>
#include <startup/dag_globalSettings.h>

#include <humanInput/dag_hiKeybIds.h>
#include <startup/dag_inpDevClsDrv.h>
#include <humanInput/dag_hiPointing.h>
#include <humanInput/dag_hiKeyboard.h>
#include <humanInput/dag_hiJoystick.h>
#include <humanInput/dag_hiGlobals.h>
#include <generic/dag_carray.h>
#include <debug/dag_debug.h>

static const int SHORTCUT_BUFFER_SIZE = 32;

static const int MOUSE_WHEEL_UP_BUTTON = 5;
static const int MOUSE_WHEEL_DOWN_BUTTON = 6;

// TODO: add Tap and Double-Tap
enum GestureEvent
{
  EV_GESTURE_TAP,
  EV_GESTURE_HOLD,
  EV_GESTURE_FLICK_L,
  EV_GESTURE_FLICK_R,
  EV_GESTURE_FLICK_U,
  EV_GESTURE_FLICK_D,
  EV_GESTURE_DOUBLE_TAP,

  EV_GESTURE_TOTAL
};
constexpr const char *gesture_names[] = {"gesture_tap", "gesture_tap_and_hold", "gesture_flick_left", "gesture_flick_right",
  "gesture_flick_up", "gesture_flick_down", "gesture_double_tap"};

const char *get_shortcut_button_name(UserInputDeviceId device_id, int button_id)
{
  const char *name = NULL;

  if (device_id == STD_MOUSE_DEVICE_ID)
  {
    if (global_cls_drv_pnt && global_cls_drv_pnt->getDeviceCount())
      name = global_cls_drv_pnt->getDevice(0)->getBtnName(button_id);
    return name ? name : "?";
  }
  if (device_id == STD_KEYBOARD_DEVICE_ID)
  {
    if (global_cls_drv_kbd && global_cls_drv_kbd->getDeviceCount())
      name = global_cls_drv_kbd->getDevice(0)->getKeyName(button_id);
    return name ? name : "?";
  }
  if (device_id == DEF_JOYSTICK_DEVICE_ID)
  {
    HumanInput::IGenJoystick *j = global_cls_drv_joy ? global_cls_drv_joy->getDefaultJoystick() : NULL;

    name = j ? j->getButtonName(button_id) : "?";
    return name ? name : "?";
  }
  if (device_id == STD_GESTURE_DEVICE_ID)
    return (button_id >= 0 && button_id < countof(gesture_names)) ? gesture_names[button_id] : "?gesture";

  return "?dev";
}


INSTANTIATE_SMART_NAMEID(EventClassId);


struct KeyboardState
{
  void setKeyDown(int btn_idx) { bitMask[btn_idx >> 5] |= (1 << (btn_idx & 0x1F)); }
  void clrKeyDown(int btn_idx) { bitMask[btn_idx >> 5] &= ~(1 << (btn_idx & 0x1F)); }
  bool isKeyDown(int btn_idx) const { return bitMask[btn_idx >> 5] & (1 << (btn_idx & 0x1F)); }
  inline void operator=(const KeyboardState &st) { memcpy(bitMask.data(), st.bitMask.data(), sizeof(bitMask)); }
  inline void clear() { mem_set_0(bitMask); }
  static constexpr int SIZE_BITS = 256;

protected:
  carray<unsigned, SIZE_BITS / 32> bitMask;
};

struct DeviceEventState
{
public:
  inline void set(UserInputDeviceId dev_id, int btn_idx, bool dir_down, int st_idx)
  {
    G_ASSERT(btn_idx >= 0);
    devId = dev_id;
    btnIdx = btn_idx;
    dirDown = dir_down;
    stateIdx = st_idx;
  }

  inline bool get(UserInputDeviceId &dev_id, int &btn_idx, int &st_idx)
  {
    dev_id = devId;
    btn_idx = btnIdx;
    st_idx = stateIdx;
    return dirDown;
  }

protected:
  UserInputDeviceId devId;
  int btnIdx, stateIdx;
  bool dirDown;
};

static KeyboardState kbdSt;
static HumanInput::ButtonBits mouseSt, joySt, gestureSt;

enum StateDevice
{
  STATE_DEV_KEYBOARD,
  STATE_DEV_MOUSE,
  STATE_DEV_JOYSTICK,
  STATE_DEV_GESTURE,

  STATE_DEV__COUNT,
};

static char stateCounter[STATE_DEV__COUNT];

static HumanInput::IGenJoystick *lastJoy = NULL;
static Tab<KeyboardState> kbdStBuf(midmem);
static Tab<HumanInput::ButtonBits> mouseStBuf(midmem), joyStBuf(midmem), gestureStBuf(midmem);
static Tab<DeviceEventState> deviceEvStack(midmem);
static KeyboardState *kbdStCtx = NULL;
static HumanInput::ButtonBits *mouseStCtx = NULL, *joyStCtx = NULL, *gestureStCtx = NULL;
static bool majorActiveBitsReady = false;
static unsigned char device_mask = 0xFF;


template <class T>
static inline bool addBufEnt(Tab<T> &buf, const T &st)
{
  if (buf.size() >= SHORTCUT_BUFFER_SIZE)
    return false;
  buf.push_back(st);

  return true;
}

static inline void addKbdEv()
{
  if (stateCounter[STATE_DEV_KEYBOARD]++ > 0)
    addBufEnt(kbdStBuf, kbdSt);
}

static inline void addMouseEv()
{
  if (stateCounter[STATE_DEV_MOUSE]++ > 0)
    addBufEnt(mouseStBuf, mouseSt);
}

static inline void addJoyEv()
{
  if (stateCounter[STATE_DEV_JOYSTICK]++ > 0)
    addBufEnt(joyStBuf, joySt);
}

static inline void addGestureEv()
{
  if (stateCounter[STATE_DEV_GESTURE]++ > 0)
    addBufEnt(gestureStBuf, gestureSt);
}

void on_shortcuts_joystick_down(HumanInput::IGenJoystick *joy, int btn_id)
{
  if (!global_cls_drv_joy || (joy != global_cls_drv_joy->getDefaultJoystick()))
    return;

  if (lastJoy != joy)
  {
    clear_shortcuts_state_buffer();
    lastJoy = joy;
  }

  addJoyEv();

  joySt.set(btn_id);

  deviceEvStack.push_back().set(DEF_JOYSTICK_DEVICE_ID, btn_id, true, joyStBuf.size());
  majorActiveBitsReady = false;
}

void on_shortcuts_joystick_up(HumanInput::IGenJoystick *joy, int btn_id)
{
  if (!global_cls_drv_joy || (joy != global_cls_drv_joy->getDefaultJoystick()))
    return;

  if (lastJoy != joy)
  {
    clear_shortcuts_state_buffer();
    lastJoy = joy;
  }

  addJoyEv();

  joySt.clr(btn_id);

  deviceEvStack.push_back().set(DEF_JOYSTICK_DEVICE_ID, btn_id, false, joyStBuf.size());
  majorActiveBitsReady = false;
}

void on_shortcuts_kbd_down(HumanInput::IGenKeyboard * /*kbd*/, int btn_idx, wchar_t /*wc*/)
{
  addKbdEv();
  kbdSt.setKeyDown(btn_idx);

  deviceEvStack.push_back().set(STD_KEYBOARD_DEVICE_ID, btn_idx, true, kbdStBuf.size());
  majorActiveBitsReady = false;
}

void on_shortcuts_kbd_up(HumanInput::IGenKeyboard * /*kbd*/, int btn_idx)
{
  addKbdEv();
  kbdSt.clrKeyDown(btn_idx);

  deviceEvStack.push_back().set(STD_KEYBOARD_DEVICE_ID, btn_idx, false, kbdStBuf.size());
  majorActiveBitsReady = false;
}

void on_shortcuts_mouse_wheel(HumanInput::IGenPointing * /*mouse*/, bool down)
{
  const int btnIdx = down ? MOUSE_WHEEL_DOWN_BUTTON : MOUSE_WHEEL_UP_BUTTON;

  deviceEvStack.push_back().set(STD_MOUSE_DEVICE_ID, btnIdx, false, -1);
}

void on_shortcuts_mouse_down(HumanInput::IGenPointing * /*mouse*/, int btn_idx)
{
  addMouseEv();
  mouseSt.set(btn_idx);

  deviceEvStack.push_back().set(STD_MOUSE_DEVICE_ID, btn_idx, true, mouseStBuf.size());
  majorActiveBitsReady = false;
}

void on_shortcuts_mouse_up(HumanInput::IGenPointing * /*mouse*/, int btn_idx)
{
  addMouseEv();
  mouseSt.clr(btn_idx);

  deviceEvStack.push_back().set(STD_MOUSE_DEVICE_ID, btn_idx, false, mouseStBuf.size());
  majorActiveBitsReady = false;
}

void stop_momentary_gestures()
{
  // These gestures have no END event and must be cleared manually on the next frame
  // or after processing whichever comes first
  const GestureEvent momentaries[] = {
    EV_GESTURE_TAP,
    EV_GESTURE_FLICK_L,
    EV_GESTURE_FLICK_R,
    EV_GESTURE_FLICK_U,
    EV_GESTURE_FLICK_D,
    EV_GESTURE_DOUBLE_TAP,
  };

  for (GestureEvent g : momentaries)
    if (gestureStCtx->get(g))
    {
      addGestureEv();
      gestureSt.clr(g);
      deviceEvStack.push_back().set(STD_GESTURE_DEVICE_ID, g, false, -1);
      majorActiveBitsReady = false;
    }
}

void on_shortcuts_gesture(HumanInput::IGenPointing *, HumanInput::Gesture::State s, const HumanInput::Gesture &g)
{
  using HumanInput::Gesture;

  switch (g.type)
  {
    case Gesture::Type::FLICK:
    {
      addGestureEv();
      constexpr int idMap[] = {EV_GESTURE_FLICK_L, EV_GESTURE_FLICK_R, EV_GESTURE_FLICK_U, EV_GESTURE_FLICK_D};
      const int id = idMap[g.dir];
      gestureSt.set(id);
      deviceEvStack.push_back().set(STD_GESTURE_DEVICE_ID, id, true, -1);
      majorActiveBitsReady = false;
    }
    break;

    case Gesture::Type::TAP:
    case Gesture::Type::DOUBLE_TAP:
    {
      addGestureEv();
      const int id = (g.type == Gesture::Type::TAP) ? EV_GESTURE_TAP : EV_GESTURE_DOUBLE_TAP;
      gestureSt.set(id);
      deviceEvStack.push_back().set(STD_GESTURE_DEVICE_ID, id, true, -1);
      majorActiveBitsReady = false;
    }
    break;

    case Gesture::Type::HOLD:
      if (s == Gesture::State::BEGIN)
      {
        addGestureEv();
        gestureSt.set(EV_GESTURE_HOLD);
        deviceEvStack.push_back().set(STD_GESTURE_DEVICE_ID, EV_GESTURE_HOLD, true, -1);
        majorActiveBitsReady = false;
      }
      else if (s == Gesture::State::END)
      {
        addGestureEv();
        gestureSt.clr(EV_GESTURE_HOLD);
        deviceEvStack.push_back().set(STD_GESTURE_DEVICE_ID, EV_GESTURE_HOLD, false, -1);
        majorActiveBitsReady = false;
      }

      break;

    default: break;
  }
}

void clear_shortcuts_state_buffer()
{
  mouseStBuf.clear();
  kbdStBuf.clear();
  joyStBuf.clear();
  gestureStBuf.clear();
  deviceEvStack.clear();
  memset(stateCounter, 0, sizeof(stateCounter));
  kbdStCtx = &kbdSt;
  mouseStCtx = &mouseSt;
  joyStCtx = &joySt;
  gestureStCtx = &gestureSt;
  majorActiveBitsReady = false;
  device_mask = 0xFF;
}
void clear_shortcuts_keyboard()
{
  if (kbdStCtx)
    kbdStCtx->clear();
}

void set_shortcut_device_mask(unsigned char mask) { device_mask = mask; }

bool is_button_pressed(UserInputDeviceId device_id, int button_id, bool use_device_mask)
{
  if (use_device_mask && !(device_mask & (1 << (int)device_id)))
    return false;
  if (device_id == STD_MOUSE_DEVICE_ID)
  {
    float mouseWheel = HumanInput::raw_state_pnt.mouse.deltaZ;
    if ((button_id == MOUSE_WHEEL_UP_BUTTON && mouseWheel > 0) || (button_id == MOUSE_WHEEL_DOWN_BUTTON && mouseWheel < 0))
      return true;
    return mouseStCtx->get(button_id);
  }
  if (device_id == STD_KEYBOARD_DEVICE_ID)
    return kbdStCtx->isKeyDown(button_id);
  if (device_id == DEF_JOYSTICK_DEVICE_ID)
    return joyStCtx->get(button_id);
  if (device_id == STD_GESTURE_DEVICE_ID)
    return gestureStCtx->get(button_id);

  return false;
}

static int compareButtonId(const void *ap, const void *bp)
{
  const ShortcutButtonId *a = (const ShortcutButtonId *)ap;
  const ShortcutButtonId *b = (const ShortcutButtonId *)bp;

  const int d = b->deviceId - a->deviceId;
  if (d)
    return d;
  return a->buttonId - b->buttonId;
}

static SmallTab<unsigned, InimemAlloc> majorActiveBits;
static Tab<unsigned short> shortcutsMajorTree(midmem);

class Shortcut
{
public:
  Shortcut() : refCount(0), groupMask(~0), combos(midmem) {}


  struct ButtonCombo
  {
    ShortcutButtonId buttons[MAX_SHORTCUT_BUTTONS];
    unsigned short majorStartIdx, majorCnt;

    ButtonCombo() : majorStartIdx(0), majorCnt(0) {}

    void sort(int num) { dag_qsort(&buttons[0], num, sizeof(buttons[0]), compareButtonId); }

    void getText(String &text)
    {
      for (int i = 0; i < MAX_SHORTCUT_BUTTONS; ++i)
      {
        if (buttons[i].deviceId == NULL_INPUT_DEVICE_ID)
          break;

        if (i > 0)
          text += "+";

        text += get_shortcut_button_name(buttons[i].deviceId, buttons[i].buttonId);
      }
    }

    void saveButton(DataBlock &blk, ShortcutButtonId &button)
    {
      blk.setInt("deviceId", button.deviceId);
      blk.setInt("buttonId", button.buttonId);
    }

    void save(DataBlock &blk)
    {
      for (int i = 0; i < MAX_SHORTCUT_BUTTONS; ++i)
      {
        if (buttons[i].deviceId == NULL_INPUT_DEVICE_ID)
          break;

        DataBlock &bblk = *blk.addNewBlock("button");
        G_ASSERT(&bblk);
        saveButton(bblk, buttons[i]);
      }
    }
    int btnCount()
    {
      for (int i = 0; i < MAX_SHORTCUT_BUTTONS; ++i)
        if (buttons[i].deviceId == NULL_INPUT_DEVICE_ID)
          return i;
      return MAX_SHORTCUT_BUTTONS;
    }

    bool isMajorOf(const ButtonCombo &c, int c_bcnt)
    {
      int my_btn_cnt = btnCount();
      if (my_btn_cnt <= c_bcnt)
        return false;
      for (int i = 0; i < c_bcnt; i++)
      {
        bool found = false;
        for (int j = 0; j < my_btn_cnt; j++)
          if (memcmp(&buttons[j], &c.buttons[i], sizeof(c.buttons[i])) == 0)
          {
            found = true;
            break;
          }
        if (!found)
          return false;
      }
      return true;
    }
  };

  static int compareButtonCombo(const ButtonCombo *a, const ButtonCombo *b)
  {
    int na, nb;
    for (na = 0; na < MAX_SHORTCUT_BUTTONS; ++na)
      if (a->buttons[na].deviceId == NULL_INPUT_DEVICE_ID)
        break;
    for (nb = 0; nb < MAX_SHORTCUT_BUTTONS; ++nb)
      if (b->buttons[nb].deviceId == NULL_INPUT_DEVICE_ID)
        break;

    return nb - na;
  }

  void clear()
  {
    eventId = EventClassId();
    clear_and_shrink(combos);
  }

  bool save(DataBlock &blk, int id)
  {
    if (!refCount && eventId.getId() == NULL_NAME_ID)
      return false;

    DataBlock &eblk = *blk.addNewBlock("event");
    G_ASSERT(&eblk);

    eblk.setStr("name", EventClassId(id));

    for (int i = 0; i < combos.size(); ++i)
    {
      DataBlock &cblk = *eblk.addNewBlock("shortcut");
      G_ASSERT(&cblk);

      combos[i].save(cblk);
    }

    return combos.size() != 0;
  }

  void load(const EventClassId &event_id, DataBlock &blk, int shortcut_name_id, int button_name_id)
  {
    eventId = event_id;

    const int num = blk.blockCount();
    for (int i = 0; i < num; ++i)
    {
      DataBlock *cblk = blk.getBlock(i);
      if (!cblk)
        continue;
      if (cblk->getBlockNameId() != shortcut_name_id)
        continue;

      ShortcutButtonId buttons[MAX_SHORTCUT_BUTTONS];
      int numButtons = 0;

      int numSub = cblk->blockCount();
      for (int j = 0; j < numSub && numButtons < MAX_SHORTCUT_BUTTONS; ++j)
      {
        DataBlock *bblk = cblk->getBlock(j);
        if (!bblk)
          continue;
        if (bblk->getBlockNameId() != button_name_id)
          continue;

        buttons[numButtons].deviceId = bblk->getInt("deviceId", NULL_INPUT_DEVICE_ID);
        buttons[numButtons].buttonId = bblk->getInt("buttonId", -1);

        if (buttons[numButtons].deviceId == NULL_INPUT_DEVICE_ID)
          continue;

        ++numButtons;
      }

      addCombo(buttons, numButtons, event_id);
    }
  }

  static bool validateButton(const ShortcutButtonId &button)
  {
    switch (button.deviceId)
    {
      case STD_KEYBOARD_DEVICE_ID: return (unsigned)button.buttonId < (unsigned)KeyboardState::SIZE_BITS;
      case STD_MOUSE_DEVICE_ID:
      case DEF_JOYSTICK_DEVICE_ID: return (unsigned)button.buttonId < (unsigned)HumanInput::ButtonBits::SZ;
      case STD_GESTURE_DEVICE_ID: return (unsigned)button.buttonId < countof(gesture_names);
    }
    return false; // bad device id
  }

  bool makeCombo(ShortcutButtonId *buttons, int num_buttons, ButtonCombo &cmb)
  {
    if (num_buttons <= 0)
      return false;

    if (num_buttons > MAX_SHORTCUT_BUTTONS)
      num_buttons = MAX_SHORTCUT_BUTTONS;

    int i, j;
    for (i = 0, j = 0; i < num_buttons; ++i)
    {
      if (buttons[i].deviceId == NULL_INPUT_DEVICE_ID)
        continue;
      if (!validateButton(buttons[i]))
      {
        debug("bad shortcut combo button %d:%d", buttons[i].deviceId, buttons[i].buttonId);
        G_ASSERTF(0, "bad shortcut combo button %d:%d", buttons[i].deviceId, buttons[i].buttonId);
        return false;
      }
      cmb.buttons[j++] = buttons[i];
    }

    int num = j;
    if (!num)
      return false;

    for (; j < MAX_SHORTCUT_BUTTONS; ++j)
    {
      cmb.buttons[j].deviceId = NULL_INPUT_DEVICE_ID;
      cmb.buttons[j].buttonId = 0;
    }

    cmb.sort(num);

    return true;
  }

  int findCombo(const ButtonCombo &cmb)
  {
    for (int i = 0; i < combos.size(); ++i)
      if (memcmp(&combos[i], &cmb, sizeof(cmb)) == 0)
        return i;
    return -1;
  }

  void sortCombos() { sort(combos, &compareButtonCombo); }

  void addCombo(ShortcutButtonId *buttons, int num_buttons, const EventClassId &event_id)
  {
    // if (!refCount && eventId.getId()==NULL_NAME_ID) return;

    ButtonCombo cmb;
    if (!makeCombo(buttons, num_buttons, cmb))
      return;

    int i = findCombo(cmb);
    if (i >= 0)
      return;

    combos.push_back(cmb);

    // sortCombos();
    /// ^ disabled - this confused Adrenalin control options dialog by
    /// mixing hotkeys when refreshing keylist after hotkeys changed

    eventId = event_id;
  }

  void removeCombo(ShortcutButtonId *buttons, int num_buttons, const EventClassId &event_id)
  {
    if (!refCount && eventId.getId() == NULL_NAME_ID)
      return;

    ButtonCombo cmb;
    if (!makeCombo(buttons, num_buttons, cmb))
      return;

    int i = findCombo(cmb);
    if (i < 0)
      return;

    erase_items(combos, i, 1);

    eventId = event_id;
  }

  void removeAllCombos(const EventClassId &event_id)
  {
    if (!refCount && eventId.getId() == NULL_NAME_ID)
      return;

    if (combos.size())
    {
      clear_and_shrink(combos);
      eventId = event_id;
    }
  }

  void getText(String &text)
  {
    text = "";

    for (int i = 0; i < combos.size(); ++i)
    {
      if (i > 0)
        text += ",  ";
      combos[i].getText(text);
    }
  }

  inline void addRef() { refCount++; }

  inline void delRef()
  {
    G_ASSERT(refCount > 0);
    refCount--;
  }

  bool isShortcutAllowed(const ButtonCombo &c)
  {
    //== hack to make shortcut like single Tab key not activated on Alt+Tab
    if (c.buttons[1].deviceId == NULL_INPUT_DEVICE_ID && c.buttons[0].deviceId == STD_KEYBOARD_DEVICE_ID &&
        (c.buttons[0].buttonId == HumanInput::DKEY_TAB || c.buttons[0].buttonId == HumanInput::DKEY_ESCAPE))
    {
      if (is_button_pressed(STD_KEYBOARD_DEVICE_ID, HumanInput::DKEY_LALT) ||
          is_button_pressed(STD_KEYBOARD_DEVICE_ID, HumanInput::DKEY_RALT))
        return false;
    }
    return true;
  }

  inline int isActive(bool use_device_mask = true)
  {
    if (!refCount)
      return -1;
    int i, j;

    for (i = 0; i < combos.size(); ++i)
    {
      ButtonCombo &c = combos[i];

      for (j = 0; j < MAX_SHORTCUT_BUTTONS; ++j)
      {
        ShortcutButtonId &btn = c.buttons[j];
        if (btn.deviceId == NULL_INPUT_DEVICE_ID)
        {
          j = MAX_SHORTCUT_BUTTONS;
          break;
        }
        if (!is_button_pressed(btn.deviceId, btn.buttonId, use_device_mask))
          break;
      }

      if (j >= MAX_SHORTCUT_BUTTONS)
      {
        if (!isShortcutAllowed(c))
          return -1;
        if (c.majorCnt)
        {
          for (int k = c.majorStartIdx, ke = k + c.majorCnt; k < ke; k++)
          {
            int idx = shortcutsMajorTree[k];
            if (majorActiveBits[idx >> 5] & (1 << (idx & 0x1F)))
              return -1;
          }
        }

        return i;
      }
    }

    return -1;
  }

  inline bool check(UserInputDeviceId device_id, int button_id)
  {
    if (!refCount)
      return false;
    int i, j;

    for (i = 0; i < combos.size(); ++i)
    {
      ButtonCombo &c = combos[i];

      for (j = 0; j < MAX_SHORTCUT_BUTTONS; ++j)
      {
        if (c.buttons[j].deviceId == NULL_INPUT_DEVICE_ID)
        {
          j = MAX_SHORTCUT_BUTTONS;
          break;
        }
        if (c.buttons[j].deviceId == device_id && c.buttons[j].buttonId == button_id)
          break;
      }

      if (j >= MAX_SHORTCUT_BUTTONS)
        continue;

      for (j = 0; j < MAX_SHORTCUT_BUTTONS; ++j)
      {
        ShortcutButtonId &btn = c.buttons[j];
        if (btn.deviceId == NULL_INPUT_DEVICE_ID)
        {
          j = MAX_SHORTCUT_BUTTONS;
          break;
        }

        if (btn.deviceId == device_id && btn.buttonId == button_id)
          continue;

        if (!is_button_pressed(btn.deviceId, btn.buttonId))
          break;
      }

      if (j >= MAX_SHORTCUT_BUTTONS)
      {
        // debug_ctx ( "%p.shortcut identified <%s>", this, eventId.getName());
        if (!isShortcutAllowed(c))
          return false;
        return true;
      }
    }

    return false;
  }

public:
  int refCount;
  uint32_t groupMask;

  Tab<ButtonCombo> combos;

  EventClassId eventId; // Set when combos are modified, to keep this event alive, to save combos later.
};


static Tab<Shortcut> shortcuts(midmem_ptr());
static bool shortcutsTreeReady = true;
static FastIntList majorShortcuts;
static Tab<unsigned short> tmpMajorLeaf(midmem);

static int findTmpDataStart()
{
  for (int i = 0, e = shortcutsMajorTree.size() - tmpMajorLeaf.size(); i <= e; i++)
    if (mem_eq(tmpMajorLeaf, shortcutsMajorTree.data() + i))
      return i;
  return append_items(shortcutsMajorTree, tmpMajorLeaf.size(), tmpMajorLeaf.data());
}

static void buildShortcutsTree()
{
  debug("%s", __FUNCTION__);
  shortcutsMajorTree.clear();
  majorShortcuts.reset();
  int major_sz = 0;
  for (int i = 0; i < shortcuts.size(); i++)
  {
    for (int j = 0; j < shortcuts[i].combos.size(); j++)
    {
      tmpMajorLeaf.clear();
      Shortcut::ButtonCombo &c = shortcuts[i].combos[j];
      int bcnt = c.btnCount();

      for (int k = 0; k < shortcuts.size(); k++)
      {
        if (k == i || ((shortcuts[i].groupMask & shortcuts[k].groupMask) == 0 &&
                        shortcuts[i].groupMask != (shortcuts[i].groupMask | shortcuts[k].groupMask)))
          continue;
        for (int l = 0; l < shortcuts[k].combos.size(); l++)
          if (shortcuts[k].combos[l].isMajorOf(c, bcnt))
          {
            tmpMajorLeaf.push_back((k << 4) + l);
            majorShortcuts.addInt(k);
            if (k >= major_sz)
              major_sz = k + 1;
          }
      }

      if (tmpMajorLeaf.size() == 0)
        c.majorStartIdx = c.majorCnt = 0;
      else
      {
        c.majorStartIdx = findTmpDataStart();
        c.majorCnt = tmpMajorLeaf.size();
      }
    }
  }
  clear_and_resize(majorActiveBits, ((major_sz << 4) + 31) / 32);
  mem_set_0(majorActiveBits);
  majorActiveBitsReady = false;

  shortcutsTreeReady = true;
  /*
    debug("shortcutsMajorTree.count=%d", shortcutsMajorTree.size());
    for (int i = 0; i < shortcutsMajorTree.size(); i ++)
      debug("shortcutsMajorTree %d: %x", i, shortcutsMajorTree[i]);
    dag::ConstSpan<int> lst = majorShortcuts.getList();
    for (int i = 0; i < lst.size(); i ++)
      debug("major %d: %d", i, lst[i]);
  */
}
static void fillmajorActiveBits()
{
  mem_set_0(majorActiveBits);
  for (int i = 0; i < majorShortcuts.getList().size(); i++)
  {
    int sid = majorShortcuts.getList()[i];
    Shortcut &sc = shortcuts[sid];
    int idx = sc.isActive();
    if (idx >= 0)
    {
      idx += (sid << 4);
      // debug("major %x", idx);
      majorActiveBits[idx >> 5] |= 1 << (idx & 0x1F);
    }
  }
  majorActiveBitsReady = true;
}

static void registerShortcut(const EventClassId &event_id)
{
  int id = event_id.getId();

  if (id < 0)
    return;

  if (id >= shortcuts.size())
    shortcuts.resize(id + 1);

  shortcuts[id].addRef();
  shortcutsTreeReady = false;
}


static void unregisterShortcut(const EventClassId &event_id)
{
  int id = event_id.getId();

  if (id < 0 || id >= shortcuts.size())
    return;

  shortcuts[id].delRef();
  shortcutsTreeReady = false;
}


static bool checkShortcut(const EventClassId &event_id, UserInputDeviceId device_id, int button_id)
{
  int id = event_id.getId();

  if (id < 0 || id >= shortcuts.size())
    return false;

  return shortcuts[id].check(device_id, button_id);
}


static void loadShortcut(const EventClassId &event_id, DataBlock &blk, int shortcut_name_id, int button_name_id)
{
  int id = event_id.getId();

  if (id < 0)
    return;

  if (id >= shortcuts.size())
    shortcuts.resize(id + 1);

  shortcuts[id].load(event_id, blk, shortcut_name_id, button_name_id);
  shortcutsTreeReady = false;
}


void add_shortcut(const EventClassId &shortcut_id, ShortcutButtonId *buttons, int num_buttons)
{
  int id = shortcut_id.getId();

  if (id < 0)
    return;

  if (id >= shortcuts.size())
    shortcuts.resize(id + 1);

  shortcuts[id].addCombo(buttons, num_buttons, shortcut_id);
  shortcutsTreeReady = false;
}


void remove_shortcut(const EventClassId &shortcut_id, ShortcutButtonId *buttons, int num_buttons)
{
  int id = shortcut_id.getId();

  if (id < 0 || id >= shortcuts.size())
    return;

  shortcuts[id].removeCombo(buttons, num_buttons, shortcut_id);
  shortcutsTreeReady = false;
}


void remove_shortcuts(const EventClassId &shortcut_id)
{
  int id = shortcut_id.getId();

  if (id < 0 || id >= shortcuts.size())
    return;

  shortcuts[id].removeAllCombos(shortcut_id);
  shortcutsTreeReady = false;
}

void get_shortcut_buttons(const EventClassId &shortcut_id, int combo_index, ShortcutButtonId *buttons, int &num_buttons,
  bool *joy_combo /*= NULL*/)
{
  num_buttons = 0;

  G_ASSERT(combo_index >= 0 && combo_index < shortcuts[shortcut_id.getId()].combos.size());

  bool isJoyCombo = false;
  Shortcut::ButtonCombo &combo = shortcuts[shortcut_id.getId()].combos[combo_index];
  while ((num_buttons < MAX_SHORTCUT_BUTTONS) && (combo.buttons[num_buttons].deviceId != NULL_INPUT_DEVICE_ID))
  {
    buttons[num_buttons] = combo.buttons[num_buttons];
    if (buttons[num_buttons].deviceId >= DEF_JOYSTICK_DEVICE_ID)
      isJoyCombo = true;
    num_buttons++;
  }
  if (joy_combo)
    *joy_combo = isJoyCombo;
}

void get_shortcut_buttons_names(const EventClassId &shortcut_id, int combo_index, String &text)
{
  if (combo_index >= 0 && combo_index < shortcuts[shortcut_id.getId()].combos.size())
    shortcuts[shortcut_id.getId()].combos[combo_index].getText(text);
  else
    text = "";
}

int get_shortcut_combos_count(const EventClassId &shortcut_id)
{
  int id = shortcut_id.getId();
  if ((unsigned)id >= (unsigned)shortcuts.size())
    return 0;
  return shortcuts[id].combos.size();
}

void get_shortcuts_text(const EventClassId &shortcut_id, String &text)
{
  int id = shortcut_id.getId();

  if (id < 0 || id >= shortcuts.size())
  {
    text = "";
    return;
  }

  shortcuts[id].getText(text);
}

void set_shortcut_group_mask(const EventClassId &shortcut_id, uint32_t group_mask)
{
  int id = shortcut_id.getId();

  if (id < 0)
    return;

  if (id >= shortcuts.size())
    shortcuts.resize(id + 1);

  shortcuts[id].groupMask = group_mask;
  shortcutsTreeReady = false;
}


bool save_shortcuts(DataBlock &blk)
{
  bool saved = false;

  for (int i = 0; i < shortcuts.size(); ++i)
    if (shortcuts[i].save(blk, i))
      saved = true;

  return saved;
}


void load_shortcuts(DataBlock &blk)
{
  int i;
  for (i = 0; i < shortcuts.size(); ++i)
    shortcuts[i].clear();

  int eventNameId = blk.getNameId("event");
  int shortcutNameId = blk.getNameId("shortcut");
  int buttonNameId = blk.getNameId("button");

  int num = blk.blockCount();
  for (i = 0; i < num; ++i)
  {
    DataBlock *eblk = blk.getBlock(i);
    if (!eblk)
      continue;
    if (eblk->getBlockNameId() != eventNameId)
      continue;

    const char *name = eblk->getStr("name", NULL);
    if (!name)
      continue;

    EventClassId id(name);

    ::loadShortcut(id, *eblk, shortcutNameId, buttonNameId);
  }
}


class ShortcutGroup
{
public:
  ShortcutGroup(const char *group_name, const EventClassId *init_events, int num_events) : name(NULL), events(midmem_ptr())
  {
    name = str_dup(group_name, midmem);

    events.reserve(num_events);

    int i;
    for (i = 0; i < num_events; ++i)
    {
      if (init_events[i].getId() == NULL_NAME_ID)
        continue;
      events.push_back(init_events[i]);
    }

    events.shrink_to_fit();

    for (i = 0; i < events.size(); ++i)
      ::registerShortcut(events[i]);
  }

  ~ShortcutGroup()
  {
    for (int i = 0; i < events.size(); ++i)
      ::unregisterShortcut(events[i]);

    memfree(name, midmem);
  }

  EventClassId check(UserInputDeviceId device_id, int button_id)
  {
    for (int i = 0; i < events.size(); ++i)
      if (::checkShortcut(events[i], device_id, button_id))
        return events[i];

    return EventClassId();
  }

public:
  DAG_DECLARE_NEW(midmem)

  char *name;

  Tab<EventClassId> events;
};


static Tab<ShortcutGroup *> groups(inimem_ptr()), stack(inimem_ptr());


ShortcutGroup *create_shortcut_group(const char *group_name, const EventClassId *events, int num_events)
{
  G_ASSERT(group_name && *group_name);

  ShortcutGroup *grp = get_shortcut_group(group_name);
  if (grp)
    DAG_FATAL("Shortcut group '%s' is already registered!", group_name);

  G_ASSERT(!(num_events && !events));

  grp = new ShortcutGroup(group_name, events, num_events);

  groups.push_back(grp);

  return grp;
}


void release_shortcut_group(ShortcutGroup *grp)
{
  if (!grp)
    return;

  deactivate_shortcut_group(grp);

  for (int i = 0; i < groups.size(); ++i)
    if (groups[i] == grp)
    {
      erase_items(groups, i, 1);
      break;
    }

  delete grp;
}


ShortcutGroup *get_shortcut_group(const char *group_name)
{
  if (!group_name)
    return NULL;

  for (int i = 0; i < groups.size(); ++i)
    if (groups[i] && strcmp(groups[i]->name, group_name) == 0)
      return groups[i];

  return NULL;
}


bool is_shortcut_active(const EventClassId &shortcut_id, bool use_device_mask /*= true*/)
{
  int id = shortcut_id.getId();

  if (id < 0 || id >= shortcuts.size())
    return false;
  if (!shortcutsTreeReady)
    buildShortcutsTree();
  if (!majorActiveBitsReady)
    fillmajorActiveBits();

  return shortcuts[id].isActive(use_device_mask) != -1;
}


int groupsNumber() { return groups.size(); }

int getGroupId(const char *name)
{
  for (int i = 0; i < groups.size(); i++)
    if (strcmp(groups[i]->name, name) == 0)
      return i;
  return -1;
}

char *groupName(int n) { return groups[n]->name; }

int shortcutsNumber(int group) { return groups[group]->events.size(); }

const char *shortcutName(int group, int n) { return groups[group]->events[n]; }

int buttonsNumber(int group, int shortcut) { return shortcuts[groups[group]->events[shortcut].getId()].combos.size(); }

String *buttonName(int group, int shortcut, int n)
{
  String *text;
  text = new String;
  shortcuts[groups[group]->events[shortcut].getId()].combos[n].getText(*text);
  return text;
}

void removeButton(int group, int shortcut, int n)
{
  if (n == -1)
    return;
  erase_items(shortcuts[groups[group]->events[shortcut].getId()].combos, n, 1);
  shortcutsTreeReady = false;
}

void removeAllButtons(int group, int shortcut)
{
  shortcuts[groups[group]->events[shortcut].getId()].combos.clear();
  shortcutsTreeReady = false;
}

void activate_shortcut_group(ShortcutGroup *grp)
{
  if (!grp)
    return;

  deactivate_shortcut_group(grp);

  stack.push_back(grp);
}

void deactivate_shortcut_group(ShortcutGroup *grp)
{
  if (!grp)
    return;

  for (int i = stack.size() - 1; i >= 0; --i)
    if (stack[i] == grp)
      erase_items(stack, i, 1);
}

static EventClassId check_shortcut(UserInputDeviceId device_id, int button_id, bool down)
{
  if (down || (device_id == NULL_INPUT_DEVICE_ID))
    return EventClassId();

  for (int i = stack.size() - 1; i >= 0; --i)
  {
    const EventClassId &id = stack[i]->check(device_id, button_id);
    if (id.getId() != NULL_NAME_ID)
      return id;
  }

  return EventClassId();
}

static bool can_auto_clear = false;
static void auto_clear_shortcuts_state_buffer()
{
  stop_momentary_gestures(); // each frame at least

  if (can_auto_clear)
  {
    clear_shortcuts_state_buffer();
    can_auto_clear = false;
  }
}

EventClassId check_shortcut()
{
  const int devCnt = deviceEvStack.size();

  kbdStCtx = kbdStBuf.size() ? &kbdStBuf[0] : &kbdSt;
  mouseStCtx = mouseStBuf.size() ? &mouseStBuf[0] : &mouseSt;
  joyStCtx = joyStBuf.size() ? &joyStBuf[0] : &joySt;
  gestureStCtx = gestureStBuf.size() ? &gestureStBuf[0] : &gestureSt;

  can_auto_clear = true;
  for (int i = 0; i < devCnt; i++)
  {
    UserInputDeviceId device;
    int btnIdx = -1, sidx;
    const bool down = deviceEvStack[i].get(device, btnIdx, sidx);

    switch (device)
    {
      case STD_KEYBOARD_DEVICE_ID: kbdStCtx = (sidx >= 0 && sidx < kbdStBuf.size()) ? &kbdStBuf[sidx] : &kbdSt; break;
      case STD_MOUSE_DEVICE_ID: mouseStCtx = (sidx >= 0 && sidx < mouseStBuf.size()) ? &mouseStBuf[sidx] : &mouseSt; break;
      case DEF_JOYSTICK_DEVICE_ID: joyStCtx = (sidx >= 0 && sidx < joyStBuf.size()) ? &joyStBuf[sidx] : &joySt; break;
      case STD_GESTURE_DEVICE_ID: gestureStCtx = (sidx >= 0 && sidx < gestureStBuf.size()) ? &gestureStBuf[sidx] : &gestureSt; break;
      default: G_ASSERT(0 && "not supported device");
    }

    const EventClassId id = check_shortcut(device, btnIdx, down);
    if (id.getId() != NULL_NAME_ID)
    {
      kbdStCtx = &kbdSt;
      mouseStCtx = &mouseSt;
      joyStCtx = &joySt;
      gestureStCtx = &gestureSt;
      return id;
    }
  }

  kbdStCtx = &kbdSt;
  mouseStCtx = &mouseSt;
  joyStCtx = &joySt;
  gestureStCtx = &gestureSt;
  return EventClassId();
}

void init_shortcuts()
{
  clear_shortcuts_state_buffer();
  kbdSt.clear();
  mouseSt.reset();
  joySt.reset();
  gestureSt.reset();
  ::dgs_on_dagor_cycle_start = auto_clear_shortcuts_state_buffer;
}

void shutdown_shortcuts()
{
  for (auto group : groups)
    release_shortcut_group(group);

  for (auto &shortcut : shortcuts)
    shortcut.clear();

  EventClassId::shutdown();
}
