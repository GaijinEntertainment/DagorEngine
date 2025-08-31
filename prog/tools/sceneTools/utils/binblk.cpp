// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_fileIo.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_string.h>
#include <libTools/util/signSha256.h>
#include <libTools/util/conLogWriter.h>

#include <stdio.h>
#if _TARGET_PC_WIN
#include <io.h>
#endif
#include <fcntl.h>
#include <stdlib.h>

namespace dblk
{
void save_to_bbf3(const DataBlock &blk, IGenSave &cwr);
}

static ConsoleLogWriter conlog;
static bool copy_shown = false;

static void init_dagor(int argc, char **argv, bool hideDebug);
static void shutdown_dagor();

static void show_usage();
static void show_copyright();

static bool printf_fatal_handler(const char *msg, const char * /*call_stack*/, const char * /*file*/, int /*line*/)
{
  printf("FATAL: %s\n", msg);
  exit(13);
}

//==============================================================================
int __cdecl main(int argc, char **argv)
{
  DataBlock::fatalOnMissingFile = false;
  DataBlock::fatalOnLoadFailed = false;
  DataBlock::fatalOnBadVarType = false;
  DataBlock::fatalOnMissingVar = false;
  DataBlock::parseIncludesAsParams = true;

  if (argc < 2)
  {
    ::show_copyright();
    ::show_usage();
    return 1;
  }

  const char *outFile = NULL;
  const char *sign_private_key = NULL;
  const char *verify_public_key = NULL;
  bool hideDebug = strcmp(argv[1], "-") == 0;
  bool validate_only = false;
  bool delSrc = false;
  int textToBin = -1;
  bool textCompact = false;
  bool noOverwrite = false;
  bool save_be = false;
  bool pack = false;
  bool save_json = false, json_unquoted = false;
  bool save_bbf3 = false;
  int json_ln_max_param = 4, json_ln_max_blk = 1;

  for (int i = 2; i < argc; ++i)
  {
    if (strcmp(argv[i], "-sign") == 0)
    {
      i++;
      if (i < argc)
        sign_private_key = argv[i];
      else
      {
        printf("WARNING: -sign <KEY> requires private key\n");
        ::shutdown_dagor();
        return 2;
      }
    }
    else if (strcmp(argv[i], "-verify") == 0)
    {
      i++;
      if (i < argc)
        verify_public_key = argv[i];
      else
      {
        printf("WARNING: -verify <KEY> requires public key\n");
        ::shutdown_dagor();
        return 2;
      }
    }
    else if (strcmp(argv[i], "-comments") == 0)
      DataBlock::parseCommentsAsParams = true;
    else if (strcmp(argv[i], "-final") == 0)
    {
      DataBlock::parseIncludesAsParams = false;
      DataBlock::parseCommentsAsParams = false;
    }
    else if (strcmp(argv[i], "-bbf3") == 0)
      save_bbf3 = true;
    else if (strncmp(argv[i], "-root:", 6) == 0)
    {
      DataBlock::setRootIncludeResolver(argv[i] + 6);
    }
    else if (strnicmp("-mount:", argv[i], 7) == 0)
    {
      if (const char *p = strchr(argv[i] + 7, '='))
        dd_set_named_mount_path(String::mk_sub_str(argv[i] + 7, p), p + 1);
      else
      {
        printf("ERR: bad mount option: %s (expected name=dir format)\n\n", argv[i]);
        return 13;
      }
    }
    else if (strnicmp("-addpath:", argv[i], 9) == 0)
      continue;
    else if (argv[i][0] == '-')
    {
      switch (argv[i][1])
      {
        case 'h':
        case 'H': hideDebug = true; break;

        case 'd':
        case 'D': delSrc = true; break;

        case 'b':
        case 'B': textToBin = 1; break;

        case 't':
        case 'T':
          textToBin = 0;
          textCompact = argv[i][2] == '0';
          break;

        case 's':
        case 'S': noOverwrite = true; break;

        case 'x':
        case 'X': printf("ERROR: %s: big endian format not supported!\n", argv[i]); break;

        case 'z':
          textToBin = 1;
          pack = true;
          break;
        case 'Z':
          printf("ERROR: %s: big endian format not supported!\n", argv[i]);
          textToBin = 1;
          pack = true;
          break;

        case 'J': json_unquoted = true;
        case 'j':
          textToBin = 0;
          save_json = true;
          if (argv[i][2] == ':')
          {
            json_ln_max_param = atoi(argv[i] + 3);
            if (const char *p = strchr(argv[i] + 3, ':'))
              json_ln_max_blk = atoi(p + 1);
          }
          break;

        case 'v':
          copy_shown = true;
          validate_only = true;
          ::dgs_execute_quiet = true;
          ::dgs_fatal_handler = &printf_fatal_handler;
          break;

        case 0:
          hideDebug = true;
          if (!outFile)
            outFile = argv[i];
          break;

        default:
          if (!copy_shown && !hideDebug)
            ::show_copyright();
          printf("WARNING: Unknown parameter \"%s\"\n", argv[i]);
      }
    }
    else
    {
      if (!outFile)
        outFile = argv[i];
      else
      {
        if (!copy_shown && !hideDebug)
          ::show_copyright();

        printf("WARNING: Unknown parameter \"%s\"\n", argv[i]);
      }
    }
  }

  if (!copy_shown && !hideDebug)
    ::show_copyright();

  if (!textToBin && sign_private_key)
  {
    printf("FATAL: Unable to sign BLK in text format\n");
    ::shutdown_dagor();
    return 2;
  }

  ::init_dagor(argc, argv, hideDebug);

  if (verify_public_key && !sign_private_key)
  {
    if (strcmp(argv[1], "-") == 0)
    {
      printf("FATAL: Unable to verify signature of text BLK from stdin\n");
      ::shutdown_dagor();
      return 2;
    }

    if (!verify_digital_signature(argv[1], verify_public_key, conlog))
    {
      printf("FATAL: \"%s\" signature verification failed\n", argv[1]);
      ::shutdown_dagor();
      return 2;
    }
  }

  DataBlock blk;
  file_ptr_t fp = strcmp(argv[1], "-") == 0 ? (file_ptr_t)stdin : df_open(argv[1], DF_READ | DF_IGNORE_MISSING);
#if _TARGET_PC_WIN
  if (fp == (file_ptr_t)stdin)
    _setmode(fileno(stdin), _O_BINARY);
#endif
  if (textToBin == -1 && fp == (file_ptr_t)stdin)
    textToBin = 1;
  if (!fp)
  {
    printf("FATAL: Unable to load input file \"%s\"\n", argv[1]);
    ::shutdown_dagor();
    return 1;
  }
  LFileGeneralLoadCB crd(fp);

  struct Rep : DataBlock::IErrorReporterPipe
  {
    void reportError(const char *error_text, bool serious_err) override { printf("ERR: %s\n", error_text); }
  } rep;
  DataBlock::InstallReporterRAII repRaii(&rep);

  if (!blk.loadFromStream(crd))
  {
    printf("FATAL: Failed to load BLK from \"%s\"\n", argv[1]);
    if (validate_only && fp != stdin)
    {
      crd.seekto(0);
      DataBlock::fatalOnLoadFailed = true;
      DataBlock::fatalOnBadVarType = true;
      DataBlock::fatalOnMissingVar = true;
      blk.loadFromStream(crd, argv[1]);
    }
    ::shutdown_dagor();
    return 1;
  }
  if (validate_only)
  {
    ::shutdown_dagor();
    return 0;
  }
  if (textToBin == -1)
  {
    unsigned hdr[2] = {0, 0};
    crd.seekto(0);
    if (crd.tryRead(hdr, sizeof(hdr)) == sizeof(hdr))
    {
      if (hdr[0] == _MAKE4C('BBF') || hdr[0] == _MAKE4C('BBZ') || hdr[0] == _MAKE4C('BBz'))
        textToBin = 0;
      else if (hdr[0] == _MAKE4C('SB') && hdr[1] == _MAKE4C('blk'))
        textToBin = 1;
    }

    if (textToBin == -1 && (hdr[0] & 0xFF) >= 1 && (hdr[0] & 0xFF) <= 5) // new BLK2 binary formats
      textToBin = 0;
    if (textToBin == -1)
      textToBin = 1;
  }

  if (fp != (file_ptr_t)stdin)
    df_close(fp);

  String outPath;

  if (outFile)
    outPath = String(outFile);
  else if (fp == stdin)
    outPath = "-";
  else
  {
    if (textToBin)
      outPath = String(256, "%s.bin", argv[1]);
    else
    {
      const char *ext = ::dd_get_fname_ext(argv[1]);
      if (!stricmp(ext, "bin"))
      {
        outPath = String(argv[1]);
        outPath.resize(ext - argv[1]);
        outPath[outPath.length()] = 0;
      }
      else if (save_json)
        outPath = String(256, "%s.json", argv[1]);
      else
        outPath = String(256, "%s.blk", argv[1]);
    }
  }

  if (::dd_file_exist(outPath) && noOverwrite)
  {
    printf("WARNING: file \"%s\" already exists, skip conversion\n", (const char *)outPath);

    if (delSrc)
      printf("WARNING: -d option was blocked to prevent loss of sorce file \"%s\"", argv[1]);
  }
  else
  {
    bool result = false;

    fp = strcmp(outPath, "-") == 0 ? (file_ptr_t)stdout : df_open(outPath, DF_CREATE | DF_WRITE);
#if _TARGET_PC_WIN
    if (fp == (file_ptr_t)stdout)
      _setmode(fileno(stdout), _O_BINARY);
#endif
    LFileGeneralSaveCB cwr(fp);

    if (textToBin)
    {
      if (pack && !save_bbf3)
        dblk::pack_to_stream(blk, cwr);
      else if (!save_bbf3)
        blk.saveToStream(cwr);
      else
        dblk::save_to_bbf3(blk, cwr);
      result = true;
    }
    else if (save_json)
      result = dblk::export_to_json_text_stream(blk, cwr, json_unquoted, json_ln_max_param, json_ln_max_blk);
    else
      result = textCompact ? blk.saveToTextStreamCompact(cwr) : blk.saveToTextStream(cwr);

    if (fp != (file_ptr_t)stdout)
      df_close(fp);

    if (!result)
    {
      printf("FATAL: unable to save file \"%s\"\n", (const char *)outPath);
      ::shutdown_dagor();
      return 1;
    }
    else if (!hideDebug)
      printf("File \"%s\" successfully saved\n", (const char *)outPath);
    if (sign_private_key)
    {
      if (!add_digital_signature(outPath, sign_private_key, conlog))
      {
        ::dd_erase(outPath);
        ::shutdown_dagor();
        return 1;
      }
      if (verify_public_key)
        if (!verify_digital_signature(outPath, verify_public_key, conlog))
        {
          printf("FATAL: \"%s\" signature post-verification failed\n", outPath.str());
          ::dd_erase(outPath);
          ::shutdown_dagor();
          return 1;
        }
    }

    if (delSrc)
      ::dd_erase(argv[1]);
  }

  ::shutdown_dagor();
  return 0;
}


