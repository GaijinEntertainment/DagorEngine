// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>

// ############################################################################
// ##                                                                        ##
// ##  MAIN.CPP                                                              ##
// ##                                                                        ##
// ##  Console APP to load Q3BSP files, interpret them, organize them into   ##
// ##  a triangle mesh, and save them out in various ASCII file formats.     ##
// ##                                                                        ##
// ##  OpenSourced 12/5/2000 by John W. Ratcliff                             ##
// ##                                                                        ##
// ##  No warranty expressed or implied.                                     ##
// ##                                                                        ##
// ##  Part of the Q3BSP project, which converts a Quake 3 BSP file into a   ##
// ##  polygon mesh.                                                         ##
// ############################################################################
// ##                                                                        ##
// ##  Contact John W. Ratcliff at jratcliff@verant.com                      ##
// ############################################################################

#include "q3bsp.h"

int DagorWinMain(bool debugmode)
{
  dd_add_base_path("");
  printf("Dagor Quake to DAG \n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");

  // Usage: q3bsp <name>.BSP
  if (__argc < 2)
  {
    printf("Usage: q3bsp <name>.BSP [/lm]\n");
    printf("Where <name> is the name of a valid Quake3 BSP file.\n\n");
    printf("This utility will convert a Quake3 BSP into a valid\n");
    printf("polygon mesh and output the results into DAG file.\n");

    printf("If [/lm> is set, this tool also extracts the lightmap a\n");
    printf("data and saves it out as series of .bmp files in a lm_filename directory.\n\n");
    printf("Based on OpenSourced code by John W. Ratcliff on December 5, 2000\n");
    printf("Contact Id Software about using Quake 3 data files in and\n");
    printf("Quake 3 editing tools for commercial software development\n");
    printf("projects.\n");
    exit(1);
  }
  StdString file = __argv[1];
  StdString location = "lm/";
  bool lm = false;
  if (__argc >= 3 && (stricmp(__argv[2], "/lm") == 0 || stricmp(__argv[2], "-lm") == 0))
  {
    char buffer[1024];
    if (dd_get_fname(file.c_str()))
    {
      strcpy(buffer, dd_get_fname(file.c_str()));
    }
    else
      strcpy(buffer, file.c_str());
    for (int i = strlen(buffer) - 1; i >= 0; --i)
      if (buffer[i] == '.')
      {
        buffer[i] = 0;
        break;
      }

    location = StdString("lm_") + StdString(buffer);
    if (dd_dir_exist(location.c_str()))
    {
      printf("Directory <%s> already exists - removing it\n", location.c_str());
      if (!dd_rmdir(location.c_str()))
        printf("Could not remove <%s> - probably, it is not empty\n", location.c_str());
    }
    location += "/";
    if (!dd_mkdir(location.c_str()))
    {
      lm = false;
      printf("Can't create lightmap directory %s", location.c_str());
    }
    else
      lm = true;
  }

  // append slash, if there is no slash
  // NOTE: target buffer must have enough space for this slash
  Quake3BSP q(SGET(file), SGET(location), SGET("a"), lm);

  VertexMesh *mesh = q.GetVertexMesh();
  if (mesh)
  {

    mesh->saveDag(file + ".dag", lm ? location.c_str() : NULL);
  }
  else
  {
    printf("Failed to open file in <%s>\n", __argv[1]);
  }

  return 5;
}
