#include <util/dag_safeArg.h>
#include <util/dag_string.h>
#include <util/dag_simpleString.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_bounds2.h>
#include <math/dag_bounds3.h>
#include <math/dag_TMatrix.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/integer/dag_IPoint4.h>
#include <math/integer/dag_IBBox2.h>
#include <math/integer/dag_IBBox3.h>
#include <EASTL/string.h>
#include <math/dag_color.h>
#include <math/dag_e3dColor.h>
#include <stdio.h>

void DagorSafeArg::set(const String &s)
{
  varValue.s = s;
  varType = TYPE_STR;
}
void DagorSafeArg::set(const SimpleString &s)
{
  varValue.s = s;
  varType = TYPE_STR;
}
void DagorSafeArg::set(const eastl::basic_string<char, eastl::allocator> &s)
{
  varValue.s = s.c_str();
  varType = TYPE_STR;
}
void DagorSafeArg::set(const eastl::basic_string_view<char> &s)
{
  varValue.s = s.data();
  varType = TYPE_STR;
}
void DagorSafeArg::set(const E3DCOLOR &c)
{
  varValue.i = c;
  varType = TYPE_COL;
}

#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_PC_LINUX && _TARGET_64BIT
#define FMT_I64 "%ld"
#elif _TARGET_C3

#elif _TARGET_ANDROID && _TARGET_64BIT
#define FMT_I64 "%ld"
#else
#define FMT_I64 "%lld"
#endif

#if _TARGET_ANDROID
#define IS_FMT64(F) strstr(F, "ll")
#endif

#if defined(_MSC_VER)
#define COUNT_PRNFMT_LEN(fmt, ...) _scprintf(fmt, __VA_ARGS__)
#define PRNFMT(buf, c, fmt, ...)                 \
  do                                             \
  {                                              \
    int r = _snprintf(buf, c, fmt, __VA_ARGS__); \
    buf += r >= 0 ? r : c;                       \
    c -= r >= 0 ? r : c;                         \
  } while (0)
#define VPRNFMT(buf, c, fmt, ap)         \
  do                                     \
  {                                      \
    int r = _vsnprintf(buf, c, fmt, ap); \
    buf += r >= 0 ? r : c;               \
    c -= r >= 0 ? r : c;                 \
  } while (0)
#elif defined(__GNUC__)
#define COUNT_PRNFMT_LEN(fmt, ...) snprintf(NULL, 0, fmt, __VA_ARGS__)
#define PRNFMT(buf, c, fmt, ...)                \
  do                                            \
  {                                             \
    int r = snprintf(buf, c, fmt, __VA_ARGS__); \
    buf += r <= c ? r : c;                      \
    c -= r <= c ? r : c;                        \
  } while (0)
#define VPRNFMT(buf, c, fmt, ap)        \
  do                                    \
  {                                     \
    int r = vsnprintf(buf, c, fmt, ap); \
    buf += r <= c ? r : c;              \
    c -= r <= c ? r : c;                \
  } while (0)
#endif

#ifdef IS_FMT64
#define COUNT_PRNFMT_INT1_LEN(fmt, v1)     IS_FMT64(fmt) ? snprintf(NULL, 0, fmt, v1) : snprintf(NULL, 0, fmt, (int)v1)
#define COUNT_PRNFMT_INT2_LEN(fmt, v1, v2) IS_FMT64(fmt) ? snprintf(NULL, 0, fmt, v1, v2) : snprintf(NULL, 0, fmt, v1, (int)v2)
#define COUNT_PRNFMT_INT3_LEN(fmt, v1, v2, v3) \
  IS_FMT64(fmt) ? snprintf(NULL, 0, fmt, v1, v2, v3) : snprintf(NULL, 0, fmt, v1, v2, (int)v3)

#define PRNFMT_INT1(buf, c, fmt, v1)                                                      \
  if (int r = IS_FMT64(fmt) ? snprintf(buf, c, fmt, v1) : snprintf(buf, c, fmt, (int)v1)) \
    do                                                                                    \
    {                                                                                     \
      buf += r <= c ? r : c;                                                              \
      c -= r <= c ? r : c;                                                                \
  } while (0)
#define PRNFMT_INT2(buf, c, fmt, v1, v2)                                                          \
  if (int r = IS_FMT64(fmt) ? snprintf(buf, c, fmt, v1, v2) : snprintf(buf, c, fmt, v1, (int)v2)) \
    do                                                                                            \
    {                                                                                             \
      buf += r <= c ? r : c;                                                                      \
      c -= r <= c ? r : c;                                                                        \
  } while (0)
#define PRNFMT_INT3(buf, c, fmt, v1, v2, v3)                                                              \
  if (int r = IS_FMT64(fmt) ? snprintf(buf, c, fmt, v1, v2, v3) : snprintf(buf, c, fmt, v1, v2, (int)v3)) \
    do                                                                                                    \
    {                                                                                                     \
      buf += r <= c ? r : c;                                                                              \
      c -= r <= c ? r : c;                                                                                \
  } while (0)

#define FPRINTF_INT1(fp, fmt, v1)         IS_FMT64(fmt) ? fprintf(fp, fmt, v1) : fprintf(fp, fmt, (int)v1)
#define FPRINTF_INT2(fp, fmt, v1, v2)     IS_FMT64(fmt) ? fprintf(fp, fmt, v1, v2) : fprintf(fp, fmt, v1, (int)v2)
#define FPRINTF_INT3(fp, fmt, v1, v2, v3) IS_FMT64(fmt) ? fprintf(fp, fmt, v1, v2, v3) : fprintf(fp, fmt, v1, v2, (int)v3)

