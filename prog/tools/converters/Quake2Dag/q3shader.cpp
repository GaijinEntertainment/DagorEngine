// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>


// ############################################################################
// ##                                                                        ##
// ##  Q3SHADER.CPP                                                          ##
// ##                                                                        ##
// ##  Reads a Quake3 Shader file.                                           ##
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

#include "q3shader.h"
#include "fload.h"

QuakeShaderFactory *QuakeShaderFactory::gSingleton = 0; // global instance of data

QuakeShaderFactory::QuakeShaderFactory(void)
{
#if 1
  AddShader("base.shader");
  AddShader("base_button.shader");
  AddShader("common.shader");
  AddShader("base_floor.shader");
  AddShader("base_light.shader");
  AddShader("base_object.shader");
  AddShader("base_support.shader");
  AddShader("base_trim.shader");
  AddShader("base_wall.shader");
  AddShader("ctf.shader");
  AddShader("eerie.shader");
  AddShader("gfx.shader");
  AddShader("gothic_block.shader");
  AddShader("gothic_floor.shader");
  AddShader("gothic_light.shader");
  AddShader("gothic_trim.shader");
  AddShader("gothic_wall.shader");
  AddShader("hell.shader");
  AddShader("liquid.shader");
  AddShader("menu.shader");
  AddShader("models.shader");
  AddShader("organics.shader");
  AddShader("sfx.shader");
  AddShader("shrine.shader");
  AddShader("skin.shader");
  AddShader("sky.shader");
  AddShader("test.shader");
#endif
}

QuakeShaderFactory::~QuakeShaderFactory(void)
{
  QuakeShaderMap::iterator i;
  for (i = mShaders.begin(); i != mShaders.end(); ++i)
  {
    QuakeShader *shader = (*i).second;
    delete shader;
  }
}

QuakeShader *QuakeShaderFactory::Locate(const StdString &str) { return Locate(StringDict::gStringDict().Get(str)); }

QuakeShader *QuakeShaderFactory::Locate(const StringRef &str)
{
  QuakeShaderMap::iterator found;
  found = mShaders.find(str);
  if (found != mShaders.end())
    return (*found).second;
  return 0;
}


void QuakeShaderFactory::AddShader(const StdString &sname)
{

  mBraceCount = 0;
  mCurrent = 0;

  printf("***********SHADER PROCESS : %s \n", sname.c_str());

  Fload shader(sname);

  while (1)
  {

    char *str = shader.GetString();

    if (!str)
      break;
    ShaderString(str); // process one string.
  }

  delete mCurrent;
  mCurrent = 0;
}

void QuakeShaderFactory::ShaderString(const char *str)
{
  char workspace[1024];

  char *dest = workspace;

  while (*str)
  {
    if (str[0] == '//' && str[1] == '//')
      break;
    *dest++ = *str++;
  }
  *dest = 0;
  int len = strlen(workspace);
  if (len == 0)
    return;

  ArgList::Set(workspace); // crunch it into arguments.
  // now ready to process it as a series of arguments!
  if (mArgs.size())
    Process(mArgs);
}

void QuakeShaderFactory::Process(const StringVector &args)
{

  if (args[0] == "{")
  {
    mBraceCount++;
  }
  else
  {
    if (args[0] == "}")
    {
      mBraceCount--;

      if (mBraceCount == 0)
      {
        if (mCurrent)
        {
          const StringRef &ref = mCurrent->GetName();
          QuakeShaderMap::iterator found;
          found = mShaders.find(ref);
          if (found != mShaders.end())
          {
            printf("Can't add shader %s, it already exists!!\n", (const char *)ref);
          }
          else
          {
            mShaders[ref] = mCurrent;
            printf("Added shader: %s\n", (const char *)ref);
            mCurrent = 0;
          }
        }
        else
        {
          printf("Got closing brace without valid shader defined.\n");
        }
      }
      else
      {
        if (mBraceCount < 0)
        {
          printf("Missmatched closing brace situation!??\n");
          mBraceCount = 0;
        }
      }
    }
    else
    {
      if (!mBraceCount)
      {
        delete mCurrent; // if didn't process the last one
        mCurrent = 0;
        char name[256];
        if (GetName(args[0], name))
        {
          StringRef ref = StringDict::gStringDict().Get(name);
          mCurrent = new QuakeShader(ref);
        }
      }
      else
      {
        // process command!
        if (args[0] == "map" && args.size() == 2 && mCurrent)
        {
          char name[256];
          if (GetName(args[1], name))
          {
            const StringRef ref = StringDict::gStringDict().Get(name);
            mCurrent->AddTexture(ref);
            printf("Adding texture %s\n", (const char *)ref);
          }
        }
      }
    }
  }
}

bool QuakeShaderFactory::GetName(const StdString &str, char *tname)
{
  int len = str.size();
  if (!len)
    return false;
  const char *foo = str.c_str();
  foo = &foo[len - 1];
  while (*foo && *foo != '/')
    foo--;
  if (!*foo)
    return false;
  foo++;
  char *dest = tname;
  while (*foo)
  {
    *dest++ = *foo++;
  }
  *dest = 0;

  len = strlen(tname);
  if (len >= 4)
  {
    if (tname[len - 4] == '.')
    {
      tname[len - 4] = 0;
    }
  }
  return true;
}
