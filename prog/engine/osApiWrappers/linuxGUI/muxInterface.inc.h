// Copyright (C) Gaijin Games KFT.  All rights reserved.

bool init();
void shutdown();

bool changeGamma(float value);
void getDisplaySize(int &width, int &height, bool for_primary_output = false);
void getVideoModeList(Tab<String> &list);
void *getMainWindowPtrHandle() const;
bool isMainWindow(void *wnd) const;
void destroyMainWindow();

bool initWindow(const char *title, int winWidth, int winHeight);
void getWindowPosition(void *w, int &cx, int &cy);
void setTitle(const char *title, const char *tooltip = NULL);
void setTitleUTF8(const char *title, const char *tooltip = NULL);

int getScreenRefreshRate();
void setFullscreenMode(bool enable);

void processMessages();
bool getWindowScreenRect(void *w, linux_GUI::RECT *rect, linux_GUI::RECT *rect_unclipped);
bool getLastCursorPos(int *cx, int *cy, void *w);
void setCursor(void *w, const char *cursor_name);
void setCursorPosition(int cx, int cy, void *w);
void getAbsoluteCursorPosition(int &cx, int &cy);
bool getCursorDelta(int &cx, int &cy, void *w);
void clipCursor();
void unclipCursor();
void hideCursor(bool hide);
void *getNativeDisplay();
void *getNativeWindow(void *w);
bool getClipboardUTF8Text(char *dest, int buf_size);
bool setClipboardUTF8Text(const char *text);