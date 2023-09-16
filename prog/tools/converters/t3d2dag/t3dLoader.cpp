/***********      .---.         .-"-.      *******************\
* -------- *     /   ._.       / ґ ` \     * ---------------- *
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

#include "t3dLoader.h"
#include "platform.h"
#include <util/dag_string.h>
#include <stdio.h>
#include <generic/dag_tab.h>
#include <string.h>
#include <libTools/util/strUtil.h>
#include <debug/dag_debug.h>
#include <windows.h>
#include "unTex.h"

#define PACKAGE_FILE_TAG 0x9E2A83C1


// void debug(const char*, ...);

MaterialTable::MaterialTable() {}

MaterialTable::~MaterialTable() { clear(); }

void MaterialTable::clear()
{
  MaterialName *matName;
  int i, len = materialNames.getSize();

  for (i = 0; i < len; i++)
  {
    matName = materialNames[i];
    delete matName->name;
    // delete matName->material;
    delete matName;
  }

  materialNames.clear();
}

void MaterialTable::addMaterialName(char *name, unsigned int sizeU, unsigned int sizeV, MaterialData *mat, PolygonFlags flags)
{
  MaterialName *matName = new MaterialName;

  if (mat && name)
  {
    mat->name = strdup(name);
  }
  matName->name = new char[strlen(name) + 1];
  strcpy(matName->name, name);
  matName->sizeU = sizeU;
  matName->sizeV = sizeV;
  matName->material = mat;
  matName->flags = flags;
  materialNames.add(matName);
}

#define DECODE_COMPACT_INDEX(buffer, offset, value)         \
  do                                                        \
  {                                                         \
    unsigned char neg = (buffer)[(offset)] & 0x80;          \
    (value) = (buffer)[(offset)] & 0x3F;                    \
    if ((buffer)[(offset)++] & 0x40)                        \
    {                                                       \
      (value) |= ((buffer)[(offset)] & 0x7F) << 6;          \
      if ((buffer)[(offset)++] & 0x80)                      \
      {                                                     \
        (value) |= ((buffer)[(offset)] & 0x7F) << 13;       \
        if ((buffer)[(offset)++] & 0x80)                    \
        {                                                   \
          (value) |= ((buffer)[(offset)] & 0x7F) << 20;     \
          if ((buffer)[(offset)++] & 0x80)                  \
          {                                                 \
            (value) |= ((buffer)[(offset)++] & 0x1F) << 27; \
          }                                                 \
        }                                                   \
      }                                                     \
    }                                                       \
    if (neg)                                                \
      (value) = -(value);                                   \
  } while (0)

#define READ_COMPACT_INDEX(file, value)    \
  do                                       \
  {                                        \
    unsigned char buf = 0;                 \
    fread(&buf, 1, 1, (file));             \
    unsigned char neg = buf & 0x80;        \
    (value) = buf & 0x3F;                  \
    if (buf & 0x40)                        \
    {                                      \
      fread(&buf, 1, 1, (file));           \
      (value) |= (buf & 0x7F) << 6;        \
      if (buf & 0x80)                      \
      {                                    \
        fread(&buf, 1, 1, (file));         \
        (value) |= (buf & 0x7F) << 13;     \
        if (buf & 0x80)                    \
        {                                  \
          fread(&buf, 1, 1, (file));       \
          (value) |= (buf & 0x7F) << 20;   \
          if (buf & 0x80)                  \
          {                                \
            fread(&buf, 1, 1, (file));     \
            (value) |= (buf & 0x1F) << 27; \
          }                                \
        }                                  \
      }                                    \
    }                                      \
    if (neg)                               \
      (value) = -(value);                  \
  } while (0)

struct ExportEntry
{
  int classId, superId, objectName, serialSize, serialOffset;
  unsigned group, objectFlags;
  void read(FILE *pkg)
  {
    READ_COMPACT_INDEX(pkg, classId);
    READ_COMPACT_INDEX(pkg, superId);
    fread(&group, 4, 1, pkg);
    READ_COMPACT_INDEX(pkg, objectName);
    fread(&objectFlags, 4, 1, pkg);
    READ_COMPACT_INDEX(pkg, serialSize);
    if (serialSize > 0)
    {
      READ_COMPACT_INDEX(pkg, serialOffset);
    }
  }
};

struct ImportEntry
{
  int classId, className, objectName;
  unsigned package;
  void read(FILE *pkg)
  {
    READ_COMPACT_INDEX(pkg, classId);
    READ_COMPACT_INDEX(pkg, className);
    fread(&package, 4, 1, pkg);
    READ_COMPACT_INDEX(pkg, objectName);
  }
};

void CreateBmp24(char *fname, RGBTRIPLE *color, int u_size, int v_size)
{
  HANDLE hFile;
  DWORD RW;
  int i, j;

  // Объявим нужные структуры
  BITMAPFILEHEADER bfh;
  BITMAPINFOHEADER bih;
  BYTE Palette[1024]; // Палитра

  // Пусть у нас будет картинка размером 35 x 50 пикселей
  int Width = u_size;
  int Height = v_size;
  memset(Palette, 0, 1024); // В палитре у нас нули

  // Заполним их
  memset(&bfh, 0, sizeof(bfh));
  bfh.bfType = 0x4D42;                              // Обозначим, что это bmp 'BM'
  bfh.bfOffBits = sizeof(bfh) + sizeof(bih) + 1024; // Палитра занимает 1Kb, но мы его испоьзовать не будем
  bfh.bfSize = bfh.bfOffBits + sizeof(color[0]) * Width * Height + Height * (Width % 4); // Посчитаем размер конечного файла

  memset(&bih, 0, sizeof(bih));
  bih.biSize = sizeof(bih);   // Так положено
  bih.biBitCount = 24;        // 16 бит на пиксель
  bih.biCompression = BI_RGB; // Без сжатия
  bih.biHeight = Height;
  bih.biWidth = Width;
  bih.biPlanes = 1; // Должно быть 1
                    // А остальные поля остаются 0
  hFile = CreateFile(fname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
  if (hFile == INVALID_HANDLE_VALUE)
    return;

  // Запишем заголовки
  WriteFile(hFile, &bfh, sizeof(bfh), &RW, NULL);
  WriteFile(hFile, &bih, sizeof(bih), &RW, NULL);

  // Запишем палитру
  WriteFile(hFile, Palette, 1024, &RW, NULL);
  for (i = 0; i < Height; i++)
  {
    for (j = 0; j < Width; j++)
    {
      WriteFile(hFile, &color[i * Width + j], sizeof(color[0]), &RW, NULL);
    }
    // Выровняем по границе
    WriteFile(hFile, Palette, Width % 4, &RW, NULL);
  }

  CloseHandle(hFile);
}

bool MaterialTable::tryToLoad(char *name, unsigned int &sizeU, unsigned int &sizeV)
{
  char names[3][512];
  Tab<ExportEntry> exportTable(tmpmem_ptr());
  Tab<ImportEntry> importTable(tmpmem_ptr());
  Tab<String> allNames(tmpmem_ptr());
  int cnm = 0, cind = 0;
  for (int i = 0; i <= strlen(name); ++i)
  {
    names[cnm][cind++] = name[i];
    if (name[i] == '\0')
      break;
    if (name[i] == '.')
    {
      names[cnm][cind - 1] = 0;
      cind = 0;
      cnm++;
    }
  }
  // debug("looking for %s = (%s.%s.%s)", name, names[0], names[1], names[2]);
  if (cnm != 2)
    return false;
  FILE *pkg = fopen(String(128, "%s.utx", names[0]), "rb");
  if (!pkg)
  {
    debug("No such package <%s>", names[0]);
    return false;
  }
  unsigned magic;
  fread(&magic, 4, 1, pkg);
  bool ret = false;
  if (magic != PACKAGE_FILE_TAG)
  {
    debug("Not a package!");
    fclose(pkg);
    return ret;
    // goto end;
  }
  struct Header
  {
    unsigned version, packageFlags;
    unsigned nameCount, nameOffset;
    unsigned exportCount, exportOffset;
    unsigned importCount, importOffset;
  } header;

  fread(&header, sizeof(header), 1, pkg);
  header.version &= 0xFFFF;
  // debug("version = %d", header.version);
  fseek(pkg, header.nameOffset, SEEK_SET);
  int groupId = -1, nameId = -1;
  allNames.resize(header.nameCount);

  for (int nameIndex = 0; nameIndex < header.nameCount; ++nameIndex)
  {
    char checkName[8192];
    int c = 0;
    int len = sizeof(checkName);
    if (header.version >= 64)
    {
      len = 0;
      fread(&len, 1, 1, pkg);
    }
    for (c = 0; c < sizeof(checkName) && c < len; ++c)
    {
      fread(&checkName[c], 1, 1, pkg);
      if (checkName[c] == 0)
        break;
    }
    if (c == sizeof(checkName))
    {
      nameIndex = header.nameCount;
      break;
    }

    unsigned flags;
    fread(&flags, 4, 1, pkg);
    if (strcmp(names[1], checkName) == 0)
      groupId = nameIndex;
    if (strcmp(names[2], checkName) == 0)
      nameId = nameIndex;
    allNames[nameIndex] = checkName;
    // debug("%d %s",nameIndex, checkName);
  }
  if (nameId == -1 || groupId == -1)
  {
    debug("not found %d.%d (%s.%s)!", groupId, nameId, names[1], names[2]);
    fclose(pkg);
    return ret;
    // goto end;
  }
  exportTable.resize(header.exportCount);
  fseek(pkg, header.exportOffset, SEEK_SET);
  for (int exportIndex = 0; exportIndex < header.exportCount; ++exportIndex)
  {
    exportTable[exportIndex].read(pkg);
  }
  importTable.resize(header.importCount);
  fseek(pkg, header.importOffset, SEEK_SET);
  for (int importIndex = 0; importIndex < header.importCount; ++importIndex)
  {
    importTable[importIndex].read(pkg);
  }

  int textureExportId = -1;
  for (int exportIndex = 0; exportIndex < header.exportCount; ++exportIndex)
  {
    if (exportTable[exportIndex].objectName == nameId)
    {
      int group = exportTable[exportIndex].group;
      if (group > 0)
      {
        if (exportTable[group - 1].objectName == groupId)
        {
          textureExportId = exportIndex;
          break;
        }
      }
      else if (group < 0)
      {
        if (importTable[-group - 1].objectName == groupId)
        {
          textureExportId = exportIndex;
          break;
        }
      }
    }
  }
  // int uSize=0,vSize;
  UBitmap bitmap;
  if (textureExportId >= 0)
  {
    if (exportTable[textureExportId].serialSize > 0)
    {
      // debug("found! %d size", exportTable[textureExportId].serialSize);
      int classId = exportTable[textureExportId].classId;
      if (classId > 0)
      {
        // debug("class %s",(char*)allNames[exportTable[classId-1].objectName]);
      }
      else if (classId < 0)
      {
        // debug("class %s",(char*)allNames[importTable[-classId-1].objectName]);
      }

      Tab<unsigned char> texImg(tmpmem);
      texImg.resize(exportTable[textureExportId].serialSize);
      fseek(pkg, exportTable[textureExportId].serialOffset, SEEK_SET);
      // debug("offset=%d", exportTable[textureExportId].serialOffset);
      fread(texImg.data(), data_size(texImg), 1, pkg);
      int ofs = 0;
      unsigned char *buf = &texImg[0];
      do
      {
        int property = 0;
        DECODE_COMPACT_INDEX(buf, ofs, property);
        if (strcmp(allNames[property], "None") == 0)
          break;
        int info = buf[ofs++];
        int type = info & 0xF;
        int size = (info >> 4) & 0x7, realSize = 0;
        int isArray = (info) & (1 << 7);
        int structNameId = -1;
        const int sizes[] = {1, 2, 4, 12, 16, -1, -2, -4};
        realSize = sizes[size];
        switch (type)
        {
          case 3: isArray = 0; break;
          case 5:
            // object
            break;
          case 0xA: DECODE_COMPACT_INDEX(buf, ofs, structNameId); break;
          default: break;
        };
        if (isArray)
        {
          unsigned char arIdx = buf[ofs++];
          if (arIdx & 0xC0)
          {
            ofs += 2;
          }
          else if (arIdx & 0x80)
          {
            ofs++;
          }
        }

        // debug("property %s %s of type=%d, size = %d(%d)%s", (char*)allNames[property],
        //   structNameId >=0 ? (char*)allNames[structNameId] : "", type, realSize, size,isArray?"arra":"");
        int val = 0;
        switch (realSize)
        {
          case 1:
          case 5: val = buf[ofs]; break;
          case 2: val = *(short *)&buf[ofs]; break;
          case 4: val = *(int *)&buf[ofs]; break;
        }
        if (stricmp(allNames[property], "Palette") == 0)
        { //
          bitmap.Palette = new UPalette;
          bitmap.Palette->Colors.resize(val);
          // debug("palette=%i\n", val);
        }
        if (stricmp(allNames[property], "MipZero") == 0)
        {
          bitmap.MipZero = *(FColor *)&buf[ofs];
          // debug("MipZero=%d %d %d %d\n", bitmap.MipZero.R, bitmap.MipZero.G, bitmap.MipZero.B, bitmap.MipZero.A);
        }
        if (stricmp(allNames[property], "MaxColor") == 0)
        {
          bitmap.MaxColor = *(FColor *)&buf[ofs];
          // debug("MaxColor=%d %d %d %d\n", bitmap.MaxColor.R, bitmap.MaxColor.G, bitmap.MaxColor.B, bitmap.MaxColor.A);
        }
        /*if (stricmp(allNames[property],"InternalTime")==0 && !isArray){
          bitmap.InternalTime[0] = val;
          debug("internalTime[0]=%i\n", val);
        }
        if (stricmp(allNames[property],"InternalTime")==0 && isArray){
          bitmap.internalTime[1] = val;
          debug("internalTime[1]=%i\n", val);
        }*/
        if (stricmp(allNames[property], "UClamp") == 0)
        {
          bitmap.UClamp = val;
          // debug("uSize=%d\n", val);
        }
        if (stricmp(allNames[property], "VClamp") == 0)
        {
          bitmap.VClamp = val;
          // debug("vSize=%d\n", val);
        }
        if (stricmp(allNames[property], "Usize") == 0)
        {
          bitmap.USize = val;
          // debug("USize=%d\n", val);
        }
        if (stricmp(allNames[property], "Vsize") == 0)
        {
          bitmap.VSize = val;
          // debug("vSize=%d\n", val);
        }
        if (stricmp(allNames[property], "Ubits") == 0)
        {
          bitmap.UBits = val;
          // debug("uBits=%d\n", val);
        }
        if (stricmp(allNames[property], "Vbits") == 0)
        {
          bitmap.VBits = val;
          // debug("vBits=%d\n", val);
        }
        if (stricmp(allNames[property], "Format") == 0)
        {
          // debug("format=%d\n", val);
          bitmap.Format = (ETextureFormat)val;
        }
        if (realSize > 0)
        {
          ofs += realSize;
        }
      } while (1);
      int mipmapcount = texImg[ofs];
      if (mipmapcount > 0)
      {
        FMipmap mipmap;
        int offset = ofs + 1;
        unsigned fileOffset = *((unsigned *)&texImg[offset]);
        offset += 4;
        int mipmapSize = 0;
        DECODE_COMPACT_INDEX(texImg, offset, mipmapSize);

        // debug("mipmapSize=%i", mipmapSize);
        mipmap.DataArray.resize(mipmapSize);
        for (int i = 0; i < mipmapSize; i++)
        {
          mipmap.DataArray[i] = texImg[offset];
          offset++;
        }

        mipmap.USize = *((unsigned *)&texImg[offset]);
        mipmap.VSize = *((unsigned *)&texImg[offset + 4]);
        mipmap.UBits = texImg[offset + 8];
        mipmap.VBits = texImg[offset + 9];
        // debug("USize=%d VSize=%d UBits=%i VBits=%i", mipmap.USize, mipmap.VSize, mipmap.UBits, mipmap.VBits);
        // save_tga32( ::get_file_ext(name), (TexPixel32 *)texImg, uSize, vSize, vSize * sizeof (TexPixel32) );


        debug("(%s.%s.%s) = %s.utx-%s.%s-0.bmp", names[0], names[1], names[2], names[0], names[1], names[2]);

        /*Tab<RGBTRIPLE> colors(tmpmem);
        colors.resize(bitmap.USize*bitmap.VSize);
        for(int i=0; i<colors.size(); i++)
        {
          int c = texImg[offset+10+i];
          colors[i].rgbtRed = mipmap.DataArray[i];
          colors[i].rgbtGreen = mipmap.DataArray[i];
          colors[i].rgbtBlue = mipmap.DataArray[i];
          //debug("%i %i -> %i %i %i", i, texImg[offset+8+2+i], colors[i].rgbtRed, colors[i].rgbtGreen, colors[i].rgbtBlue);
        }

        CreateBmp24(String(128, "%s.utx-%s.%s-0.bmp", names[0], names[1], names[2]), &colors[0],
          mipmap.USize, mipmap.VSize );*/
      }
    }
  }
  if (bitmap.USize > 0)
  {
    sizeU = bitmap.USize, sizeV = bitmap.VSize;
    ret = true;
  }
  // printf("found names %d.%d %s.%s!\n", groupId, nameId, names[1], names[2]);

  // end:
  ;
  fclose(pkg);
  return ret;
}

