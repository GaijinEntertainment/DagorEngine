/* Define to 1 if you have the declaration of `tzname', and to 0 if you don't. */
#define HAVE_DECL_TZNAME  1

/* Define to 1 if you have the <dlfcn.h> header file. */
#undef HAVE_DLFCN_H

/* Define to 1 if you have the `fseeko' function. */
#undef HAVE_FSEEKO

/* Define to 1 if you have the `ftello' function. */
#undef HAVE_FTELLO

/* Define to 1 if you have the `getopt' function. */
#undef HAVE_GETOPT

/* Define to 1 if the system has the type `int16_t'. */
#undef HAVE_INT16_T

/* Define to 1 if the system has the type `int32_t'. */
#undef HAVE_INT32_T

/* Define to 1 if the system has the type `int64_t'. */
#undef HAVE_INT64_T

/* Define to 1 if the system has the type `int8_t'. */
#undef HAVE_INT8_T

/* Define to 1 if you have the <inttypes.h> header file. */
#undef HAVE_INTTYPES_H

/* Define to 1 if you have the `z' library (-lz). */
#define HAVE_LIBZ 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mkstemp' function. */
#undef HAVE_MKSTEMP

/* Define to 1 if you have the `MoveFileExA' function. */
#define HAVE_MOVEFILEEXA  1

/* Define to 1 if you have the `open' function. */
#define HAVE_OPEN 1

/* Define to 1 if you have the `snprintf' function. */
#undef HAVE_SNPRINTF

/* Define to 1 if you have the <stdint.h> header file. */
#undef HAVE_STDINT_H

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#undef HAVE_STRCASECMP

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the <strings.h> header file. */
#undef HAVE_STRINGS_H

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if `tm_zone' is a member of `struct tm'. */
#undef HAVE_STRUCT_TM_TM_ZONE

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H  1

/* Define to 1 if your `struct tm' has `tm_zone'. Deprecated, use
   `HAVE_STRUCT_TM_TM_ZONE' instead. */
#undef HAVE_TM_ZONE

/* Define to 1 if you don't have `tm_zone' but do have the external array
   `tzname'. */
#define HAVE_TZNAME 1

/* Define to 1 if the system has the type `uint16_t'. */
#undef HAVE_UINT16_T

/* Define to 1 if the system has the type `uint32_t'. */
#undef HAVE_UINT32_T

/* Define to 1 if the system has the type `uint64_t'. */
#undef HAVE_UINT64_T

/* Define to 1 if the system has the type `uint8_t'. */
#undef HAVE_UINT8_T

/* Define to 1 if you have the <unistd.h> header file. */
#undef HAVE_UNISTD_H

/* Define to 1 if you have the `_open' function. */
#define HAVE__OPEN  1

/* Define to 1 if you have the `_snprintf' function. */
#define HAVE__SNPRINTF  1

/* Define to 1 if you have the `_strcmpi' function. */
#define HAVE__STRCMPI 1

/* Define to 1 if you have the `_strdup' function. */
#define HAVE__STRDUP  1

/* Define to 1 if you have the `_stricmp' function. */
#define HAVE__STRICMP 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#undef LT_OBJDIR

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
#undef NO_MINUS_C_MINUS_O

/* Name of package */
#undef PACKAGE

/* Define to the address where bug reports for this package should be sent. */
#undef PACKAGE_BUGREPORT

/* Define to the full name of this package. */
#undef PACKAGE_NAME

/* Define to the full name and version of this package. */
#undef PACKAGE_STRING

/* Define to the one symbol short name of this package. */
#undef PACKAGE_TARNAME

/* Define to the home page for this package. */
#undef PACKAGE_URL

/* Define to the version of this package. */
#undef PACKAGE_VERSION

/* The size of `int', as computed by sizeof. */
#undef SIZEOF_INT

/* The size of `long', as computed by sizeof. */
#undef SIZEOF_LONG

/* The size of `long long', as computed by sizeof. */
#undef SIZEOF_LONG_LONG

/* The size of `short', as computed by sizeof. */
#undef SIZEOF_SHORT

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS  1

/* Define to 1 if your <sys/time.h> declares `struct tm'. */
#undef TM_IN_SYS_TIME

/* Version number of package */
#undef VERSION

#if _WIN64
#define SIZEOF_SIZE_T 8
#else
#define SIZEOF_SIZE_T 4
#endif //_WIN64

#define SIZEOF_OFF_T  4

#define HAVE_STDBOOL_H  1