#else
#define COUNT_PRNFMT_INT1_LEN COUNT_PRNFMT_LEN
#define PRNFMT_INT1           PRNFMT
#define FPRINTF_INT1          fprintf
#define COUNT_PRNFMT_INT2_LEN COUNT_PRNFMT_LEN
#define PRNFMT_INT2           PRNFMT
#define FPRINTF_INT2          fprintf
#define COUNT_PRNFMT_INT3_LEN COUNT_PRNFMT_LEN
#define PRNFMT_INT3           PRNFMT
#define FPRINTF_INT3          fprintf
#endif

#define PROCESS_VARIABLE_WD_PREC()                                                    \
  int cwp[3] = {0, 0, 0};                                                             \
  for (const char *p = strchr(fmt_buf, '*'); p && cwp[0] < 2; p = strchr(p + 1, '*')) \
  {                                                                                   \
    cwp[0]++;                                                                         \
    if (carg < anum && arg[carg].varType == DagorSafeArg::TYPE_INT)                   \
      cwp[cwp[0]] = (int)arg[carg].varValue.i;                                        \
    carg++;                                                                           \
  }

static bool check_dagor_custom_fmt(const char *&fmt, const DagorSafeArg *arg, int anum, int &carg)
{
  if (carg >= anum)
    return false;

  const DagorSafeArg &a = arg[carg];
  if (fmt[1] == '@')
  {
    fmt += 1;
    return true;
  }
  if (strncmp(&fmt[2], "p2", 2) == 0)
  {
    if (a.varType == a.TYPE_P2 || a.varType == a.TYPE_IP2)
      fmt += 3;
    else
    {
      carg++;
      return false;
    }
  }
  else if (strncmp(&fmt[2], "p3", 2) == 0)
  {
    if (a.varType == a.TYPE_P3 || a.varType == a.TYPE_IP3)
      fmt += 3;
    else
    {
      carg++;
      return false;
    }
  }
  else if (strncmp(&fmt[2], "p4", 2) == 0)
  {
    if (a.varType == a.TYPE_P4 || a.varType == a.TYPE_IP4)
      fmt += 3;
    else
    {
      carg++;
      return false;
    }
  }
  else if (strncmp(&fmt[2], "bb2", 3) == 0)
  {
    if (a.varType == a.TYPE_BB2 || a.varType == a.TYPE_IBB2)
      fmt += 4;
    else
    {
      carg++;
      return false;
    }
  }
  else if (strncmp(&fmt[2], "bb3", 3) == 0)
  {
    if (a.varType == a.TYPE_BB3 || a.varType == a.TYPE_IBB3)
      fmt += 4;
    else
    {
      carg++;
      return false;
    }
  }
  else if (strncmp(&fmt[2], "tm", 2) == 0)
  {
    if (a.varType == a.TYPE_TM)
      fmt += 3;
    else
    {
      carg++;
      return false;
    }
  }
  else if (strncmp(&fmt[2], "c4", 2) == 0)
  {
    if (a.varType == a.TYPE_COL4 || a.varType == a.TYPE_COL)
      fmt += 3;
    else
    {
      carg++;
      return false;
    }
  }
  else if (strncmp(&fmt[2], "c3", 2) == 0)
  {
    if (a.varType == a.TYPE_COL3)
      fmt += 3;
    else
    {
      carg++;
      return false;
    }
  }
  else
    return false;

  return true;
}

