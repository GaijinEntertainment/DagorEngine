#ifndef Q3SHADER_H

#define Q3SHADER_H

// ############################################################################
// ##                                                                        ##
// ##  Q3SHADER.H                                                            ##
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


#include "stringdict.h"
#include "arglist.h"

class QuakeShader
{
public:
  QuakeShader(const StringRef &ref) { mName = ref; };
  const StringRef &GetName(void) const { return mName; };
  void AddTexture(const StringRef &ref)
  {
    StdString ifoo = ref;
    if (ifoo == "chrome_env")
      return;
    if (ifoo == "tinfx")
      return;

    mTextures.push_back(ref);
  };
  bool GetBaseTexture(StringRef &ref)
  {
    if (!mTextures.size())
      return false;
    ref = mTextures[0];
    return true;
  }

private:
  StringRef mName; // name of shader.
  StringRefVector mTextures;
};

typedef std::map<StringRef, QuakeShader *> QuakeShaderMap;

class QuakeShaderFactory : public ArgList
{
public:
  QuakeShaderFactory(void);
  ~QuakeShaderFactory(void);

  QuakeShader *Locate(const StdString &str);
  QuakeShader *Locate(const StringRef &str);

  void AddShader(const StdString &sname);


  static QuakeShaderFactory &gQuakeShaderFactory(void)
  {
    if (!gSingleton)
    {
      gSingleton = new QuakeShaderFactory;
    }
    return *gSingleton;
  }

  static void ExplicitDestroy(void) // explicitely destroy the global intance
  {
    delete gSingleton;
    gSingleton = 0;
  }

  void Process(const StringVector &args);

  /// get stripped down version of the name
  bool GetName(const StdString &str, char *stripped);

private:
  int mBraceCount;
  QuakeShader *mCurrent;

  void ShaderString(const char *str);


  QuakeShaderMap mShaders; // all shaders available.

  static QuakeShaderFactory *gSingleton; // global instance of data
};

#endif
