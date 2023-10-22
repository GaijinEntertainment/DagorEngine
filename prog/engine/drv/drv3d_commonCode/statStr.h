#pragma once

#define STAT_STR_BEGIN(ADD)  \
  String ADD, stat_str_last; \
  int stat_str_last_cnt = 0

#define STAT_STR_APPEND(ADD, FINAL)                                                                          \
  do                                                                                                         \
  {                                                                                                          \
    if (!stat_str_last_cnt)                                                                                  \
      stat_str_last = eastl::move(ADD), stat_str_last_cnt = 1;                                               \
    else if (ADD == stat_str_last)                                                                           \
      stat_str_last_cnt++;                                                                                   \
    else                                                                                                     \
    {                                                                                                        \
      FINAL += stat_str_last_cnt > 1 ? String(0, "%dx%s", stat_str_last_cnt, stat_str_last) : stat_str_last; \
      stat_str_last = eastl::move(ADD), stat_str_last_cnt = 1;                                               \
    }                                                                                                        \
  } while (0)

#define STAT_STR_END(FINAL)                                                                                \
  do                                                                                                       \
  {                                                                                                        \
    FINAL += stat_str_last_cnt > 1 ? String(0, "%dx%s", stat_str_last_cnt, stat_str_last) : stat_str_last; \
    stat_str_last.clear();                                                                                 \
    stat_str_last_cnt = 0;                                                                                 \
  } while (0)