int DagorSafeArg::count_len(const char *fmt, const DagorSafeArg *arg, int anum)
{
  if (!anum)
    return (int)strlen(fmt);

  int len = 0, carg = 0;
  char fmt_buf[32];

  for (char c = *fmt; c; fmt++, c = *fmt)
  {
    if (c != '%')
    {
      len++;
      continue;
    }
    if (fmt[1] == '%')
    {
      len++;
      fmt++;
      continue;
    }

    if (fmt[1] == '~' || fmt[1] == '@')
    {
      if (!check_dagor_custom_fmt(fmt, arg, anum, carg))
      {
        len++;
        continue;
      }

      const DagorSafeArg &a = arg[carg];
      switch (a.varType)
      {
        case DagorSafeArg::TYPE_CUSTOM: len += a.varValue.printer->getLength(); break;
        case DagorSafeArg::TYPE_COL4:
          len += COUNT_PRNFMT_LEN("(%.2f,%.2f,%.2f,%.2f)", a.varValue.c4->r, a.varValue.c4->g, a.varValue.c4->b, a.varValue.c4->a);
          break;
        case DagorSafeArg::TYPE_COL3:
          len += COUNT_PRNFMT_LEN("(%.2f,%.2f,%.2f)", a.varValue.c3->r, a.varValue.c3->g, a.varValue.c3->b);
          break;
        case DagorSafeArg::TYPE_COL:
          len += COUNT_PRNFMT_LEN("(%hhu,%hhu,%hhu,%hhu)", E3DCOLOR((unsigned)a.varValue.i).r, E3DCOLOR((unsigned)a.varValue.i).g,
            E3DCOLOR((unsigned)a.varValue.i).b, E3DCOLOR((unsigned)a.varValue.i).a);
          break;
        case DagorSafeArg::TYPE_P2: len += COUNT_PRNFMT_LEN("(%.3f,%.3f)", a.varValue.p2->x, a.varValue.p2->y); break;
        case DagorSafeArg::TYPE_P3:
          len += COUNT_PRNFMT_LEN("(%.3f,%.3f,%.3f)", a.varValue.p3->x, a.varValue.p3->y, a.varValue.p3->z);
          break;
        case DagorSafeArg::TYPE_P4:
          len += COUNT_PRNFMT_LEN("(%.3f,%.3f,%.3f,%.3f)", a.varValue.p4->x, a.varValue.p4->y, a.varValue.p4->z, a.varValue.p4->w);
          break;
        case DagorSafeArg::TYPE_IP2: len += COUNT_PRNFMT_LEN("(%d,%d)", a.varValue.ip2->x, a.varValue.ip2->y); break;
        case DagorSafeArg::TYPE_IP3:
          len += COUNT_PRNFMT_LEN("(%d,%d,%d)", a.varValue.ip3->x, a.varValue.ip3->y, a.varValue.ip3->z);
          break;
        case DagorSafeArg::TYPE_IP4:
          len += COUNT_PRNFMT_LEN("(%d,%d,%d,%d)", a.varValue.ip4->x, a.varValue.ip4->y, a.varValue.ip4->z, a.varValue.ip4->w);
          break;
        case DagorSafeArg::TYPE_TM:
          len += COUNT_PRNFMT_LEN("{(%.3f,%.3f,%.3f) (%.3f,%.3f,%.3f) (%.3f,%.3f,%.3f) (%.3f,%.3f,%.3f)}", a.varValue.tm->m[0][0],
            a.varValue.tm->m[0][1], a.varValue.tm->m[0][2], a.varValue.tm->m[1][0], a.varValue.tm->m[1][1], a.varValue.tm->m[1][2],
            a.varValue.tm->m[2][0], a.varValue.tm->m[2][1], a.varValue.tm->m[2][2], a.varValue.tm->m[3][0], a.varValue.tm->m[3][1],
            a.varValue.tm->m[3][2]);
          break;
        case DagorSafeArg::TYPE_BB2:
          len += COUNT_PRNFMT_LEN("(%.3f,%.3f)-(%.3f,%.3f)", a.varValue.bb2->lim[0].x, a.varValue.bb2->lim[0].y,
            a.varValue.bb2->lim[1].x, a.varValue.bb2->lim[1].y);
          break;
        case DagorSafeArg::TYPE_BB3:
          len += COUNT_PRNFMT_LEN("(%.3f,%.3f,%.3f)-(%.3f,%.3f,%.3f)", a.varValue.bb3->lim[0].x, a.varValue.bb3->lim[0].y,
            a.varValue.bb3->lim[0].z, a.varValue.bb3->lim[1].x, a.varValue.bb3->lim[1].y, a.varValue.bb3->lim[1].z);
          break;
        case DagorSafeArg::TYPE_IBB2:
          len += COUNT_PRNFMT_LEN("(%d,%d)-(%d,%d)", a.varValue.ibb2->lim[0].x, a.varValue.ibb2->lim[0].y, a.varValue.ibb2->lim[1].x,
            a.varValue.ibb2->lim[1].y);
          break;
        case DagorSafeArg::TYPE_IBB3:
          len += COUNT_PRNFMT_LEN("(%d,%d,%d)-(%d,%d,%d)", a.varValue.ibb3->lim[0].x, a.varValue.ibb3->lim[0].y,
            a.varValue.ibb3->lim[0].z, a.varValue.ibb3->lim[1].x, a.varValue.ibb3->lim[1].y, a.varValue.ibb3->lim[1].z);
          break;

        case DagorSafeArg::TYPE_INT: len += COUNT_PRNFMT_LEN(FMT_I64, a.varValue.i); break;
        case DagorSafeArg::TYPE_DOUBLE: len += COUNT_PRNFMT_LEN("%g", a.varValue.d); break;
        case DagorSafeArg::TYPE_STR: len += COUNT_PRNFMT_LEN("%s", a.varValue.s); break;
        case DagorSafeArg::TYPE_PTR: len += COUNT_PRNFMT_LEN("%p", a.varValue.p); break;

        default: G_ASSERT(0);
      }
      carg++;
      continue;
    }

    for (int i = 1; i < 31 && fmt[i]; i++)
      if (strchr("diuoxXfFeEgGaAcsp% ", fmt[i]))
      {
        strncpy(fmt_buf, fmt, i + 1);
        fmt_buf[i + 1] = 0;

        PROCESS_VARIABLE_WD_PREC();
        if (carg < anum && fmt[i] != '%' && fmt[i] != ' ')
        {
          const DagorSafeArg &a = arg[carg];
          bool mismatch = false;
          switch (fmt[i])
          {
            case 'd':
            case 'i':
            case 'u':
            case 'o':
            case 'x':
            case 'X':
            case 'c':
              if (a.varType == a.TYPE_INT)
                switch (cwp[0])
                {
                  case 0: len += COUNT_PRNFMT_INT1_LEN(fmt_buf, a.varValue.i); break;
                  case 1: len += COUNT_PRNFMT_INT2_LEN(fmt_buf, cwp[1], a.varValue.i); break;
                  case 2: len += COUNT_PRNFMT_INT3_LEN(fmt_buf, cwp[1], cwp[2], a.varValue.i); break;
                }
              else if (a.varType == a.TYPE_DOUBLE)
                switch (cwp[0])
                {
                  case 0: len += COUNT_PRNFMT_LEN(fmt_buf, (int)a.varValue.d); break;
                  case 1: len += COUNT_PRNFMT_LEN(fmt_buf, cwp[1], (int)a.varValue.d); break;
                  case 2: len += COUNT_PRNFMT_LEN(fmt_buf, cwp[1], cwp[2], (int)a.varValue.d); break;
                }
              else
                mismatch = true;
              break;
            case 'f':
            case 'F':
            case 'e':
            case 'E':
            case 'g':
            case 'G':
            case 'a':
            case 'A':
              if (a.varType == a.TYPE_DOUBLE)
                switch (cwp[0])
                {
                  case 0: len += COUNT_PRNFMT_LEN(fmt_buf, a.varValue.d); break;
                  case 1: len += COUNT_PRNFMT_LEN(fmt_buf, cwp[1], a.varValue.d); break;
                  case 2: len += COUNT_PRNFMT_LEN(fmt_buf, cwp[1], cwp[2], a.varValue.d); break;
                }
              else if (a.varType == a.TYPE_INT)
                switch (cwp[0])
                {
                  case 0: len += COUNT_PRNFMT_LEN(fmt_buf, (double)a.varValue.i); break;
                  case 1: len += COUNT_PRNFMT_LEN(fmt_buf, cwp[1], (double)a.varValue.i); break;
                  case 2: len += COUNT_PRNFMT_LEN(fmt_buf, cwp[1], cwp[2], (double)a.varValue.i); break;
                }
              else
                mismatch = true;
              break;
            case 's':
              if (a.varType == a.TYPE_STR || a.varType == a.TYPE_PTR)
                switch (cwp[0])
                {
                  case 0: len += COUNT_PRNFMT_LEN(fmt_buf, a.varValue.s); break;
                  case 1: len += COUNT_PRNFMT_LEN(fmt_buf, cwp[1], a.varValue.s); break;
                  case 2: len += COUNT_PRNFMT_LEN(fmt_buf, cwp[1], cwp[2], a.varValue.s); break;
                }
              else
                mismatch = true;
              break;
            case 'p':
              G_ASSERT(cwp[0] == 0);
              if ((a.varType >= a.TYPE_PTR && a.varType < a.TYPE_PTR_END) || a.varType == a.TYPE_INT || a.varType == a.TYPE_STR)
                len += COUNT_PRNFMT_LEN(fmt_buf, a.varValue.p);
              else
                mismatch = true;
              break;

            default: mismatch = true;
          }

          if (mismatch)
          {
            if (a.varType == a.TYPE_INT)
              len += COUNT_PRNFMT_LEN(FMT_I64, a.varValue.i);
            else if (a.varType == a.TYPE_DOUBLE)
              len += COUNT_PRNFMT_LEN("%g", a.varValue.d);
            else if (a.varType == a.TYPE_STR)
              len += COUNT_PRNFMT_LEN("%s", a.varValue.s);
            else if (a.varType >= a.TYPE_PTR && a.varType < a.TYPE_PTR_END)
              len += COUNT_PRNFMT_LEN("%p", a.varValue.p);
          }
        }
        else
          len += i + 1 + 4; // output as ~(%...)~
        carg++;
        fmt += i;
        break;
      }
  }

  return len;
}

