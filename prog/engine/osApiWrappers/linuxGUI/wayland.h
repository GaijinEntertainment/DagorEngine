// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <osApiWrappers/dag_linuxGUI.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>

// avoid including lib headers in outer modules via forward declarations
struct wl_display;
struct wl_registry;
struct wl_compositor;
struct wl_data_device_manager;
struct wl_output;
struct wl_interface;
struct wl_seat;
struct wl_data_device;
struct wl_data_source;
struct wl_data_offer;
struct wl_keyboard;
struct wl_array;
struct wl_pointer;
struct wl_surface;
struct xdg_wm_base;
struct xdg_surface;
struct xdg_toplevel;
struct wp_viewporter;
struct wp_viewport;
struct zwp_relative_pointer_manager_v1;
struct zwp_pointer_constraints_v1;
struct zwp_relative_pointer_v1;
struct zwp_confined_pointer_v1;
struct zwp_locked_pointer_v1;
struct xkb_keymap;
struct xkb_state;
struct xkb_context;

typedef int32_t wl_fixed_t_proxy;

struct Wayland
{
#include "muxInterface.inc.h"

  void *libHandle;
  bool loadLib();
  void unloadLib();

  template <typename DataType, typename ObjType, typename ParentType, int MAX_OBJS>
  struct BlockPool
  {
    static_assert(MAX_OBJS < 64);
    DataType arr[MAX_OBJS];
    uint64_t freeBits = (1ull << MAX_OBJS) - 1;

    DataType *add(ObjType obj, ParentType *parent)
    {
      uint64_t idx = 0;
      for (uint64_t freeBitsItr = freeBits; freeBitsItr != 0; freeBitsItr >>= 1, ++idx)
      {
        if ((freeBitsItr & 1) == 0)
          continue;
        arr[idx].obj = obj;
        arr[idx].parent = parent;
        freeBits &= ~(1ull << idx);
        return &arr[idx];
      }
      return nullptr;
    }

    void remove(DataType *data)
    {
      uint32_t idx = toIndex(data);
      G_ASSERT(idx < MAX_OBJS);
      *data = {};
      freeBits |= 1ull << idx;
    }

    void removeByObj(ObjType obj)
    {
      for (DataType &i : arr)
        if (i.obj == obj)
        {
          i = {};
          freeBits |= 1ull << (&i - &arr[0]);
          break;
        }
    }

    bool isEmpty() { return freeBits == ((1ull << MAX_OBJS) - 1); }

    bool full() { return freeBits == 0; }

    void reset()
    {
      freeBits = (1ull << MAX_OBJS) - 1;
      for (DataType &i : arr)
        i = {};
    }

    DataType *find(ObjType obj)
    {
      for (DataType &i : arr)
        if (i.obj == obj)
          return &i;
      return nullptr;
    }

    uint32_t toIndex(DataType *data) { return data - &arr[0]; }

    bool isValid(uint32_t index) { return ((1ull << index) & freeBits) == 0; }

    uint64_t validMask() { return ~freeBits & ((1ull << MAX_OBJS) - 1); }
  };


  struct Output
  {
    wl_output *obj;
    Wayland *parent;

    uint32_t idx;
    int32_t x;
    int32_t y;
    int32_t physical_width;
    int32_t physical_height;
    int32_t scaleFactor;
    int32_t subpixel;
    String manufacturer;
    String model;
    String name;
    String desc;
    int32_t transform;

    struct Mode
    {
      int32_t width;
      int32_t height;
      int32_t refresh;
    };
    struct
    {
      Mode current;
      Mode preferred;
    } mode;

    void dumpToLog();
    void destroy();
  };

  struct InputGlobal
  {
    uint32_t name;
    const char *interface;
    uint32_t version;
  };

  struct GlobalRequest
  {
    const wl_interface &interface;
    uint32_t version;
    void **dst;
  };
  dag::Vector<void *> globalsCache;
  bool bindGlobal(const InputGlobal &in, GlobalRequest rq);

  // wl callbacks
  void cbRegistryGlobal(const InputGlobal &in);
  void cbRegistryGlobalRemove(uint32_t name);

  enum RegistryMinVersions
  {
    COMPOSITOR = 4,
    XDG_WM_BASE = 4,
    OUTPUT = 4,
    SEAT = 8,
    VIEWPORTER = 1,
    RELATIVE_POINTER = 1,
    POINTER_CONSTRAINTS = 1,
    DATA_DEVICE_MANAGER = 3,
  };

  BlockPool<Output, wl_output *, Wayland, 16> outputs;
  Output *getDefaultOutput();

  struct Pointer;
  struct Keyboard;

  struct Window
  {
    wl_surface *obj;
    Wayland *parent;
    Pointer *activePointer;
    Keyboard *activeKeyboard;

    bool fullscreen;
    bool outputsChanged;
    Output *fullscreenOnOutput;
    uint32_t onOutputs;
    int32_t boundsWidth;
    int32_t boundsHeight;
    int32_t reportedWidth;
    int32_t reportedHeight;

