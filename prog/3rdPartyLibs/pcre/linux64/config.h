/* config.h for CMake builds */

#define HAVE_BCOPY 1

/* #undef HAVE_BITS_TYPE_TRAITS_H */
/* #undef HAVE_BZLIB_H */

#define HAVE_DIRENT_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_LIMITS_H 1
#define HAVE_LONG_LONG 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMORY_H 1

/* #undef HAVE_READLINE_HISTORY_H */
/* #undef HAVE_READLINE_READLINE_H */

#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRERROR 1
#define HAVE_STRING 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1

/* #undef HAVE_STRTOLL */

#ifdef _TARGET_C3

#else
#define HAVE_STRTOQ 1
#endif

#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1

/* #undef HAVE_TYPE_TRAITS_H */

#define HAVE_UNISTD_H 1
#define HAVE_UNSIGNED_LONG_LONG 1

/* #undef HAVE_WINDOWS_H */

#define HAVE_ZLIB_H 1

/* #undef HAVE__STRTOI64 */

#define LINK_SIZE 2
#define MATCH_LIMIT 10000000

#define MATCH_LIMIT_RECURSION MATCH_LIMIT

#define MAX_NAME_COUNT 10000
#define MAX_NAME_SIZE 32
#define NEWLINE 10

/* #undef PCRE_EXP_DEFN */
/* #undef PCRE_STATIC */

#define POSIX_MALLOC_THRESHOLD 10

#define STDC_HEADERS 1

/* #undef SUPPORT_LIBBZ2 */
/* #undef SUPPORT_LIBREADLINE */
/* #undef SUPPORT_LIBZ */
/* #undef SUPPORT_UCP */
#define SUPPORT_UTF8 1
/* #undef const */
/* #undef size_t */

/* end config.h for CMake builds */