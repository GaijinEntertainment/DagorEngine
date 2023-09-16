#include <util/dag_hierBitArray.h>
#include <osApiWrappers/dag_unicode.h>
#include <debug/dag_logSys.h>

#define __UNLIMITED_BASE_PATH 1
#include <startup/dag_mainCon.inc.cpp>
#include <stdio.h>
#include <stdlib.h>

typedef HierConstSizeBitArray<8, ConstSizeBitArray<8>> unicode_range_t;

int DagorWinMain(bool debugmode)
{
  debug_enable_timestamps(false);
  printf("UTF8 text range detectors\n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");

  if (dgs_argc < 2)
  {
    printf("\nUsage: utf8ranges-dev.exe arg.utf8.txt [arg1.utf8.txt...]\n");
    return 1;
  }

  unicode_range_t urange;
  Tab<char> utf8(tmpmem);
  Tab<wchar_t> utf16(tmpmem);

  for (int i = 1; i < dgs_argc; i++)
  {
    FILE *fp = fopen(dgs_argv[i], "rb");
    if (fp)
    {
      fseek(fp, 0, SEEK_END);
      utf8.resize(ftell(fp));
      fseek(fp, 0, SEEK_SET);
      utf8.resize(fread(utf8.data(), 1, utf8.size(), fp));
      fclose(fp);

      utf16.resize(utf8.size());

#if _TARGET_PC_WIN
      int used = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), utf8.size(), utf16.data(), utf16.size());
      G_ASSERT(used <= utf16.size());
      utf16.resize(used);
#else
      G_VERIFY(utf8_to_wcs(utf8.data(), utf16.data(), utf16.size()));
#endif

      for (int i = 0; i < utf16.size(); i++)
        urange.set(utf16[i]);
    }
    else
      printf("ERR: cant open utf8 file <%s>\n", dgs_argv[i]);
  }

  for (int gi = 0; gi < 256; gi++)
  {
    int si = 0;
    while (si < 256)
      if (urange.get(gi * 256 + si))
      {
        printf("u%02Xxx\n", gi);
        break;
      }
      else
        si++;
  }
  return 0;
}
