// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// temporarily disabled due to visible artifacts
#if defined(ENABLE_LAYOUT) && defined(MAX_RELEASE_R22) && MAX_RELEASE >= MAX_RELEASE_R22

#include <vector>
#include <max.h>

struct DialogLayout;

void attach_layout_to_rollup(HWND hWnd, LPCWSTR dialog_name);
void attach_layout_to_dialog(HWND hWnd, LPCWSTR dialog_name);
void update_layout(HWND hWnd, LPARAM lParam);

#else

#define attach_layout_to_rollup(hWnd, dialog_name)
#define attach_layout_to_dialog(hWnd, dialog_name)
#define update_layout(hWnd, lParam) ;

#endif

#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

#define LAYOUT_MESSAGE (WM_USER)
