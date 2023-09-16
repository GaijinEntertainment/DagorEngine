#include <util/dag_baseDef.h>
#include <util/dag_globDef.h>
#include <stdio.h>
#include <ctime>
#include <cmath>
#include <cstring>

#if _TARGET_C1 | _TARGET_C2

#endif


static bool read_int(const char *&p, const char *const end, int digits, int &result)
{
  result = 0;
  if ((end - p) < digits)
    return false;
  for (int i = 0; i < digits; i++)
  {
    if (p[i] < '0' || p[i] > '9')
      return false;
    result = result * 10 + (p[i] - '0');
  }
  p += digits;
  return true;
}


// taken from RFC 3339
static bool is_leap_year(int year) { return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)); }


static const int days_per_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static bool is_valid_day(int year, int month, int day)
{
  if (day < 1)
    return false;
  if (month < 1 || month > 12)
    return false;
  const bool naiveResult = day <= days_per_month[month - 1];
  if (naiveResult)
    return true;
  return day == 29 && month == 2 && is_leap_year(year);
}


static bool iso8601_parse_date(const char *&p, const char *end, int &year, int &month, int &day)
{
  bool haveDashes = false;
  bool dayIsRequired = false;

  if (!read_int(p, end, 4, year))
    return false;

  month = 1;
  day = 1;

  if ((end - p) > 0)
  {
    if (*p == '-')
    {
      ++p;
      haveDashes = true;
    }
    else if (*p >= '0' && *p <= '9')
    {
      // YYYYMMDD is ok but YYYYMM is invalid because 6 digits must be interpreted as hhmmss according to iso8601.
      // We do not support time of day without date anyway though, so 6 digits is an invalid timestamp.
      dayIsRequired = true;
    }
  }

  if ((end - p) <= 0 || *p == 'T')
    return !haveDashes;

  if (!read_int(p, end, 2, month))
    return false;
  if (month < 1 || month > 12)
    return false;

  if ((end - p) > 0 && haveDashes && *p != 'T')
  {
    if (*p != '-')
      return false;
    ++p;
    dayIsRequired = true;
  }

  if ((end - p) <= 0 || *p == 'T')
    return !dayIsRequired;

  if (!read_int(p, end, 2, day))
    return false;

  return is_valid_day(year, month, day);
}


static bool parse_tz_offset(const char *&p, const char *end, int &offsetSec)
{
  if ((end - p) < 1)
    return false;

  if (*p == 'Z')
  {
    ++p;
    offsetSec = 0;
    return (end - p) == 0;
  }

  bool isNeg = false;
  int offHours = 0, offMinutes = 0;
  if (*p == '-')
  {
    isNeg = true;
    ++p;
  }
  else if (*p == '+')
  {
    ++p;
  }
  else
  {
    return false;
  }

  if (!read_int(p, end, 2, offHours))
    return false;
  if (offHours > 14)
    return false;

  if ((end - p) >= 2)
  {
    if (*p == ':')
      ++p;
    if (!read_int(p, end, 2, offMinutes))
      return false;
  }
  if (offMinutes > 59)
    return false;

  offsetSec = (offHours * 60 + offMinutes) * 60;
  if (isNeg)
    offsetSec = -offsetSec;

  return (end - p) == 0;
}


static bool parse_millis(const char *&p, const char *end, int &millis)
{
  millis = 0;
  int digit = 0;
  int digitsRead = 0;
  int multiplier = 100;

  while (read_int(p, end, 1, digit))
  {
    ++digitsRead;
    millis += digit * multiplier;
    multiplier /= 10;
  }

  return digitsRead > 0;
}


static bool parse_from_seconds(const char *&p, const char *end, int &second, int &millis, int &offsetSec)
{
  if (!read_int(p, end, 2, second))
    return false;

  bool leap = false;
  if (second > 59)
  {
    if (second > 60)
      return false;
    // skip possible leap seconds
    leap = true;
    second = 59;
    millis = 999;
  }

  if ((end - p) > 0)
  {
    if (*p == '+' || *p == '-' || *p == 'Z')
      return parse_tz_offset(p, end, offsetSec);

    if (*p == '.' || *p == ',')
    {
      ++p;
      if (!parse_millis(p, end, millis))
        return false;
      if (leap)
        millis = 999;
    }

    if ((end - p) > 0)
    {
      if (*p == '+' || *p == '-' || *p == 'Z')
        return parse_tz_offset(p, end, offsetSec);
      return false;
    }
  }

  return true;
}