bool MaterialTable::getMaterialFromName(char *name, unsigned int &sizeU, unsigned int &sizeV, MaterialData *&mat, PolygonFlags &flags)
{
  MaterialName *matName;
  int i, len = materialNames.getSize();

  for (i = 0; i < len; i++)
  {
    matName = materialNames[i];

    if (stricmp(name, matName->name) == 0)
    {
      sizeU = matName->sizeU;
      sizeV = matName->sizeV;
      mat = matName->material;
      flags = matName->flags;
      return true;
    }
  }
  if (tryToLoad(name, sizeU, sizeV))
  {
    mat = new MaterialData;
    addMaterialName(name, sizeU, sizeV, mat, flags);
    return true;
  }
  printf("missing texture <%s>\n", name);
  if (len > 0)
  {
    matName = materialNames[0];
    sizeU = matName->sizeU;
    sizeV = matName->sizeV;
    mat = matName->material;
    flags = matName->flags;
  }
  else
  {
    sizeU = 512;
    sizeV = 512;
    mat = NULL;
    flags = 0;
  }
  addMaterialName(name, sizeU, sizeV, mat, flags);
  return false;
}

/* ------------------------------------------------------------------- */

T3dLoader::T3dLoader() {}

T3dLoader::~T3dLoader() {}

