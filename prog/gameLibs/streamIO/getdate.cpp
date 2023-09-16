
#include <string.h>
#include <cstdio>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

// take source code here
// https://github.com/curl/curl/lib/parsedate.c

#if (_TARGET_APPLE || _TARGET_C3 || _TARGET_XBOX || _TARGET_C1 || _TARGET_C2)
#include <time.h>
#endif

namespace streamio
{

enum Assume
{
  DATE_MDAY,
  DATE_YEAR,
  DATE_TIME
};

/* this is a clone of 'struct tm' but with all fields we don't need or use
   cut out */
struct MyTm
{
  int tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;
  int tm_year; /* full year */
};

#define PARSEDATE_OK     0
#define PARSEDATE_FAIL   -1
#define PARSEDATE_LATER  1
#define PARSEDATE_SOONER 2

static void skip(const char **date)
{
  /* skip everything that aren't letters or digits */
  while (**date && !isalnum(**date))
    (*date)++;
}

const char *const weekday_short[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

static const char *const weekday[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};

static int checkday(const char *check, size_t len)
{
  int i;
  const char *const *what;
  bool found = false;
  if (len > 3)
    what = &weekday[0];
  else
    what = &weekday_short[0];
  for (i = 0; i < 7; i++)
  {
    if (stricmp(check, what[0]) == 0)
    {
      found = true;
      break;
    }
    what++;
  }
  return found ? i : -1;
}

const char *const month_short[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static int checkmonth(const char *check)
{
  int i;
  const char *const *what;
  bool found = false;

  what = &month_short[0];
  for (i = 0; i < 12; i++)
  {
    if (stricmp(check, what[0]) == 0)
    {
      found = true;
      break;
    }
    what++;
  }
  return found ? i : -1; /* return the offset or -1, no real offset is -1 */
}

struct TZinfo
{
  char name[5];
  int offset; /* +/- in minutes */
};

/* Here's a bunch of frequently used time zone names. These were supported
   by the old getdate parser. */
#define T_DAYZONE -60 /* offset for daylight savings time */
static const TZinfo tz[] = {
  {"GMT", 0},               /* Greenwich Mean */
  {"UT", 0},                /* Universal Time */
  {"UTC", 0},               /* Universal (Coordinated) */
  {"WET", 0},               /* Western European */
  {"BST", 0 T_DAYZONE},     /* British Summer */
  {"WAT", 60},              /* West Africa */
  {"AST", 240},             /* Atlantic Standard */
  {"ADT", 240 T_DAYZONE},   /* Atlantic Daylight */
  {"EST", 300},             /* Eastern Standard */
  {"EDT", 300 T_DAYZONE},   /* Eastern Daylight */
  {"CST", 360},             /* Central Standard */
  {"CDT", 360 T_DAYZONE},   /* Central Daylight */
  {"MST", 420},             /* Mountain Standard */
  {"MDT", 420 T_DAYZONE},   /* Mountain Daylight */
  {"PST", 480},             /* Pacific Standard */
  {"PDT", 480 T_DAYZONE},   /* Pacific Daylight */
  {"YST", 540},             /* Yukon Standard */
  {"YDT", 540 T_DAYZONE},   /* Yukon Daylight */
  {"HST", 600},             /* Hawaii Standard */
  {"HDT", 600 T_DAYZONE},   /* Hawaii Daylight */
  {"CAT", 600},             /* Central Alaska */
  {"AHST", 600},            /* Alaska-Hawaii Standard */
  {"NT", 660},              /* Nome */
  {"IDLW", 720},            /* International Date Line West */
  {"CET", -60},             /* Central European */
  {"MET", -60},             /* Middle European */
  {"MEWT", -60},            /* Middle European Winter */
  {"MEST", -60 T_DAYZONE},  /* Middle European Summer */
  {"CEST", -60 T_DAYZONE},  /* Central European Summer */
  {"MESZ", -60 T_DAYZONE},  /* Middle European Summer */
  {"FWT", -60},             /* French Winter */
  {"FST", -60 T_DAYZONE},   /* French Summer */
  {"EET", -120},            /* Eastern Europe, USSR Zone 1 */
  {"WAST", -420},           /* West Australian Standard */
  {"WADT", -420 T_DAYZONE}, /* West Australian Daylight */
  {"CCT", -480},            /* China Coast, USSR Zone 7 */
  {"JST", -540},            /* Japan Standard, USSR Zone 8 */
  {"EAST", -600},           /* Eastern Australian Standard */
  {"EADT", -600 T_DAYZONE}, /* Eastern Australian Daylight */
  {"GST", -600},            /* Guam Standard, USSR Zone 9 */
  {"NZT", -720},            /* New Zealand */
  {"NZST", -720},           /* New Zealand Standard */
  {"NZDT", -720 T_DAYZONE}, /* New Zealand Daylight */
  {"IDLE", -720},           /* International Date Line East */
  /* Next up: Military timezone names. RFC822 allowed these, but (as noted in
     RFC 1123) had their signs wrong. Here we use the correct signs to match
     actual military usage.
   */
  {"A", 1 * 60}, /* Alpha */
  {"B", 2 * 60}, /* Bravo */
  {"C", 3 * 60}, /* Charlie */
  {"D", 4 * 60}, /* Delta */
  {"E", 5 * 60}, /* Echo */
  {"F", 6 * 60}, /* Foxtrot */
  {"G", 7 * 60}, /* Golf */
  {"H", 8 * 60}, /* Hotel */
  {"I", 9 * 60}, /* India */
  /* "J", Juliet is not used as a timezone, to indicate the observer's local
     time */
  {"K", 10 * 60},  /* Kilo */
  {"L", 11 * 60},  /* Lima */
  {"M", 12 * 60},  /* Mike */
  {"N", -1 * 60},  /* November */
  {"O", -2 * 60},  /* Oscar */
  {"P", -3 * 60},  /* Papa */
  {"Q", -4 * 60},  /* Quebec */
  {"R", -5 * 60},  /* Romeo */
  {"S", -6 * 60},  /* Sierra */
  {"T", -7 * 60},  /* Tango */
  {"U", -8 * 60},  /* Uniform */
  {"V", -9 * 60},  /* Victor */
  {"W", -10 * 60}, /* Whiskey */
  {"X", -11 * 60}, /* X-ray */
  {"Y", -12 * 60}, /* Yankee */
  {"Z", 0},        /* Zulu, zero meridian, a.k.a. UTC */
};

/* return the time zone offset between GMT and the input one, in number
   of seconds or -1 if the timezone wasn't found/legal */

static int checktz(const char *check)
{
  unsigned int i;
  const TZinfo *what;
  bool found = false;

  what = tz;
  for (i = 0; i < sizeof(tz) / sizeof(tz[0]); i++)
  {
    if (stricmp(check, what->name) == 0)
    {
      found = true;
      break;
    }
    what++;
  }
  return found ? what->offset * 60 : -1;
}


/* struct tm to time since epoch in GMT time zone.
 * This is similar to the standard mktime function but for GMT only, and
 * doesn't suffer from the various bugs and portability problems that
 * some systems' implementations have.
 *
 * Returns 0 on success, otherwise non-zero.
 */
static void my_timegm(MyTm *tm, time_t *t)
{
  static const int month_days_cumulative[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  int month, year, leap_days;

  year = tm->tm_year;
  month = tm->tm_mon;
  if (month < 0)
  {
    year += (11 - month) / 12;
    month = 11 - (11 - month) % 12;
  }
  else if (month >= 12)
  {
    year -= month / 12;
    month = month % 12;
  }

  leap_days = year - (tm->tm_mon <= 1);
  leap_days = ((leap_days / 4) - (leap_days / 100) + (leap_days / 400) - (1969 / 4) + (1969 / 100) - (1969 / 400));

  *t = ((((time_t)(year - 1970) * 365 + leap_days + month_days_cumulative[month] + tm->tm_mday - 1) * 24 + tm->tm_hour) * 60 +
         tm->tm_min) *
         60 +
       tm->tm_sec;
}

/*
 * parsedate()
 *
 * Returns:
 *
 * PARSEDATE_OK     - a fine conversion
 * PARSEDATE_FAIL   - failed to convert
 * PARSEDATE_LATER  - time overflow at the far end of time_t
 * PARSEDATE_SOONER - time underflow at the low end of time_t
 */
static int parsedate(const char *date, time_t *output)
{
  time_t t = 0;
  int wdaynum = -1; /* day of the week number, 0-6 (mon-sun) */
  int monnum = -1;  /* month of the year number, 0-11 */
  int mdaynum = -1; /* day of month, 1 - 31 */
  int hournum = -1;
  int minnum = -1;
  int secnum = -1;
  int yearnum = -1;
  int tzoff = -1;
  MyTm tm;
  Assume dignext = DATE_MDAY;
  const char *indate = date; /* save the original pointer */
  int part = 0;              /* max 6 parts */

  while (*date && (part < 6))
  {
    bool found = false;

    skip(&date);

    if (isalpha(*date))
    {
      /* a name coming up */
      char buf[32] = "";
      size_t len;
      if (sscanf(date,
            "%31[ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz]",
            buf))
        len = strlen(buf);
      else
        len = 0;

      if (wdaynum == -1)
      {
        wdaynum = checkday(buf, len);
        if (wdaynum != -1)
          found = true;
      }

      if (!found && (monnum == -1))
      {
        monnum = checkmonth(buf);
        if (monnum != -1)
          found = true;
      }

      if (!found && (tzoff == -1))
      {
        /* this just must be a time zone string */
        tzoff = checktz(buf);
        if (tzoff != -1)
          found = true;
      }

      if (!found)
        return PARSEDATE_FAIL; /* bad string */

      date += len;
    }
    else if (isdigit(*date))
    {
      /* a digit */
      int val;
      char *end;
      int len = 0;
      if ((secnum == -1) && (3 == sscanf(date, "%02d:%02d:%02d%n", &hournum, &minnum, &secnum, &len)))
      {
        /* time stamp! */
        date += len;
      }
      else if ((secnum == -1) && (2 == sscanf(date, "%02d:%02d%n", &hournum, &minnum, &len)))
      {
        /* time stamp without seconds */
        date += len;
        secnum = 0;
      }
      else
      {
        long lval;
        int error;
        int old_errno;

        old_errno = errno;
        errno = 0;
        lval = strtol(date, &end, 10);
        error = errno;
        if (errno != old_errno)
          errno = old_errno;

        if (error)
          return PARSEDATE_FAIL;

#if LONG_MAX != INT_MAX
        if ((lval > (long)INT_MAX) || (lval < (long)INT_MIN))
          return PARSEDATE_FAIL;
#endif

        val = static_cast<int>(lval);

        if ((tzoff == -1) && ((end - date) == 4) && (val <= 1400) && (indate < date) && ((date[-1] == '+' || date[-1] == '-')))
        {
          /* four digits and a value less than or equal to 1400 (to take into
             account all sorts of funny time zone diffs) and it is preceded
             with a plus or minus. This is a time zone indication.  1400 is
             picked since +1300 is frequently used and +1400 is mentioned as
             an edge number in the document "ISO C 200X Proposal: Timezone
             Functions" at http://david.tribble.com/text/c0xtimezone.html If
             anyone has a more authoritative source for the exact maximum time
             zone offsets, please speak up! */
          found = true;
          tzoff = (val / 100 * 60 + val % 100) * 60;

          /* the + and - prefix indicates the local time compared to GMT,
             this we need their reversed math to get what we want */
          tzoff = date[-1] == '+' ? -tzoff : tzoff;
        }

        if (((end - date) == 8) && (yearnum == -1) && (monnum == -1) && (mdaynum == -1))
        {
          /* 8 digits, no year, month or day yet. This is YYYYMMDD */
          found = true;
          yearnum = val / 10000;
          monnum = (val % 10000) / 100 - 1; /* month is 0 - 11 */
          mdaynum = val % 100;
        }

        if (!found && (dignext == DATE_MDAY) && (mdaynum == -1))
        {
          if ((val > 0) && (val < 32))
          {
            mdaynum = val;
            found = true;
          }
          dignext = DATE_YEAR;
        }

        if (!found && (dignext == DATE_YEAR) && (yearnum == -1))
        {
          yearnum = val;
          found = true;
          if (yearnum < 100)
          {
            if (yearnum > 70)
              yearnum += 1900;
            else
              yearnum += 2000;
          }
          if (mdaynum == -1)
            dignext = DATE_MDAY;
        }

        if (!found)
          return PARSEDATE_FAIL;

        date = end;
      }
    }

    part++;
  }

  if (-1 == secnum)
    secnum = minnum = hournum = 0; /* no time, make it zero */

  if ((-1 == mdaynum) || (-1 == monnum) || (-1 == yearnum))
    /* lacks vital info, fail */
    return PARSEDATE_FAIL;

  if (yearnum < 1970)
  {
    /* only positive numbers cannot return earlier */
    *output = 0; // TIME_T_MIN
    return PARSEDATE_SOONER;
  }

  if ((mdaynum > 31) || (monnum > 11) || (hournum > 23) || (minnum > 59) || (secnum > 60))
    return PARSEDATE_FAIL; /* clearly an illegal date */

  tm.tm_sec = secnum;
  tm.tm_min = minnum;
  tm.tm_hour = hournum;
  tm.tm_mday = mdaynum;
  tm.tm_mon = monnum;
  tm.tm_year = yearnum;

  /* my_timegm() returns a time_t. time_t is often 32 bits, sometimes even on
     architectures that feature 64 bit 'long' but ultimately time_t is the
     correct data type to use.
  */
  my_timegm(&tm, &t);

  /* Add the time zone diff between local time zone and GMT. */
  if (tzoff == -1)
    tzoff = 0;

  if ((tzoff > 0) && (t > UINT_MAX - tzoff))
  {
    *output = UINT_MAX;
    return PARSEDATE_LATER; /* time_t overflow */
  }

  t += tzoff;

  *output = t;

  return PARSEDATE_OK;
}

time_t getdate(const char *p, const time_t *now)
{
  time_t parsed = -1;
  int rc = parsedate(p, &parsed);
  (void)now; /* legacy argument from the past that we ignore */

  if (rc == PARSEDATE_OK)
  {
    if (parsed == -1)
      /* avoid returning -1 for a working scenario */
      parsed++;
    return parsed;
  }
  /* everything else is fail */
  return -1;
}

} // namespace streamio
