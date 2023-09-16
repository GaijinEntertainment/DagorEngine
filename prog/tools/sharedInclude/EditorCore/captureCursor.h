#ifndef __CAPTURE_CURSOR_H
#define __CAPTURE_CURSOR_H
#pragma once

struct EcRect;

// global functions
/// @addtogroup EditorCore
//@{
/// Capture mouse cursor and hold it inside main Editor window.
void capture_cursor(void *handle);

/// Release mouse cursor captured by capture_cursor().
void release_cursor();

/// Wrap mouse cursor.
/// The function wraps mouse cursor to opposite side of Editor's main window
/// when it reaches window border and continues moving.
/// @param[in] p1,p2 - initial x,y coordinates
/// @param[out] p1,p2 - final (wraped) x,y coordinates
void cursor_wrap(int &p1, int &p2, void *handle = NULL);

/// Wrap mouse cursor.
/// The function wraps mouse cursor to opposite side of specified area
/// when it reaches area border and continues moving.
/// @param[in] rc - area coordinates
/// @param[in] p1,p2 - initial x,y coordinates
/// @param[out] p1,p2 - final (wraped) x,y coordinates
/// @param[out] wrapedX,wrapedY - dX,dY increment storage. Used for accumulating
///  cursor increments and creating objects with sizes greater than window size.
void area_cursor_wrap(const EcRect &rc, int &p1, int &p2, int &wrapedX, int &wrapedY);
//@}

#endif
