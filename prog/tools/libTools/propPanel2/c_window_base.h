#pragma once

#define _WIN32_WINNT 0x0600

#include <generic/dag_tab.h>
#include <propPanel2/c_common.h>
#include <libTools/util/hdpiUtil.h>

using hdpi::_pxActual;
using hdpi::_pxS;
using hdpi::_pxScaled;

const char PROPERTY_WINDOW_CLASS_NAME[] = "PropertyWindowClass";
const char CONTAINER_WINDOW_CLASS_NAME[] = "ContainerWindowClass";

// with user colors
const char USER_PROPERTY_WINDOW_CLASS_NAME[] = "UserPropertyWindowClass";
const char USER_CONTAINER_WINDOW_CLASS_NAME[] = "UserContainerWindowClass";


void debug_int(int value);
void debug_string(const char value[]);

#define USER_GUI_COLOR RGB(197, 197, 197)

enum
{
  SCROLL_MOUSE_JUMP = 200,
};

class DataBlock;
class String;

class WindowBase
{
  friend class WindowBaseHandler;

public:
  WindowBase(WindowBase *parent, const char class_name[], unsigned long ex_style, unsigned long style, const char caption[], int x,
    int y, int w, int h);

  virtual ~WindowBase();

  virtual void show();
  virtual void hide();
  virtual void setEnabled(bool enabled);

  void refresh(bool all = false);
  void resizeWindow(unsigned w, unsigned h);
  void moveWindow(int x, int y);
  void updateWindow();
  void getClientSize(int &cw, int &ch);
  void setZAfter(void *handle);
  void setFont(TFontHandle font);
  bool isActive();
  void setFocus();
  void setDragAcceptFiles(bool accept = true);

  void *getHandle() const;
  void *getParentHandle() const;
  TInstHandle getModule() const;
  unsigned getID() const;
  void *getRootWindowHandle() const { return mRootWindowHandle; }

  virtual void setTextValue(const char value[]);
  virtual int getTextValue(char *buffer, int buflen) const;
  bool getUserColorsFlag() { return mUserGuiColor; }
  bool skipDialogProc() { return noDialogProc; }

  unsigned getWidth() { return mWidth; }
  unsigned getHeight() { return mHeight; }
  int getX() { return mX; }
  int getY() { return mY; }

  long copyToClipboard();
  long pasteFromClipboard();

  virtual int getIndex() const { return 0; } // for radio groups

  // If an application processes a message, it should return non zero.
  // return 0 -> the deffault message proc will be called.
  virtual intptr_t onControlNotify(void *info);
  virtual intptr_t onControlDrawItem(void *info);
  virtual intptr_t onControlCommand(unsigned notify_code, unsigned elem_id);
  virtual intptr_t onResize(unsigned new_width, unsigned new_height);
  virtual intptr_t onDrag(int new_x, int new_y);
  virtual intptr_t onMouseMove(int x, int y);
  virtual intptr_t onLButtonUp(long x, long y);
  virtual intptr_t onLButtonDown(long x, long y);
  virtual intptr_t onLButtonDClick(long x, long y);
  virtual intptr_t onRButtonUp(long x, long y);
  virtual intptr_t onRButtonDown(long x, long y);
  virtual intptr_t onButtonStaticDraw(void *hdc);
  virtual intptr_t onButtonDraw(void *hdc);
  virtual intptr_t onDrawEdit(void *hdc);
  virtual intptr_t onKeyDown(unsigned v_key, int flags);
  virtual intptr_t onVScroll(int dy, bool is_wheel);
  virtual intptr_t onChar(unsigned code, int flags);
  virtual intptr_t onHScroll(int dx);
  virtual intptr_t onCaptureChanged(void *window_gaining_capture);
  virtual void onTabFocusChange(){};
  virtual bool onClose();
  virtual void onPostEvent(unsigned id){};
  virtual void onPaint(){};
  virtual void onKillFocus(){};
  virtual intptr_t onClipboardCopy(DataBlock &blk);
  virtual intptr_t onClipboardPaste(const DataBlock &blk);
  virtual intptr_t onDropFiles(const dag::Vector<String> &files);

  static TFontHandle getNormalFont();
  static TFontHandle getBoldFont();
  static TFontHandle getComboFont();
  static TFontHandle getSmallPrettyFont();
  static TFontHandle getSmallButtonFont();
  static TFontHandle getSmallPlotFont();

protected:
  unsigned getNextChildID();

  unsigned mID;
  unsigned mIDCounter;
  WindowBase *mParent;
  void *mHandle;
  TInstHandle mHInstance;
  intptr_t mWndProc;

  unsigned mWidth, mHeight;
  int mX, mY;
  int mScrollVPos, mScrollHPos;
  bool mUserGuiColor;
  bool noDialogProc;
  bool scrollParentFirst;

private:
  static TFontHandle mFNH, mFBH, mFSPH, mFCH, mFSBH, mFSPlH;
  void *mRootWindowHandle;
};
