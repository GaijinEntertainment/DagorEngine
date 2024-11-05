// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/dtx/dtxSave.h>
#include <libTools/util/de_TextureName.h>
#include <ioSys/dag_ioUtils.h>
#include <osApiWrappers/dag_files.h>
#include <3d/ddsFormat.h>
#include <libTools/dtx/dtx.h>

namespace ddstexture
{
//=========================================================================================================
bool write_to_cb(IGenSave &cwr, const char *name, Header *header, bool write_file_size)
{
  if (!*name)
    return false;

  file_ptr_t fp = df_open(name, DF_READ);
  if (!fp)
    return false;

  int size = df_length(fp);
  if (write_file_size) //-V1051
    cwr.writeInt(4 + sizeof(Header) + size);
  Header ftr;
  if (header)
    ftr = *header;
  else
    ftr.getFromFile(name);

  cwr.writeInt(sizeof(Header));
  cwr.write(&ftr, sizeof(ftr));

  copy_file_to_stream(fp, cwr, size);

  df_close(fp);
  return true;
}

//=========================================================================================================

bool write_cb_to_cb(IGenSave &cwr, IGenLoad &crd, Header *header, int size)
{
  if (!header)
    return false;
  cwr.writeInt(4 + sizeof(Header) + size);
  Header ftr = *header;
  cwr.writeInt(sizeof(Header));
  cwr.write(&ftr, sizeof(ftr));
  copy_stream_to_stream(crd, cwr, size);
  return true;
}

//=========================================================================================================
bool write_to_file(IGenLoad &crd, const char *filename, int datalen)
{
  FullFileSaveCB cwr(filename);
  if (!cwr.fileHandle)
    return false;

  if (datalen == -1)
    datalen = crd.readInt();

  Header info;
  int pos = crd.tell();
  int magic = crd.readInt();

  if (!check_signature(&magic))
  {
    int footer_size = magic;
    if (sizeof(info) >= footer_size)
      crd.read(&info, footer_size);
    else
    {
      crd.read(&info, sizeof(info));
      crd.seekrel(footer_size - sizeof(info));
    }
    datalen -= footer_size + 4;
  }

  if ((info.addr_mode_v & 7) == 0)
    info.addr_mode_v = info.addr_mode_u;

  copy_stream_to_stream(crd, cwr, datalen);
  return true;
}


//==============================================================================
bool getDdsInfo(const char *filename, int *src_type, int *src_format, int *src_mipmaps, int *width, int *height)
{
  FullFileLoadCB crd(filename);

  if (!crd.fileHandle)
    return false;

  DAGOR_TRY
  {
    int magic = crd.readInt();
    if (!check_signature(&magic))
      return false;

    DDSURFACEDESC2 header;
    crd.read(&header, sizeof(header));

    if (src_mipmaps)
      *src_mipmaps = header.dwMipMapCount;

    if (width)
      *width = header.dwWidth;

    if (height)
      *height = header.dwHeight;

    if ((header.ddpfPixelFormat.dwFlags & DDPF_RGB) && src_format)
      *src_format =
        (header.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS) ? ddstexture::Converter::fmtARGB : ddstexture::Converter::fmtRGB;

    switch (header.ddpfPixelFormat.dwFourCC)
    {
      case FOURCC_DXT1:
        if (src_format)
          *src_format =
            (header.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS ? ddstexture::Converter::fmtDXT1a : ddstexture::Converter::fmtDXT1);
        break;

      case FOURCC_DXT3:
        if (src_format)
          *src_format = ddstexture::Converter::fmtDXT3;
        break;

      case FOURCC_DXT5:
        if (src_format)
          *src_format = ddstexture::Converter::fmtDXT5;
        break;

      default:
        if (!(header.ddpfPixelFormat.dwFlags & DDPF_FOURCC) && (header.ddpfPixelFormat.dwFlags & DDPF_RGB)) // DDPF_RGB
        {
          if (src_format)
            *src_format =
              (header.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS) ? ddstexture::Converter::fmtARGB : ddstexture::Converter::fmtRGB;
          break;
        }
        else
          return false;
    }

    if (src_type)
    {
      if (header.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP)
        *src_type = ddstexture::Converter::typeCube;
      else if (header.ddsCaps.dwCaps2 & DDSCAPS2_VOLUME)
        *src_type = ddstexture::Converter::typeVolume;
      else
        *src_type = ddstexture::Converter::type2D;
    }
  }
  DAGOR_CATCH(const IGenLoad::LoadException &e) { return false; }

  return true;
}
} // namespace ddstexture


