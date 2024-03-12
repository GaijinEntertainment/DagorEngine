/***********      .---.         .-"-.      *******************\
* -------- *     /   ._.       / ´ ` \     * ---------------- *
* Author's *     \_  (__\      \_°v°_/     * humus@rogers.com *
*   note   *     //   \\       //   \\     * ICQ #47010716    *
* -------- *    ((     ))     ((     ))    * ---------------- *
*          ****--""---""-------""---""--****                  ********\
* This file is a part of the work done by Humus. You are free to use  *
* the code in any way you like, modified, unmodified or copy'n'pasted *
* into your own work. However, I expect you to respect these points:  *
*  @ If you use this file and its contents unmodified, or use a major *
*    part of this file, please credit the author and leave this note. *
*  @ For use in anything commercial, please request my approval.      *
*  @ Share your work and ideas too as much as you can.                *
\*********************************************************************/

#ifndef _T3DLOADER_H_
#define _T3DLOADER_H_

#include "tokenizer.h"
#include "stack.h"
#include <string.h>

#include "polygon.h"

typedef class Polygon *(*PolygonCreator)(const int size);
typedef bool (*PolygonFilter)(const int flags);

#define PF_Invisible        1
#define PF_Masked           2
#define PF_Translucent      4
#define PF_NotSolid         8
#define PF_Environment      16
#define PF_ForceViewZone    16
#define PF_Semisolid        32
#define PF_Modulated        64
#define PF_FakeBackdrop     128
#define PF_TwoSided         256
#define PF_AutoUPan         512
#define PF_AutoVPan         1024
#define PF_NoSmooth         2048
#define PF_BigWavy          4096
#define PF_SpecialPoly      4096
#define PF_SmallWavy        8192
#define PF_Flat             16384
#define PF_LowShadowDetail  32768
#define PF_NoMerge          65536
#define PF_CloudWavy        131072
#define PF_DirtyShadows     262144
#define PF_BrightCorners    524288
#define PF_SpecialLit       1048576
#define PF_Gouraud          2097152
#define PF_NoBoundRejection 2097152
#define PF_Unlit            4194304
#define PF_HighShadowDetail 8388608
#define PF_Portal           67108864
#define PF_Mirrored         134217728

#define PF_NoOcclude (PF_Masked | PF_Translucent | PF_Invisible | PF_Modulated)
#define PF_NoShadows (PF_Unlit | PF_Invisible | PF_Environment | PF_FakeBackdrop)

enum STATE
{
  NONE,
  MAP,
  BRUSH,
  POLYGON,
  POLYGONLIST,
  ACTOR
};

/* ########################################################################################################### */
class MaterialData
{
public:
  // DAG_DECLARE_NEW(midmem)
  int flags;
  int usecnt; // internal reference count
  char *name, *script;

  MaterialData()
  {
    flags = 0;
    usecnt = 1;
    name = script = NULL;
  }
};

struct MaterialName
{
  char *name;
  MaterialData *material;
  unsigned int sizeU, sizeV;
  PolygonFlags flags;
  MaterialName()
  {
    name = NULL;
    flags = 0;
    material = NULL;
  }
};

class MaterialTable
{
private:
  bool tryToLoad(char *name, unsigned int &u, unsigned int &v);

public:
  MaterialTable();
  ~MaterialTable();

  void clear();
  void addMaterialName(char *name, unsigned int sizeU, unsigned int sizeV, MaterialData *mat, PolygonFlags flags);
  bool getMaterialFromName(char *name, unsigned int &sizeU, unsigned int &sizeV, MaterialData *&mat, PolygonFlags &flags);

  Set<MaterialName *> materialNames;
};

/* ########################################################################################################### */

class T3dLoader
{
private:
  Tokenizer tok;

  const Vertex fix(const Vertex &v) { return Vertex(v.x, v.z, -v.y); }

  Vertex readVertex();

public:
  T3dLoader();
  ~T3dLoader();

  bool loadFromFile(const char *fileName, Set<class Polygon *> &polygons, MaterialTable &materialTable, PolygonCreator polyCreator,
    PolygonFilter polygonFilter, const Vertex &displacement);
};

void fixTJunctions(Set<class Polygon *> &polys);
void tesselate(Set<class Polygon *> &polys, const float maxArea, const float maxLength);

#endif // _T3DLOADER_H_