int DagorSafeArg::print_fmt(char *buf, int len, const char *fmt, const DagorSafeArg *arg, int anum)
{
  G_FAST_ASSERT(buf && fmt && len > 0); // Don't use regular `G_ASSERT` since it might create unwanted recursion

  if (!anum)
  {
    int l = (int)strlen(fmt);
    if (len < l + 1)
      l = (len > 0) ? (len - 1) : 0;
    memcpy(buf, fmt, l);
    buf[l] = 0;
    return l;
  }

  int carg = 0;
  char fmt_buf[32];
  char *bufp = buf;

  for (char c = *fmt; c && len; fmt++, c = *fmt)
  {
    if (c != '%')
    {
      *bufp = c;
      bufp++;
      len--;
      continue;
    }
    if (fmt[1] == '%')
    {
      *bufp = c;
      bufp++;
      len--;
      fmt++;
      continue;
    }

    if (fmt[1] == '~' || fmt[1] == '@')
    {
      if (!check_dagor_custom_fmt(fmt, arg, anum, carg))
      {
        *bufp = c;
        bufp++;
        len--;
        continue;
      }

      const DagorSafeArg &a = arg[carg];
      switch (a.varType)
      {
        case DagorSafeArg::TYPE_CUSTOM: a.varValue.printer->print(bufp, len); break;
        case DagorSafeArg::TYPE_COL4:
          PRNFMT(bufp, len, "(%.2f,%.2f,%.2f,%.2f)", a.varValue.c4->r, a.varValue.c4->g, a.varValue.c4->b, a.varValue.c4->a);
          break;
        case DagorSafeArg::TYPE_COL3:
          PRNFMT(bufp, len, "(%.2f,%.2f,%.2f)", a.varValue.c3->r, a.varValue.c3->g, a.varValue.c3->b);
          break;
        case DagorSafeArg::TYPE_COL:
          PRNFMT(bufp, len, "(%hhu,%hhu,%hhu,%hhu)", E3DCOLOR((unsigned)a.varValue.i).r, E3DCOLOR((unsigned)a.varValue.i).g,
            E3DCOLOR((unsigned)a.varValue.i).b, E3DCOLOR((unsigned)a.varValue.i).a);
          break;
        case DagorSafeArg::TYPE_P2: PRNFMT(bufp, len, "(%.3f,%.3f)", a.varValue.p2->x, a.varValue.p2->y); break;
        case DagorSafeArg::TYPE_P3: PRNFMT(bufp, len, "(%.3f,%.3f,%.3f)", a.varValue.p3->x, a.varValue.p3->y, a.varValue.p3->z); break;
        case DagorSafeArg::TYPE_P4:
          PRNFMT(bufp, len, "(%.3f,%.3f,%.3f,%.3f)", a.varValue.p4->x, a.varValue.p4->y, a.varValue.p4->z, a.varValue.p4->w);
          break;
        case DagorSafeArg::TYPE_IP2: PRNFMT(bufp, len, "(%d,%d)", a.varValue.ip2->x, a.varValue.ip2->y); break;
        case DagorSafeArg::TYPE_IP3: PRNFMT(bufp, len, "(%d,%d,%d)", a.varValue.ip3->x, a.varValue.ip3->y, a.varValue.ip3->z); break;
        case DagorSafeArg::TYPE_IP4:
          PRNFMT(bufp, len, "(%d,%d,%d,%d)", a.varValue.ip4->x, a.varValue.ip4->y, a.varValue.ip4->z, a.varValue.ip4->w);
          break;
        case DagorSafeArg::TYPE_TM:
          PRNFMT(bufp, len, "{(%.3f,%.3f,%.3f) (%.3f,%.3f,%.3f) (%.3f,%.3f,%.3f) (%.3f,%.3f,%.3f)}", a.varValue.tm->m[0][0],
            a.varValue.tm->m[0][1], a.varValue.tm->m[0][2], a.varValue.tm->m[1][0], a.varValue.tm->m[1][1], a.varValue.tm->m[1][2],
            a.varValue.tm->m[2][0], a.varValue.tm->m[2][1], a.varValue.tm->m[2][2], a.varValue.tm->m[3][0], a.varValue.tm->m[3][1],
            a.varValue.tm->m[3][2]);
          break;
        case DagorSafeArg::TYPE_BB2:
          PRNFMT(bufp, len, "(%.3f,%.3f)-(%.3f,%.3f)", a.varValue.bb2->lim[0].x, a.varValue.bb2->lim[0].y, a.varValue.bb2->lim[1].x,
            a.varValue.bb2->lim[1].y);
          break;
        case DagorSafeArg::TYPE_BB3:
          PRNFMT(bufp, len, "(%.3f,%.3f,%.3f)-(%.3f,%.3f,%.3f)", a.varValue.bb3->lim[0].x, a.varValue.bb3->lim[0].y,
            a.varValue.bb3->lim[0].z, a.varValue.bb3->lim[1].x, a.varValue.bb3->lim[1].y, a.varValue.bb3->lim[1].z);
          break;
        case DagorSafeArg::TYPE_IBB2:
          PRNFMT(bufp, len, "(%d,%d)-(%d,%d)", a.varValue.ibb2->lim[0].x, a.varValue.ibb2->lim[0].y, a.varValue.ibb2->lim[1].x,
            a.varValue.ibb2->lim[1].y);
          break;
        case DagorSafeArg::TYPE_IBB3:
          PRNFMT(bufp, len, "(%d,%d,%d)-(%d,%d,%d)", a.varValue.ibb3->lim[0].x, a.varValue.ibb3->lim[0].y, a.varValue.ibb3->lim[0].z,
            a.varValue.ibb3->lim[1].x, a.varValue.ibb3->lim[1].y, a.varValue.ibb3->lim[1].z);
          break;

        case DagorSafeArg::TYPE_INT: PRNFMT(bufp, len, FMT_I64, a.varValue.i); break;
        case DagorSafeArg::TYPE_DOUBLE: PRNFMT(bufp, len, "%g", a.varValue.d); break;
        case DagorSafeArg::TYPE_STR: PRNFMT(bufp, len, "%s", a.varValue.s); break;
        case DagorSafeArg::TYPE_PTR: PRNFMT(bufp, len, "%p", a.varValue.p); break;

        default: G_ASSERT(0);
      }
      carg++;
      continue;
    }

    for (int i = 1; i < 31 && fmt[i]; i++)
      if (strchr("diuoxXfFeEgGaAcsp% ", fmt[i]))
      {
        strncpy(fmt_buf, fmt, i + 1);
        fmt_buf[i + 1] = 0;

        PROCESS_VARIABLE_WD_PREC();
        if (carg < anum && fmt[i] != '%' && fmt[i] != ' ')
        {
          const DagorSafeArg &a = arg[carg];
          bool mismatch = false;
          switch (fmt[i])
          {
            case 'd':
            case 'i':
            case 'u':
            case 'o':
            case 'x':
            case 'X':
            case 'c':
              if (a.varType == a.TYPE_INT)
                switch (cwp[0])
                {
                  case 0: PRNFMT_INT1(bufp, len, fmt_buf, a.varValue.i); break;
                  case 1: PRNFMT_INT2(bufp, len, fmt_buf, cwp[1], a.varValue.i); break;
                  case 2: PRNFMT_INT3(bufp, len, fmt_buf, cwp[1], cwp[2], a.varValue.i); break;
                }
              else if (a.varType == a.TYPE_DOUBLE)
                switch (cwp[0])
                {
                  case 0: PRNFMT(bufp, len, fmt_buf, (int)a.varValue.d); break;
                  case 1: PRNFMT(bufp, len, fmt_buf, cwp[1], (int)a.varValue.d); break;
                  case 2: PRNFMT(bufp, len, fmt_buf, cwp[1], cwp[2], (int)a.varValue.d); break;
                }
              else
                mismatch = true;
              break;
            case 'f':
            case 'F':
            case 'e':
            case 'E':
            case 'g':
            case 'G':
            case 'a':
            case 'A':
              if (a.varType == a.TYPE_DOUBLE)
                switch (cwp[0])
                {
                  case 0: PRNFMT(bufp, len, fmt_buf, a.varValue.d); break;
                  case 1: PRNFMT(bufp, len, fmt_buf, cwp[1], a.varValue.d); break;
                  case 2: PRNFMT(bufp, len, fmt_buf, cwp[1], cwp[2], a.varValue.d); break;
                }
              else if (a.varType == a.TYPE_INT)
                switch (cwp[0])
                {
                  case 0: PRNFMT(bufp, len, fmt_buf, (double)a.varValue.i); break;
                  case 1: PRNFMT(bufp, len, fmt_buf, cwp[1], (double)a.varValue.i); break;
                  case 2: PRNFMT(bufp, len, fmt_buf, cwp[1], cwp[2], (double)a.varValue.i); break;
                }
              else
                mismatch = true;
              break;
            case 's':
              if (a.varType == a.TYPE_STR || a.varType == a.TYPE_PTR)
                switch (cwp[0])
                {
                  case 0: PRNFMT(bufp, len, fmt_buf, a.varValue.s); break;
                  case 1: PRNFMT(bufp, len, fmt_buf, cwp[1], a.varValue.s); break;
                  case 2: PRNFMT(bufp, len, fmt_buf, cwp[1], cwp[2], a.varValue.s); break;
                }
              else
                mismatch = true;
              break;
            case 'p':
              G_ASSERT(cwp[0] == 0);
              if ((a.varType >= a.TYPE_PTR && a.varType < a.TYPE_PTR_END) || a.varType == a.TYPE_INT || a.varType == a.TYPE_STR)
                PRNFMT(bufp, len, fmt_buf, a.varValue.p);
              else
                mismatch = true;
              break;

            default: mismatch = true;
          }

          if (mismatch)
          {
            if (a.varType == a.TYPE_INT)
              PRNFMT(bufp, len, FMT_I64, a.varValue.i);
            else if (a.varType == a.TYPE_DOUBLE)
              PRNFMT(bufp, len, "%g", a.varValue.d);
            else if (a.varType == a.TYPE_STR)
              PRNFMT(bufp, len, "%s", a.varValue.s);
            else if (a.varType >= a.TYPE_PTR && a.varType < a.TYPE_PTR_END)
              PRNFMT(bufp, len, "%p", a.varValue.p);
          }
        }
        else if (len >= i + 1 + 4)
        {
          bufp[0] = '~';
          bufp[1] = '(';
          bufp += 2;
          strncpy(bufp, fmt, i + 1);
          bufp += i + 1;
          bufp[0] = ')';
          bufp[1] = '~';
          bufp += 2;
          len -= i + 1 + 4;
        }
        else
          bufp++, len = 0;

        carg++;
        fmt += i;
        break;
      }
  }

  if (len)
    *bufp = 0;
  else if (bufp > buf)
  {
    bufp[-1] = 0;
    return bufp - buf - 1;
  }
  return bufp - buf;
}

