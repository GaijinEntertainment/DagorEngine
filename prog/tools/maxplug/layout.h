// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <vector>
#include <max.h>

struct DialogLayout;

void attach_layout_to_dialog(HWND hWnd, LPCWSTR dialog_name);
void detach_layout_from_dialog(HWND hWnd);
void update_layout(HWND hWnd, LPARAM lParam);

#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

#define LAYOUT_MESSAGE (WM_USER)