static bool parse_from_minutes(const char *&p, const char *end, bool haveColons, int &minute, int &second, int &millis, int &offsetSec)
{
  if (!read_int(p, end, 2, minute))
    return false;
  if (minute > 59)
    return false;

  if ((end - p) > 0)
  {
    if (*p == '+' || *p == '-' || *p == 'Z')
      return parse_tz_offset(p, end, offsetSec);
    if (haveColons)
    {
      if (*p != ':')
        return false;
      ++p;
    }

    return parse_from_seconds(p, end, second, millis, offsetSec);
  }

  return true;
}


static bool iso8601_parse_time(const char *&p, const char *end, int &hour, int &minute, int &second, int &millis, int &offsetSec)
{
  hour = minute = second = millis = offsetSec = 0;

  if ((end - p) < 2)
    return false;
  if (!read_int(p, end, 2, hour))
    return false;
  if (hour > 23)
    return false;

  if ((end - p) > 0)
  {
    if (*p == '+' || *p == '-' || *p == 'Z')
      return parse_tz_offset(p, end, offsetSec);

    bool haveColons = false;
    if (*p == ':')
    {
      haveColons = true;
      ++p;
    }
    return parse_from_minutes(p, end, haveColons, minute, second, millis, offsetSec);
  }

  return true;
}


long long iso8601_parse(const char *str, size_t len)
{
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;
  int millis = 0;
  int offsetSec = 0;
  const char *const end = str + len;
  const char *p = str;

  if (!iso8601_parse_date(p, end, year, month, day))
    return 0;

  bool reqTime = false;
  if ((end - p) > 0)
  {
    if (*p != 'T')
      return 0;
    ++p;
    reqTime = true;
  }

  if (reqTime && !iso8601_parse_time(p, end, hour, minute, second, millis, offsetSec))
    return 0;

  struct tm t;
  memset(&t, 0, sizeof(t));
  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = minute;
  t.tm_sec = second;
#if _TARGET_PC_WIN | _TARGET_XBOX
  time_t timestampSec = ::_mkgmtime(&t);
#else
  time_t timestampSec = ::timegm(&t);
#endif

  if (timestampSec < 0)
    return 0;

  // RCF3339: For example, 18:50:00-04:00 is the same time as 22:50:00Z
  timestampSec -= offsetSec;

  return (long long)timestampSec * 1000LL + millis;
}


long long iso8601_parse(const char *str) { return iso8601_parse(str, strlen(str)); }


void iso8601_format(long long timeMsec, char *buffer, size_t N)
{
  if (timeMsec <= 0)
  {
    SNPRINTF(buffer, N, "%s", "1970-01-01T00:00:00Z");
    return;
  }

  time_t timeSec = (time_t)(timeMsec / 1000LL);
  int msecPart = (int)(timeMsec % 1000LL);

  struct tm tmout;
#if _TARGET_PC_WIN || _TARGET_XBOX
  gmtime_s(&tmout, &timeSec);
#elif _TARGET_C1 | _TARGET_C2

#else
  gmtime_r(&timeSec, &tmout);
#endif

  if (msecPart)
  {
    SNPRINTF(buffer, N, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", tmout.tm_year + 1900, tmout.tm_mon + 1, tmout.tm_mday, tmout.tm_hour,
      tmout.tm_min, tmout.tm_sec, msecPart);
  }
  else
  {
    SNPRINTF(buffer, N, "%04d-%02d-%02dT%02d:%02d:%02dZ", tmout.tm_year + 1900, tmout.tm_mon + 1, tmout.tm_mday, tmout.tm_hour,
      tmout.tm_min, tmout.tm_sec);
  }
}