int DagorSafeArg::fprint_fmt(void *fp, const char *fmt, const DagorSafeArg *arg, int anum)
{
  if (!anum)
    return fprintf((FILE *)fp, "%s", fmt);

  int carg = 0, len = 0;
  char fmt_buf[32];
  const char *str_s = NULL;
  int str_l = 0;

  for (char c = *fmt; c; fmt++, c = *fmt)
  {
    if (c != '%')
    {
      (str_l) ? str_l++ : (str_s = fmt, str_l = 1);
      continue;
    }
    if (fmt[1] == '%')
    {
      (str_l) ? str_l++ : (str_s = fmt, str_l = 1);
      fmt++;
      continue;
    }

    if (fmt[1] == '~' || fmt[1] == '@')
    {
      if (!check_dagor_custom_fmt(fmt, arg, anum, carg))
      {
        (str_l) ? str_l++ : (str_s = fmt, str_l = 1);
        continue;
      }

      if (str_l)
      {
        len += (int)fwrite(str_s, 1, str_l, (FILE *)fp);
        str_l = 0;
      }
      const DagorSafeArg &a = arg[carg];
      switch (a.varType)
      {
        case DagorSafeArg::TYPE_COL4:
          len += fprintf((FILE *)fp, "(%.2f,%.2f,%.2f,%.2f)", a.varValue.c4->r, a.varValue.c4->g, a.varValue.c4->b, a.varValue.c4->a);
          break;
        case DagorSafeArg::TYPE_COL3:
          len += fprintf((FILE *)fp, "(%.2f,%.2f,%.2f)", a.varValue.c3->r, a.varValue.c3->g, a.varValue.c3->b);
          break;
        case DagorSafeArg::TYPE_COL:
          len += fprintf((FILE *)fp, "(%hhu,%hhu,%hhu,%hhu)", E3DCOLOR((unsigned)a.varValue.i).r, E3DCOLOR((unsigned)a.varValue.i).g,
            E3DCOLOR((unsigned)a.varValue.i).b, E3DCOLOR((unsigned)a.varValue.i).a);
          break;
        case DagorSafeArg::TYPE_P2: len += fprintf((FILE *)fp, "(%.3f,%.3f)", a.varValue.p2->x, a.varValue.p2->y); break;
        case DagorSafeArg::TYPE_P3:
          len += fprintf((FILE *)fp, "(%.3f,%.3f,%.3f)", a.varValue.p3->x, a.varValue.p3->y, a.varValue.p3->z);
          break;
        case DagorSafeArg::TYPE_P4:
          len += fprintf((FILE *)fp, "(%.3f,%.3f,%.3f,%.3f)", a.varValue.p4->x, a.varValue.p4->y, a.varValue.p4->z, a.varValue.p4->w);
          break;
        case DagorSafeArg::TYPE_IP2: len += fprintf((FILE *)fp, "(%d,%d)", a.varValue.ip2->x, a.varValue.ip2->y); break;
        case DagorSafeArg::TYPE_IP3:
          len += fprintf((FILE *)fp, "(%d,%d,%d)", a.varValue.ip3->x, a.varValue.ip3->y, a.varValue.ip3->z);
          break;
        case DagorSafeArg::TYPE_IP4:
          len += fprintf((FILE *)fp, "(%d,%d,%d,%d)", a.varValue.ip4->x, a.varValue.ip4->y, a.varValue.ip4->z, a.varValue.ip4->w);
          break;
        case DagorSafeArg::TYPE_TM:
          len += fprintf((FILE *)fp, "{(%.3f,%.3f,%.3f) (%.3f,%.3f,%.3f) (%.3f,%.3f,%.3f) (%.3f,%.3f,%.3f)}", a.varValue.tm->m[0][0],
            a.varValue.tm->m[0][1], a.varValue.tm->m[0][2], a.varValue.tm->m[1][0], a.varValue.tm->m[1][1], a.varValue.tm->m[1][2],
            a.varValue.tm->m[2][0], a.varValue.tm->m[2][1], a.varValue.tm->m[2][2], a.varValue.tm->m[3][0], a.varValue.tm->m[3][1],
            a.varValue.tm->m[3][2]);
          break;
        case DagorSafeArg::TYPE_BB2:
          len += fprintf((FILE *)fp, "(%.3f,%.3f)-(%.3f,%.3f)", a.varValue.bb2->lim[0].x, a.varValue.bb2->lim[0].y,
            a.varValue.bb2->lim[1].x, a.varValue.bb2->lim[1].y);
          break;
        case DagorSafeArg::TYPE_BB3:
          len += fprintf((FILE *)fp, "(%.3f,%.3f,%.3f)-(%.3f,%.3f,%.3f)", a.varValue.bb3->lim[0].x, a.varValue.bb3->lim[0].y,
            a.varValue.bb3->lim[0].z, a.varValue.bb3->lim[1].x, a.varValue.bb3->lim[1].y, a.varValue.bb3->lim[1].z);
          break;
        case DagorSafeArg::TYPE_IBB2:
          len += fprintf((FILE *)fp, "(%d,%d)-(%d,%d)", a.varValue.ibb2->lim[0].x, a.varValue.ibb2->lim[0].y,
            a.varValue.ibb2->lim[1].x, a.varValue.ibb2->lim[1].y);
          break;
        case DagorSafeArg::TYPE_IBB3:
          len += fprintf((FILE *)fp, "(%d,%d,%d)-(%d,%d,%d)", a.varValue.ibb3->lim[0].x, a.varValue.ibb3->lim[0].y,
            a.varValue.ibb3->lim[0].z, a.varValue.ibb3->lim[1].x, a.varValue.ibb3->lim[1].y, a.varValue.ibb3->lim[1].z);
          break;

        case DagorSafeArg::TYPE_INT: len += fprintf((FILE *)fp, FMT_I64, a.varValue.i); break;
        case DagorSafeArg::TYPE_DOUBLE: len += fprintf((FILE *)fp, "%g", a.varValue.d); break;
        case DagorSafeArg::TYPE_STR: len += fprintf((FILE *)fp, "%s", a.varValue.s); break;
        case DagorSafeArg::TYPE_PTR: len += fprintf((FILE *)fp, "%p", a.varValue.p); break;

        default: G_ASSERT(0);
      }
      carg++;
      continue;
    }

    if (str_l)
    {
      len += (int)fwrite(str_s, 1, str_l, (FILE *)fp);
      str_l = 0;
    }
    for (int i = 1; i < 31 && fmt[i]; i++)
      if (strchr("diuoxXfFeEgGaAcsp% ", fmt[i]))
      {
        strncpy(fmt_buf, fmt, i + 1);
        fmt_buf[i + 1] = 0;

        PROCESS_VARIABLE_WD_PREC();
        if (carg < anum && fmt[i] != '%' && fmt[i] != ' ')
        {
          const DagorSafeArg &a = arg[carg];
          bool mismatch = false;
          switch (fmt[i])
          {
            case 'd':
            case 'i':
            case 'u':
            case 'o':
            case 'x':
            case 'X':
            case 'c':
              if (a.varType == a.TYPE_INT)
                switch (cwp[0])
                {
                  case 0: len += FPRINTF_INT1((FILE *)fp, fmt_buf, a.varValue.i); break;
                  case 1: len += FPRINTF_INT2((FILE *)fp, fmt_buf, cwp[1], a.varValue.i); break;
                  case 2: len += FPRINTF_INT3((FILE *)fp, fmt_buf, cwp[1], cwp[2], a.varValue.i); break;
                }
              else if (a.varType == a.TYPE_DOUBLE)
                switch (cwp[0])
                {
                  case 0: len += fprintf((FILE *)fp, fmt_buf, (int)a.varValue.d); break;
                  case 1: len += fprintf((FILE *)fp, fmt_buf, cwp[1], (int)a.varValue.d); break;
                  case 2: len += fprintf((FILE *)fp, fmt_buf, cwp[1], cwp[2], (int)a.varValue.d); break;
                }
              else
                mismatch = true;
              break;
            case 'f':
            case 'F':
            case 'e':
            case 'E':
            case 'g':
            case 'G':
            case 'a':
            case 'A':
              if (a.varType == a.TYPE_DOUBLE)
                switch (cwp[0])
                {
                  case 0: len += fprintf((FILE *)fp, fmt_buf, a.varValue.d); break;
                  case 1: len += fprintf((FILE *)fp, fmt_buf, cwp[1], a.varValue.d); break;
                  case 2: len += fprintf((FILE *)fp, fmt_buf, cwp[1], cwp[2], a.varValue.d); break;
                }
              else if (a.varType == a.TYPE_INT)
                switch (cwp[0])
                {
                  case 0: len += fprintf((FILE *)fp, fmt_buf, (double)a.varValue.i); break;
                  case 1: len += fprintf((FILE *)fp, fmt_buf, cwp[1], (double)a.varValue.i); break;
                  case 2: len += fprintf((FILE *)fp, fmt_buf, cwp[1], cwp[2], (double)a.varValue.i); break;
                }
              else
                mismatch = true;
              break;
            case 's':
              if (a.varType == a.TYPE_STR || a.varType == a.TYPE_PTR)
                switch (cwp[0])
                {
                  case 0: len += fprintf((FILE *)fp, fmt_buf, a.varValue.s); break;
                  case 1: len += fprintf((FILE *)fp, fmt_buf, cwp[1], a.varValue.s); break;
                  case 2: len += fprintf((FILE *)fp, fmt_buf, cwp[1], cwp[2], a.varValue.s); break;
                }
              else
                mismatch = true;
              break;
            case 'p':
              G_ASSERT(cwp[0] == 0);
              if ((a.varType >= a.TYPE_PTR && a.varType < a.TYPE_PTR_END) || a.varType == a.TYPE_INT || a.varType == a.TYPE_STR)
                len += fprintf((FILE *)fp, fmt_buf, a.varValue.p);
              else
                mismatch = true;
              break;

            default: mismatch = true;
          }

          if (mismatch)
          {
            if (a.varType == a.TYPE_INT)
              len += fprintf((FILE *)fp, FMT_I64, a.varValue.i);
            else if (a.varType == a.TYPE_DOUBLE)
              len += fprintf((FILE *)fp, "%g", a.varValue.d);
            else if (a.varType == a.TYPE_STR)
              len += fprintf((FILE *)fp, "%s", a.varValue.s);
            else if (a.varType >= a.TYPE_PTR && a.varType < a.TYPE_PTR_END)
              len += fprintf((FILE *)fp, "%p", a.varValue.p);
          }
        }
        else
          len += fprintf((FILE *)fp, "~(%.*s)~", i + 1, fmt);

        carg++;
        fmt += i;
        break;
      }
  }
  if (str_l)
    len += (int)fwrite(str_s, 1, str_l, (FILE *)fp);

  return len;
}

int DagorSafeArg::mixed_print_fmt(char *buf, int len, const char *fmt, const void *valist_or_dsa, int dsa_num)
{
  if (dsa_num >= 0)
    return print_fmt(buf, len, fmt, (const DagorSafeArg *)valist_or_dsa, dsa_num);

  char *bufp = buf;
  DAG_VACOPY(args_copy, *(va_list *)valist_or_dsa);
  VPRNFMT(bufp, len, fmt, args_copy);
  DAG_VACOPYEND(args_copy);

  if (len)
    *bufp = 0;
  else if (bufp > buf)
  {
    bufp[-1] = 0;
    return bufp - buf - 1;
  }
  return bufp - buf;
}
int DagorSafeArg::mixed_fprint_fmt(void *fp, const char *fmt, const void *valist_or_dsa, int dsa_num)
{
  if (dsa_num >= 0)
    return fprint_fmt(fp, fmt, (const DagorSafeArg *)valist_or_dsa, dsa_num);

  DAG_VACOPY(args_copy, *(va_list *)valist_or_dsa);
  int cnt = vfprintf((FILE *)fp, fmt, args_copy);
  DAG_VACOPYEND(args_copy);
  return cnt;
}
