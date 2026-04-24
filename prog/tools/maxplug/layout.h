// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// temporarily disabled due to visible artifacts

#include <vector>
#include <max.h>

struct DialogLayout;

void attach_layout_to_rollup(HWND hWnd, LPCWSTR dialog_name);
void attach_layout_to_dialog(HWND hWnd, LPCWSTR dialog_name);
void update_layout(HWND hWnd, LPARAM lParam);

#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

#define LAYOUT_MESSAGE (WM_USER)