//==============================================================================
static void init_dagor(int argc, char **argv, bool hideDebug)
{
  ::dd_init_local_conv();
  ::dd_add_base_path("");
  for (int i = 2; i < argc; ++i)
    if (strnicmp("-addpath:", argv[i], 9) == 0)
    {
      char buf[DAGOR_MAX_PATH] = {0};
      strncpy(buf, argv[i] + 9, DAGOR_MAX_PATH);
      dd_append_slash_c(buf);
      dd_simplify_fname_c(buf);
      dd_add_base_path(buf);
      if (!hideDebug)
        printf("NOTE: using basepath: %s\n", buf);
    }
}


//==============================================================================
static void shutdown_dagor() {}


//==============================================================================
static void show_usage()
{
  printf("usage: binblk <file name> [output file] [options]\n");
  printf("where\n");
  printf("file name\tpath to BLK file (or - for stdin)\n");
  printf("output file\tpath to otput BLK file (or - for stdout)\n");
  printf("options\t\tadditional options:\n");
  printf("\t\t-b  convert text BLK to binary format (default)\n");
  printf("\t\t-t  convert binary BLK to text format\n");
  printf("\t\t-t0 convert binary BLK to text format (compact)\n");
  printf("\t\t-d  delete source file\n");
  printf("\t\t-s  do not overwrite existing files\n");
  printf("\t\t-x  write binary BLK in Big Endian format\n");
  printf("\t\t-z  write packed binary BLK in Little Endian format\n");
  printf("\t\t-Z  write packed binary BLK in Big Endian format\n");
  printf("\t\t-v  validate input file only\n");
  printf("\t\t-h  hide copyright and log messages\n");
  printf("\t\t-sign <private_key_fname>    sign binary BLK with SHA256 digest\n");
  printf("\t\t-verify <public_key_fname>   verify SHA256 signature of binary BLK\n");
  printf("\t\t-comments    preserve comments\n");
  printf("\t\t-final       resolve includes and strip comments\n");
  printf("\t\t-bbf3        save binary using legacy BBF\\3 binary format\n");
  printf("\t\t-root:<dir>  set root dir for includes\n");
  printf("\t\t-mount:mnt=dir  add named mount (will be used to resolve includes with named mounts)\n");
  printf("\t\t-addpath:<dir>  add base path (will be used to search files)\n");
}


//==============================================================================
static void show_copyright()
{
  printf("BLK file converter & validator\n");
  printf("Copyright (c) Gaijin Games KFT, 2023\n");

  copy_shown = true;
}
