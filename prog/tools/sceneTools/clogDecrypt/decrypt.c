// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

extern const unsigned char *get_log_crypt_key();

int main(int argc, char **argv)
{
  const unsigned char *key0 = get_log_crypt_key();

  int i, std_out = 0, sz, cur = 0;
  FILE *fp, *fp2 = NULL;
  char fp2_name[260] = {0};
  unsigned char *buf;

  fp = fopen(argv[1], "rb");
  if (!fp)
  {
    printf("can't open <%s> \n", argv[1]);
    return 1;
  }

  for (sz = 0; sz < argc && !std_out; ++sz)
    if (strstr(argv[sz], "-stdout"))
      std_out = 1;

  strcpy(fp2_name, argv[1]);
  buf = (unsigned char *)strchr(fp2_name, '.');
  if (buf)
    *buf = 0;
  strcat(fp2_name, ".txt");
  if (!std_out)
  {
    fp2 = fopen(fp2_name, "wb");
    if (!fp2)
    {
      fclose(fp);
      printf("can't open <%s> \n", argv[2]);
      return 1;
    }
  }

  buf = (unsigned char *)malloc(32 << 20); // 32MB buffer

  for (;;)
  {
    sz = fread(buf, 1, (32 << 20), fp);
    if (sz <= 0)
      break;
    for (i = 0; i < sz; i++)
    {
      buf[i] = buf[i] ^ key0[cur];
      if (cur == 127)
        cur = 0;
      else
        cur++;
    }
    if (std_out)
      puts((const char *)buf);
    else
      fwrite(buf, 1, sz, fp2);
  }

  fclose(fp);
  if (fp2)
    fclose(fp2);
  free(buf);

  return 0;
}