//==============================================================================
bool tgaWithAlpha(const char *filename)
{
  FullFileLoadCB crd(filename);
  if (!crd.fileHandle)
    return false;

#pragma pack(push, 1)
  struct TgaHeader
  {
    unsigned char idlen, cmtype, pack;
    unsigned short cmorg, cmlen;
    unsigned char cmes;
    short xo, yo, w, h;
    char bits, desc;
  } hdr;
#pragma pack(pop)

  DAGOR_TRY
  {
    crd.read(&hdr, sizeof(hdr));
    ;
    if (hdr.idlen)
      crd.seekrel(hdr.idlen);
    if (hdr.cmlen)
      crd.seekrel(hdr.cmlen * ((hdr.cmes + 7) / 8));
  }
  DAGOR_CATCH(const IGenLoad::LoadException &e) { return false; }

  return hdr.bits == 32;
}


/*
inline static bool check_dds_signature(void *ptr) { return *(uint32_t*)ptr== 0x20534444; };

//======================================================================================================================
bool dtx_write_to_stream(IGenSave &cwr, const char *filename,
    int addr_mode_u, int addr_mode_v, int HQ_mip, int MQ_mip, int LQ_mip)
{
  String  name(filename);//findExisting(filename);
  file_ptr_t fp = df_open (name, DF_READ );
  if ( !fp )
    return false;

  int size = df_length ( fp );
  cwr.writeInt ( 4+sizeof(DDSTextureLoader::DTXFooter)+size );
  DDSTextureLoader::DTXFooter ftr;
  if (name.find('@'))
    ftr.getFromName(name);
  else
  {
    ftr.addr_mode_u=addr_mode_u;
    ftr.addr_mode_v=addr_mode_v;
    ftr.HQ_mip=HQ_mip;
    ftr.MQ_mip=MQ_mip;
    ftr.LQ_mip=LQ_mip;
  }

  cwr.writeInt ( sizeof(DDSTextureLoader::DTXFooter) );
  cwr.write ( &ftr, sizeof(ftr));

  copy_file_to_stream ( fp, cwr, size );

  df_close ( fp );
  return true;
}



//======================================================================================================================
bool dtx_write_to_file(IGenLoad &crd, const char *filename)
{
  FullFileSaveCB cwr ( filename );
  if ( !cwr.fileHandle )
    return false;

  int datalen=crd.readInt();

  DDSTextureLoader::DTXFooter info;
  int       pos = crd.tell();
  int       magic=crd.readInt();

  if (!check_dds_signature(&magic))
  {
    int footer_size=magic;
    if (sizeof(info)>=footer_size)
      crd.read(&info, footer_size);
    else
    {
      crd.read(&info, sizeof(info));
      crd.seekrel(footer_size-sizeof(info));
    }
    datalen-=footer_size+4;
  }

  if ((info.addr_mode_v & 7)==0)
    info.addr_mode_v=info.addr_mode_u;

  copy_stream_to_stream ( crd, cwr, datalen );
  return true;
}

String dtx_find_existing(const String &fname)
{
  if (fname=="")
    return fname;

  if (!fname.suffix(".dds") && !fname.suffix(".dtx"))
    return fname;

  String  name(fname);
  if (!::dd_file_exist(name))
    name=dtx_footer_simplify_file_name(fname);
  else
    return name;

  if (!::dd_file_exist(name))
    name.replace(".dds", ".dtx");
  else
    return name;

  if (!::dd_file_exist(name))
  {
    String  loc(name);
    ::location_from_path(loc);

    // find some file_name@[].dds - file
    name=::make_full_path(loc, ::get_file_name_wo_ext(name)+"@*.dds");
    alefind_t find;
    if (::dd_find_first(name, 0, &find))
      name=::make_full_path(loc, find.name);
    else
      name="";
    ::dd_find_close(&find);
  }

  return name;
}

String dtx_footer_simplify_file_name(const char *filename)
{
  String  loc, name;
  ::split_path(filename, loc, name);
  const char *info=name.find('@');
  if (!info)
    info=name.find('.');

  if (!info)
    return filename;

  return ::make_full_path(loc, String(name, info)+".dds");
}
*/
