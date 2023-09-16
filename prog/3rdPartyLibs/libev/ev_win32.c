/*
 * libev win32 compatibility cruft (_not_ a backend)
 *
 * Copyright (c) 2007,2008,2009 Marc Alexander Lehmann <libev@schmorp.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modifica-
 * tion, are permitted provided that the following conditions are met:
 *
 *   1.  Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *   2.  Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MER-
 * CHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPE-
 * CIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTH-
 * ERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License ("GPL") version 2 or any later version,
 * in which case the provisions of the GPL are applicable instead of
 * the above. If you wish to allow the use of your version of this file
 * only under the terms of the GPL and not to allow others to use your
 * version of this file under the BSD license, indicate your decision
 * by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL. If you do not delete the
 * provisions above, a recipient may use your version of this file under
 * either the BSD or the GPL.
 */

#ifdef _WIN32
#include "ev_pipe_emul.c"

/* timeb.h is actually xsi legacy functionality */
#include <sys/timeb.h>

/* note: the comment below could not be substantiated, but what would I care */
/* MSDN says this is required to handle SIGFPE */
/* my wild guess would be that using something floating-pointy is required */
/* for the crt to do something about it */
volatile double SIGFPE_REQ = 0.0f;

#undef pipe
#define pipe(filedes) ev_pipe (filedes)

#define EV_HAVE_EV_TIME 1
ev_tstamp
ev_time (void)
{
  FILETIME ft;
  ULARGE_INTEGER ui;

  GetSystemTimeAsFileTime (&ft);
  ui.u.LowPart  = ft.dwLowDateTime;
  ui.u.HighPart = ft.dwHighDateTime;

  /* msvc cannot convert ulonglong to double... yes, it is that sucky */
  return (LONGLONG)(ui.QuadPart - 116444736000000000) * 1e-7;
}

#endif

