#include <ioSys/dag_fileIo.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_debug.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <ctype.h>

#define LANG_JAP_N 1
#define LANG_JAP_T "japanese"

#define LANG_CHN_N 2
#define LANG_CHN_T "chinese"


extern const char *process_japanese_string(const char *str, wchar_t sep_char = '\t');
extern const char *process_chinese_string(const char *str, wchar_t sep_char = '\t');

void print_usage()
{
  printf("Writing Systems Operating System v1.0\n");
  printf("usage: wsos <source> <destination> <language>\n");
  printf("language: " LANG_JAP_T " " LANG_CHN_T "\n\n");
}

static void __cdecl ctrl_break_handler(int) { quit_game(0); }


int DagorWinMain(bool debugmode)
{
  if (dgs_argc < 4)
  {
    print_usage();
    return -1;
  }
  signal(SIGINT, ctrl_break_handler);

  const char *langArg = dgs_argv[3];
  const char *sourceArg = dgs_argv[1];
  const char *destArg = dgs_argv[2];

  int lang = 0;
  if (!stricmp(langArg, LANG_CHN_T))
    lang = LANG_CHN_N;
  if (!stricmp(langArg, LANG_JAP_T))
    lang = LANG_JAP_N;

  if (!lang)
  {
    printf("wsos: Invalid language: '%s'\n", langArg);
    return -1;
  }

  file_ptr_t f = df_open(sourceArg, DF_READ | DF_IGNORE_MISSING);
  if (!f)
  {
    printf("wsos: Cannot open file '%s'\n", sourceArg);
    return -1;
  }

  int len = df_length(f);
  Tab<char> data;
  data.resize(len + 1);
  bool ok = df_read(f, data.data(), len) == len;
  data[len] = 0;
  df_close(f);

  if (!ok)
  {
    printf("wsos: Error reading data from '%s'\n", sourceArg);
    return -1;
  }

  const char *output = NULL;

  if (lang == LANG_JAP_N)
    output = process_japanese_string(&data[0]);

  if (lang == LANG_CHN_N)
    output = process_chinese_string(&data[0]);

  G_ASSERT(output != NULL);

  f = df_open(destArg, DF_WRITE | DF_CREATE);
  if (!f)
  {
    printf("wsos: Cannot write to file '%s'\n", destArg);
    return -1;
  }

  df_write(f, output, int(strlen(output)));
  df_close(f);

  return 0;
}