    struct
    {
      xdg_surface *surface;
      xdg_toplevel *toplevel;
    } xdg;

    struct
    {
      wp_viewport *viewport;
    } wp;

    void cbClose();
    void destroy();
    void changeFullscreen(bool enable);
    void resetupFullscreen();
    void processChangedOutputs();
    Output *getFirstOutput();
    void cbEnterOutput(Output *output);
    void cbLeaveOutput(Output *output);
    void cbConfigure();
  };

  BlockPool<Window, wl_surface *, Wayland, 16> windows;
  Window *mainWindow;
  Window *setupWindow();

  struct Seat
  {
    wl_seat *obj;
    Wayland *parent;
    String name;
    wl_data_device *dataDevice;
    wl_data_source *outboundClipboard;
    uint32_t lastKbSerial;
    String outboundClipboardText;
    String lastOfferMimeTypes;
    wl_data_offer *lastOffer;
    struct InboundTransfer
    {
      String text;
      wl_data_offer *offer;
      int pipe[2];
      bool completed;

      void recive(wl_data_offer *from);
      void cleanup();
      void process();

      ~InboundTransfer() { cleanup(); }
    } inbound;

    bool setClipboardUTF8Text(const char *text);
    bool getClipboardUTF8Text(char *dest, int buf_size);

    void cbSeatCaps(uint32_t capabilities);
    void cbSendData(const char *mime_type, int32_t fd);
    void cbSelection(wl_data_offer *offer);
    void cbOfferType(wl_data_offer *offer, const char *mime);
    void cbNewOffer(wl_data_offer *offer);
  };
  BlockPool<Seat, wl_seat *, Wayland, 4> seats;

  struct Pointer
  {
    wl_pointer *obj;
    Seat *parent;

    wl_fixed_t_proxy x;
    wl_fixed_t_proxy y;
    uint32_t buttonsPressBits;
    wl_fixed_t_proxy dltX;
    wl_fixed_t_proxy dltY;
    Window *onWindow;
    uint32_t enterSerial;
    bool hidden;
    bool posChanged;
    bool clip;
    bool clipChanged;
    bool validDeltas;

    struct
    {
      bool dirty;
      uint32_t source;
      uint32_t axis;
      wl_fixed_t_proxy value;
    } axis;

    struct
    {
      zwp_relative_pointer_v1 *relativePointerV1;
      zwp_confined_pointer_v1 *confinedPointerV1;
      zwp_locked_pointer_v1 *lockedPointerV1;
    } zwp;

    void resetupHideState();
    void resetupClipState();

    void cbSetPointerPos(wl_fixed_t_proxy x, wl_fixed_t_proxy y);
    void cbSetPointerButton(uint32_t button, uint32_t state);
    void cbFrame();
    void cbPointerFocus(wl_surface *surf, wl_fixed_t_proxy x, wl_fixed_t_proxy y, bool enter, uint32_t serial);
    void cbAxis(uint32_t axis, wl_fixed_t_proxy value);
    void cbAxisSrc(uint32_t axis_source);
    void cbRelativeMotion(wl_fixed_t_proxy x, wl_fixed_t_proxy y);
  };
  BlockPool<Pointer, wl_pointer *, Seat, 4> pointers;

  struct Keyboard
  {
    wl_keyboard *obj;
    Seat *parent;

    Window *focusedOn;
    xkb_keymap *keymap;
    xkb_state *xkbState;
    int64_t repeatDelayTicks;
    int64_t repeatAfterTicks;

    struct KeyRepeatTrigger
    {
      uint32_t key;
      int64_t repeatAfterTicks;
    };
    dag::Vector<KeyRepeatTrigger> repeatStack;
    void trackRepeat(bool pressed, uint32_t key);
    void processRepeats();

    void sendKeyMsg(bool pressed, uint32_t keycode);

    void cbKeymap(uint32_t format, int32_t fd, uint32_t size);
    void cbKey(uint32_t key, uint32_t state, uint32_t time_ms, uint32_t serial);
    void cbEnter(wl_surface *surface, wl_array *keys, uint32_t serial);
    void cbLeave(wl_surface *surface);
    void cbModifier(uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group);
    void cbRepeatInfo(uint32_t delay_ms, uint32_t rate_per_second);
  };
  BlockPool<Keyboard, wl_keyboard *, Seat, 4> keyboards;

  xkb_context *xkbCtx;

  struct
  {
    wl_display *display;
    wl_registry *registry;
    wl_compositor *compositor;
    wl_data_device_manager *dataDeviceManager;
  } wl;

  struct
  {
    wp_viewporter *viewporter;
  } wp;

  struct
  {
    xdg_wm_base *wmBase;
  } xdg;

  struct
  {
    zwp_relative_pointer_manager_v1 *relativePointerManagerV1;
    zwp_pointer_constraints_v1 *pointerConstraintsV1;
  } zwp;
};