Vertex T3dLoader::readVertex()
{
  char str[256];
  float x, y, z;

  tok.next(str); // sign
  x = (*str == '+') ? 1.0f : -1.0f;
  tok.next(str); // number
  x *= intAdjustf((float)atof(str));

  tok.next(NULL); // ,
  tok.next(str);  // sign
  y = (*str == '+') ? 1.0f : -1.0f;
  tok.next(str); // number
  y *= intAdjustf((float)atof(str));

  tok.next(NULL); // ,
  tok.next(str);  // sign
  z = (*str == '+') ? 1.0f : -1.0f;
  tok.next(str); // number
  z *= intAdjustf((float)atof(str));

  return Vertex(x, y, z);
}

bool textureNameAlphabetical(const char ch)
{
  return ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' || ch == '.' || ch == '-');
}

bool T3dLoader::loadFromFile(const char *fileName, Set<class Polygon *> &polygons, MaterialTable &materialTable,
  PolygonCreator polyCreator, PolygonFilter polygonFilter, const Vertex &displacement)
{
  char str[256];
  Stack<STATE> states;
  STATE state = NONE;
  int i, len;
  int from = 0;

  // Polygon variables
  class Polygon *poly;
  // class Polygon *poly;
  Set<Vertex> vertices;
  int flags = 0;
  PolygonFlags pflags;
  Vertex normal;
  Vertex origo, textureU, textureV;
  float panU = 0, panV = 0;
  MaterialData *material;
  unsigned int sizeU, sizeV;

  // Actor variables
  bool usePolygons = true;
  Vertex location(0, 0, 0);
  Vertex prepivot(0, 0, 0);
  int vertexComponent;


  char debugg[256];


  tok.setFile(fileName);

  while (tok.next(str))
  {
    if (stricmp(str, "Begin") == 0)
    {
      tok.next(str);
      states.push_back(state);

      if (stricmp(str, "Map") == 0)
      {
        state = MAP;
      }
      else if (stricmp(str, "Polygon") == 0)
      {
        state = POLYGON;
        if (usePolygons)
        {
          vertices.clear();
          flags = 0;
          panU = panV = 0.0f;
        }
      }
      else if (stricmp(str, "Actor") == 0)
      {
        state = ACTOR;
        pflags = PF_NONE;
        location = Vertex(0, 0, 0);
        from = polygons.getSize();
        usePolygons = true;
      }
      else
      {
        state = NONE;
      }
    }
    else if (stricmp(str, "End") == 0)
    {
      tok.next(NULL);

      switch (state)
      {
        case POLYGON:
          if (usePolygons)
          {
            if (polygonFilter(flags))
            {
              // if ((flags & PF_Invisible) == 0){
              len = vertices.getSize();

              // poly = new class Polygon(len);
              poly = polyCreator(len);


              for (i = 0; i < len; i++)
              {
                // poly->setVertex(i, vertices[i]);
                poly->setVertex(len - i - 1, vertices[i] + displacement);
              }

              if (flags & PF_TwoSided)
              {
                poly->setFlags(PF_DOUBLESIDED, true);
              }
              if (flags & PF_Unlit)
              {
                poly->setFlags(PF_UNLIT, true);
              }
              if (flags & PF_Translucent)
              {
                poly->setFlags(PF_TRANSLUCENT, true);
              }
              if (flags & PF_Portal)
              {
                poly->setFlags(PF_PORTAL, true);
              }

              /*if (flags & PF_SpecialPoly){
                  poly->setFlags(PF_FOGVOLUME | PF_NONBLOCKING, true);
              }*/

              poly->setFlags(pflags, true);

              // origo -= (panU * (textureU / length(textureU)) + panV * (textureV / length(textureV)));
              // origo += ((-panU - 2) * textureU * 2 + panV * textureV * 2);

              origo -= (panU * textureU) / lengthSqr(textureU);
              origo -= (panV * textureV) / lengthSqr(textureV);

              textureU = fix(textureU);
              textureV = fix(textureV);

              poly->setTexCoordSystem(fix(origo) + displacement, textureU / (float)sizeU, textureV / (float)sizeV);
              poly->setMaterial(material);
              poly->finalize();
              // poly->setNormal(normal);
              polygons.add(poly);
            }
          }
          break;
        default: break;
      }

      state = states.pop_back();
    }
    else
    {


      switch (state)
      {
        case POLYGON:
          if (usePolygons)
          {
            if (stricmp(str, "Flags") == 0)
            {
              tok.next(NULL); // =
              tok.next(str);  // flags
              if (str[0] == '-')
              {
                tok.next(str); // flags
                flags = -atoi(str);
              }
              else
                flags = atoi(str);
            }
            else if (polygonFilter(flags))
            { // if ((flags & PF_Invisible) == 0){
              if (stricmp(str, "Vertex") == 0)
              {
                Vertex v = readVertex();
                vertices.add(fix(v + location));
              }
              else if (stricmp(str, "Normal") == 0)
              {
                normal = fix(readVertex());
              }
              else if (stricmp(str, "Texture") == 0)
              {
                tok.next(NULL);                         // =
                tok.next(str, textureNameAlphabetical); // texture name

                materialTable.getMaterialFromName(str, sizeU, sizeV, material, pflags);
                strcpy(debugg, str);
              }
              else if (stricmp(str, "TextureU") == 0)
              {
                textureU = readVertex();
              }
              else if (stricmp(str, "TextureV") == 0)
              {
                textureV = readVertex();
              }
              else if (stricmp(str, "Origin") == 0)
              {
                origo = readVertex();
              }
              else if (stricmp(str, "Pan") == 0)
              {
                tok.next(NULL); // U
                tok.next(NULL); // =
                tok.next(str);  // U pan or -
                if (str[0] == '-')
                {
                  tok.next(str); // U pan
                  panU = -(float)atof(str);
                }
                else
                  panU = (float)atof(str);

                tok.next(NULL); // V
                tok.next(NULL); // =
                tok.next(str);  // V pan or -
                if (str[0] == '-')
                {
                  tok.next(str); // V pan
                  panV = -(float)atof(str);
                }
                else
                  panV = (float)atof(str);
              }
            }
          }
          break;
        case ACTOR:
          if (stricmp(str, "Group") == 0)
          {
            usePolygons = false;
          }
          else if (stricmp(str, "CsgOper") == 0)
          {
            tok.next(NULL); // =
            tok.next(str);
            if (stricmp(str, "CSG_Add"))
            {
              pflags |= PF_ADD;
            }
            else
            {
              pflags &= ~PF_ADD;
            }
          }
          else if (stricmp(str, "Location") == 0)
          {
            float value;
            tok.next(NULL); // =
            tok.next(NULL); // (
            do
            {
              tok.next(str);
              vertexComponent = (*str - 'X');

              tok.next(NULL); // =
              tok.next(str);  // sign/value
              if (*str == '-')
              {
                tok.next(str); // number
                value = -(float)atof(str);
              }
              else
              {
                value = (float)atof(str);
              }
              location[vertexComponent] = value;
              tok.next(str);
            } while (*str == ',');
            for (int j = from; j < polygons.getSize(); j++)
            {
              for (int i = 0; i < polygons[j]->getnVertices(); i++)
                polygons[j]->setVertex(i, polygons[j]->getVertex(i) + fix(location));
            }
          }
          else if (stricmp(str, "PrePivot") == 0)
          {
            float value;
            tok.next(NULL); // =
            tok.next(NULL); // (
            do
            {
              tok.next(str);
              vertexComponent = (*str - 'X');

              tok.next(NULL); // =
              tok.next(str);  // sign/value
              if (*str == '-')
              {
                tok.next(str); // number
                value = -(float)atof(str);
              }
              else
              {
                value = (float)atof(str);
              }
              prepivot[vertexComponent] = value;
              tok.next(str);
            } while (*str == ',');
            for (int j = from; j < polygons.getSize(); j++)
            {
              for (int i = 0; i < polygons[j]->getnVertices(); i++)
                polygons[j]->setVertex(i, polygons[j]->getVertex(i) - fix(prepivot));
            }
          }
          /*else if (stricmp(str, "Rotation") == 0)
          {
            float value;
            tok.next(NULL); // =
            tok.next(NULL); // (
            do
            {
              tok.next(str);
              vertexComponent = (*str - 'X');

              tok.next(NULL); // =
              tok.next(str);  // sign/value
              if (*str == '-')
              {
                tok.next(str);  // number
                value = -(float) atof(str);
              }
              else
              {
                value = (float) atof(str);
              }
              prepivot[vertexComponent] = value;
              tok.next(str);
            }
            while (*str == ',');
            /*for(int j=from; j<polygons.getSize(); j++)
            {
              for(int i=0; i<polygons[j]->getnVertices(); i++)
                polygons[j]->setVertex(i, polygons[j]->getVertex(i)-fix(prepivot));
            }
          }
          break;*/
        default: break;
      }
    }
  }

  return true;
}

