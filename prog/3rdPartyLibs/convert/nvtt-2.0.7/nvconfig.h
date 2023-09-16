#ifndef NV_CONFIG
#define NV_CONFIG

//#define HAVE_UNISTD_H
#define HAVE_STDARG_H
//#define HAVE_SIGNAL_H
//#define HAVE_EXECINFO_H
#define HAVE_MALLOC_H

#if !defined(_M_X64)
//#define HAVE_PNG
#define HAVE_JPEG
//#define HAVE_TIFF
#endif

#define NV_NO_ASSERT 1

#endif // NV_CONFIG