void fixTJunctions(Set<class Polygon *> &polys)
{
  unsigned int i, j, k, l, len = polys.getSize();
  Vertex prev, curr, dis;

  for (i = 0; i < len; i++)
  {
    for (j = 0; j < len; j++)
    {
      if (i != j)
      {
        prev = polys[i]->getLastVertex();
        for (k = 0; k < polys[i]->getnVertices(); k++)
        {
          curr = polys[i]->getVertex(k);
          dis = curr - prev;

          for (l = 0; l < polys[j]->getnVertices(); l++)
          {

            float t = dis * (polys[j]->getVertex(l) - prev) / lengthSqr(dis);

            if (t > 0.1f && t < 0.9f)
            {
              if (lengthSqr(prev + t * dis - polys[j]->getVertex(l)) < 0.05f)
              {
                polys[i]->insertVertex(k, polys[j]->getVertex(l));
                k++;
                // polys[i]->insertVertex(4, Vertex(0,0,0));
                return;
              }
            }
          }

          prev = curr;
        }
      }
    }
  }
}

void tesselate(Set<class Polygon *> &polys, const float maxArea, const float maxLength)
{
  unsigned int i, j, len;

  i = 0;
  do
  {
    float area = 0, max = 0;
    unsigned maxIndex = 0;

    class Polygon *poly = polys[i];

    len = poly->getnVertices();
    Vertex prev = poly->getVertex(len - 1);
    for (j = 0; j < len; j++)
    {
      Vertex curr = poly->getVertex(j);

      if (j > 1)
      {
        area += length(cross(curr - poly->getVertex(0), prev - poly->getVertex(0)));
      }

      float d = lengthSqr(prev - curr);
      if (d > max)
      {
        max = d;
        maxIndex = j;
      }

      prev = curr;
    }

    if (area > maxArea || max > sqrf(maxLength))
    {
      Vertex v0 = poly->getVertex(maxIndex == 0 ? len - 1 : maxIndex - 1);
      Vertex v1 = poly->getVertex(maxIndex);
      Vertex mid0 = 0.5f * (v0 + v1);

      Vertex curr = poly->getVertex(maxIndex);
      float a, halfArea = 0;
      unsigned int otherIndex = maxIndex;
      do
      {
        otherIndex++;
        prev = curr;
        curr = poly->getVertex(otherIndex % len);

        a = length(cross(curr - mid0, prev - mid0));
        halfArea += a;

      } while (2 * halfArea < area);

      float t = (halfArea - area * 0.5f) / a;
      Vertex mid1 = curr + t * (prev - curr);

      class Polygon *poly0, *poly1;

      unsigned int n0, n1;
      n0 = (otherIndex < maxIndex) ? otherIndex - maxIndex + len : otherIndex - maxIndex;
      n1 = len - n0;

      poly0 = poly->create(n0 + 2);
      poly->initialize(poly0);
      poly1 = poly->create(n1 + 2);
      poly->initialize(poly1);

      poly0->setVertex(0, mid1);
      poly0->setVertex(1, mid0);
      for (j = 0; j < n0; j++)
      {
        poly0->setVertex(j + 2, poly->getVertex((maxIndex + j) % len));
      }
      poly1->setVertex(0, mid0);
      poly1->setVertex(1, mid1);
      for (j = 0; j < n1; j++)
      {
        poly1->setVertex(j + 2, poly->getVertex((otherIndex + j) % len));
      }

      poly0->finalize();
      poly1->finalize();

      delete poly;
      polys.remove(i);

      polys.add(poly0);
      polys.add(poly1);
    }
    else
    {
      i++;
    }

  } while (i < polys.getSize());
}
