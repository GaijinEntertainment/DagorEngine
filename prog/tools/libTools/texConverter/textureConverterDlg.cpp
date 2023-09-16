#include <texConverter/textureConverterDlg.h>

#include <EditorCore/ec_interface.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_direct.h>
#include <libTools/dtx/dtx.h>
#include <libTools/dtx/dtxHeader.h>
#include <ioSys/dag_fileIo.h>
#include <image/dag_loadImage.h>
#include <image/dag_texPixel.h>

#include <propPanel2/c_panel_base.h>
#include <winGuiWrapper/wgw_dialogs.h>

namespace ddstexture
{
const char *ConverterDlg::addressing[] = {
  "Wrap",
  "Mirror",
  "Clamp",
  "Border",
  "As U",
};

const char *ConverterDlg::formats[] = {
  "Current",
  "ARGB - full alpha",
  "RGB - no alpha",
  "DXT1 - no alpha",
  "DXT1a - 1-bit alpha",
  "DXT3 - explicit alpha",
  "DXT5 - interpolated alpha",
};

const char *ConverterDlg::types[] = {
  "2D",
  "Cube",
  "Volume",
  "NoChange",
};

const char *ConverterDlg::mipmapTypes[] = {
  "None",
  "UseExisting",
  "Generate",
};

const char *ConverterDlg::mipmapFilters[] = {
  "Point",
  "Box",
  "Triangle",
  "Quadratic",
  "Cubic",
  "Catrom",
  "Mitchell",
  "Gaussian",
  "Sinc",
  "Bessel",
  "Hanning",
  "Hamming",
  "Blackman",
  "Kaiser",
};

enum
{
  FILES_COMBOBOX_ID = 100,  // files
  ADDRESSING_U_COMBOBOX_ID, // AddressingU
  ADDRESSING_V_COMBOBOX_ID, // AddressingV

  TYPE_COMBOBOX_ID,          // type
  FORMAT_COMBOBOX_ID,        // format
  MIPMAP_TYPE_COMBOBOX_ID,   // mipmapType
  MIPMAP_FILTER_COMBOBOX_ID, // mipmapFilter

  BROWSE_SOURCE_BUTTON_ID,
  SET_DEFAILT_BUTTON_ID,

  SRC_DIR_EDIT_ID,          // srcDir
  SRC_TYPE_EDIT_ID,         // srcType
  SRC_FORMAT_EDIT_ID,       // srcFormat
  SRC_MIPMAPS_EDIT_ID,      // srcMipmaps
  DST_NAME_MIPMAPS_EDIT_ID, // dstName

  QUALITY_H_EDIT_ID, // QualityH
  QUALITY_M_EDIT_ID, // QualityM
  QUALITY_L_EDIT_ID, // QualityL

  SRC_GROUP_ID,
  DST_GROUP_ID,
  MIPMAP_GROUP_ID,

  ST_ADDRESSING_ID,
  ST_QUALITY_ID,

  MIPMAP_FILTER_WIDTH_FLOAT_EDIT_ID,
  MIPMAP_FILTER_BLUR_FLOAT_EDIT_ID,
  MIPMAP_FILTER_GAMMA_FLOAT_EDIT_ID,

  COPY_TO_SRC_LOCATION_CHECKBOX_ID, // copyToSrcLocation
};

//==============================================================================
//  DDS Converter Dialog
//==============================================================================
ConverterDlg::ConverterDlg() :
  CDialogWindow(NULL, DLG_WIDTH, DLG_HEIGHT, "Texture Importer"),
  specificType((ddstexture::Converter::TextureType)-1),
  noChangeName(false)
{
  mPanel = getPanel();
  G_ASSERT(mPanel && "No panel in CamerasConfigDlg");

  mPanel->createCombo(FILES_COMBOBOX_ID, "", {}, 0);
  mPanel->createButton(BROWSE_SOURCE_BUTTON_ID, "...");

  mPanel->createFileEditBox(SRC_DIR_EDIT_ID, "");
  mPanel->setEnabledById(SRC_DIR_EDIT_ID, false);
  mPanel->createFileEditBox(SRC_TYPE_EDIT_ID, "");
  mPanel->setEnabledById(SRC_TYPE_EDIT_ID, false);
  mPanel->createFileEditBox(SRC_FORMAT_EDIT_ID, "");
  mPanel->setEnabledById(SRC_FORMAT_EDIT_ID, false);
  mPanel->createFileEditBox(SRC_MIPMAPS_EDIT_ID, "");
  mPanel->setEnabledById(SRC_MIPMAPS_EDIT_ID, false);

  mPanel->createButton(SET_DEFAILT_BUTTON_ID, "Default");

  mPanel->createGroup(SRC_GROUP_ID, "Source");
  mPanel->createGroup(DST_GROUP_ID, "Destination");
  mPanel->createGroup(MIPMAP_GROUP_ID, "MipMap filter");

  mPanel->createFileEditBox(DST_NAME_MIPMAPS_EDIT_ID, "");
  mPanel->setEnabledById(DST_NAME_MIPMAPS_EDIT_ID, false);

  mPanel->createStatic(ST_ADDRESSING_ID, "Addressing:");

  Tab<String> addressingTab(midmem);
  for (int i = 0; i < sizeof(addressing) / sizeof(addressing[0]); ++i)
    addressingTab.push_back() = addressing[i];
  mPanel->createCombo(ADDRESSING_U_COMBOBOX_ID, "U", addressingTab, 0);
  mPanel->createCombo(ADDRESSING_V_COMBOBOX_ID, "V", addressingTab, TEXADDR_AS_U - 1);

  mPanel->createStatic(ST_QUALITY_ID, "Quality:");

  mPanel->createEditInt(QUALITY_H_EDIT_ID, "Hi", 0);
  mPanel->createEditInt(QUALITY_M_EDIT_ID, "Med", 1);
  mPanel->createEditInt(QUALITY_L_EDIT_ID, "Low", 2);

  Tab<String> typesTab(midmem);
  for (int i = 0; i < sizeof(types) / sizeof(types[0]); ++i)
    typesTab.push_back() = types[i];
  mPanel->createCombo(TYPE_COMBOBOX_ID, "Type:", typesTab, 0);

  Tab<String> formatsTab(midmem);
  for (int i = 0; i < sizeof(formats) / sizeof(formats[0]); ++i)
    formatsTab.push_back() = formats[i];
  mPanel->createCombo(FORMAT_COMBOBOX_ID, "Format:", formatsTab, 0);

  Tab<String> mipmapTypesTab(midmem);
  for (int i = 0; i < sizeof(mipmapTypes) / sizeof(mipmapTypes[0]); ++i)
    mipmapTypesTab.push_back() = mipmapTypes[i];
  mPanel->createCombo(MIPMAP_TYPE_COMBOBOX_ID, "Type:", mipmapTypesTab, 0);

  Tab<String> mipmapFiltersTab(midmem);
  for (int i = 0; i < sizeof(mipmapFilters) / sizeof(mipmapFilters[0]); ++i)
    mipmapFiltersTab.push_back() = mipmapFilters[i];
  mPanel->createCombo(MIPMAP_FILTER_COMBOBOX_ID, "Filter:", mipmapFiltersTab, options.mipmapFilter);


  mPanel->createEditFloat(MIPMAP_FILTER_WIDTH_FLOAT_EDIT_ID, "Width:", 0.0);
  mPanel->createEditFloat(MIPMAP_FILTER_BLUR_FLOAT_EDIT_ID, "Blur:", 1.0);
  mPanel->createEditFloat(MIPMAP_FILTER_GAMMA_FLOAT_EDIT_ID, "Gamma:", 2.2);

  mPanel->createCheckBox(COPY_TO_SRC_LOCATION_CHECKBOX_ID, "Copy to source location", options.copyTosrcLocation);

  // default settings
  if (specificType == (ddstexture::Converter::TextureType)-1)
    options.type = ddstexture::Converter::type2D;
  options.mipmapFilter = ddstexture::Converter::filterKaiser;
  options.mipmapFilterWidth = 0;
  options.mipmapFilterBlur = 1;
  options.mipmapFilterGamma = 2.2;

  // from old ConverterDlg::init()
  if (noChangeName)
  {
    mPanel->setEnabledById(ADDRESSING_U_COMBOBOX_ID, false);
    mPanel->setEnabledById(ADDRESSING_V_COMBOBOX_ID, false);
    mPanel->setEnabledById(QUALITY_H_EDIT_ID, false);
    mPanel->setEnabledById(QUALITY_M_EDIT_ID, false);
    mPanel->setEnabledById(QUALITY_L_EDIT_ID, false);
    mPanel->setEnabledById(SET_DEFAILT_BUTTON_ID, false);
  }

  if (specificType != (ddstexture::Converter::TextureType)-1)
  {
    options.type = specificType;
    mPanel->setInt(TYPE_COMBOBOX_ID, specificType);
    mPanel->setEnabledById(TYPE_COMBOBOX_ID, false);
  }

  findSources();
}


//==============================================================================
DDSPathName ConverterDlg::convert(const char *src_name, const char *dst_name, bool fill_options_only)
{
  srcFilename = src_name;
  dstFilename = dst_name;

  if (srcFilename == "" || !::dd_file_exist(src_name))
  {
    srcFilename = wingw::file_open_dlg(NULL, "Select texture " + srcFilename.getName(),
      String(512,
        "Texture by name|%s*|All texture files|*.tga;*.dds;*.jpg;*.dtx;*.hdr|"
        "TGA-Files|*.dds|DDS-Files|*.Tga|JPEG-Files|*.jpg|"
        "BMP files|*.bmp|HDR files|*.hdr|DTX-Files|*.dtx|All files|*.*",
        (const char *)srcFilename.getName(false)),
      NULL, srcFilename, srcFilename);

    if (srcFilename == "")
      return "";
  }

  if (showDialog() == DIALOG_ID_OK)
  {
    if (dstFilename == "")
    {
      dstFilename = srcFilename;
      dstFilename.setExt("dds");
      updateDstFilename();
    }

    if (srcFilename != "")
    {
      if (fill_options_only)
        return dstFilename;
      else if (options.convert(srcFilename, dstFilename))
        return dstFilename;
    }

    wingw::message_box(wingw::MBS_OK, "Can't import texture", "from %s\nto %s", srcFilename.str(), dstFilename.str());
  }

  return "";
}


//==============================================================================
void ConverterDlg::findSources()
{
  FilePathName name = srcFilename;

  alefind_t find = {0};
  name.setName(name.getName(false));

  int files_count = 0, selected = 0;
  mPanel->setStrings(FILES_COMBOBOX_ID, {});
  Tab<String> filesTab(midmem);
  int selectItem = 0;
  for (int result = ::dd_find_first(name + "*", 0, &find); result; result = ::dd_find_next(&find), files_count++)
  {
    name.setName(find.name);
    FilePathName ext = name.getExt();
    if (ext == "dds" || ext == "dtx" || ext == "tga" || ext == "jpg" || ext == "hdr" || ext == "bmp" || ext == "psd")
    {
      if (srcFilename == "")
        srcFilename = name;

      filesTab.push_back(name.getName());
      if (name == srcFilename)
        selectItem = files_count;
    }
  }
  mPanel->setStrings(FILES_COMBOBOX_ID, filesTab);
  mPanel->setInt(FILES_COMBOBOX_ID, selectItem);
  ::dd_find_close(&find);

  mPanel->setText(SRC_DIR_EDIT_ID, String("Path: ") + srcFilename.getPath());
  fileSelected();
}


//==============================================================================
void ConverterDlg::fileSelected()
{
  int src_type = -1, src_format = -1, src_mipmaps = 0;

  // default settings
  options.format = ddstexture::Converter::fmtCurrent;
  options.mipmapType = ddstexture::Converter::mipmapUseExisting;

  if (srcFilename.getExt() == "psd")
  {
    bool used_alpha;
    TexImage32 *img = load_image(srcFilename, tmpmem, &used_alpha);
    tmpmem->free(img);

    if (!img || used_alpha)
    {
      src_format = ddstexture::Converter::fmtARGB;
      options.format = ddstexture::Converter::fmtDXT5;
    }
    else
    {
      src_format = ddstexture::Converter::fmtRGB;
      options.format = ddstexture::Converter::fmtDXT1;
    }

    options.mipmapType = ddstexture::Converter::mipmapGenerate;
    if (specificType == (ddstexture::Converter::TextureType)-1)
      src_type = options.type = ddstexture::Converter::type2D;
  }
  else if (srcFilename.getExt() == "jpg")
  {
    src_format = ddstexture::Converter::fmtRGB;
    options.format = ddstexture::Converter::fmtDXT1;
    options.mipmapType = ddstexture::Converter::mipmapGenerate;
    if (specificType == (ddstexture::Converter::TextureType)-1)
      src_type = options.type = ddstexture::Converter::type2D;
  }
  else if (srcFilename.getExt() == "tga")
  {
    if (tgaWithAlpha(srcFilename))
    {
      src_format = ddstexture::Converter::fmtARGB;
      options.format = ddstexture::Converter::fmtDXT5;
    }
    else
    {
      src_format = ddstexture::Converter::fmtRGB;
      options.format = ddstexture::Converter::fmtDXT1;
    }

    options.mipmapType = ddstexture::Converter::mipmapGenerate;
    if (specificType == (ddstexture::Converter::TextureType)-1)
      src_type = options.type = ddstexture::Converter::type2D;
  }
  else if (srcFilename.getExt() == "dds" || srcFilename.getExt() == "dtx")
  {
    getDdsInfo(srcFilename, &src_type, &src_format, &src_mipmaps);
    if (src_type != -1 && specificType == (ddstexture::Converter::TextureType)-1)
      options.type = ddstexture::Converter::typeDefault;

    Header header;
    header.getFromFile(srcFilename);
    mPanel->setInt(ADDRESSING_U_COMBOBOX_ID, header.addr_mode_u - 1);
    mPanel->setInt(ADDRESSING_V_COMBOBOX_ID, (header.addr_mode_v == header.addr_mode_u ? TEXADDR_AS_U : header.addr_mode_v) - 1);
    mPanel->setInt(QUALITY_H_EDIT_ID, header.HQ_mip);
    mPanel->setInt(QUALITY_M_EDIT_ID, header.MQ_mip);
    mPanel->setInt(QUALITY_L_EDIT_ID, header.LQ_mip);
  }

  mPanel->setText(SRC_TYPE_EDIT_ID, String("Type: ") + (src_type == -1 ? "unknown" : types[src_type]));
  mPanel->setText(SRC_FORMAT_EDIT_ID, String("Format: ") + (src_format == -1 ? "unknown" : formats[src_format]));
  mPanel->setText(SRC_MIPMAPS_EDIT_ID, String(128, "Mipmaps: %d", src_mipmaps));

  mPanel->setInt(TYPE_COMBOBOX_ID, options.type);
  mPanel->setInt(FORMAT_COMBOBOX_ID, options.format);
  mPanel->setInt(MIPMAP_TYPE_COMBOBOX_ID, options.mipmapType);
  // if (src_type != -1 && specificType == (ddstexture::Converter::TextureType)-1)
  //   type.select(options.type);

  updateDstFilename();
}


//==============================================================================
void ConverterDlg::updateDstFilename()
{
  DDSPathName str = (dstFilename == "" ? srcFilename : dstFilename);

  if (!noChangeName)
  {
    Header header;
    header.addr_mode_u = mPanel->getInt(ADDRESSING_U_COMBOBOX_ID) + 1; // AddressingU.getSel()+1;
    header.addr_mode_v = mPanel->getInt(ADDRESSING_V_COMBOBOX_ID) + 1; // AddressingV.getSel()+1;
    if (header.addr_mode_v == TEXADDR_AS_U)
      header.addr_mode_v = header.addr_mode_u;
    header.HQ_mip = mPanel->getInt(QUALITY_H_EDIT_ID); // QualityH.getVal();
    header.MQ_mip = mPanel->getInt(QUALITY_M_EDIT_ID); // QualityM.getVal();
    header.LQ_mip = mPanel->getInt(QUALITY_L_EDIT_ID); // QualityL.getVal();

    str = header.makeName(str);
  }

  mPanel->setText(DST_NAME_MIPMAPS_EDIT_ID, str);

  if (dstFilename != "")
    dstFilename = str;
}


void ConverterDlg::onClick(int pcb_id, PropertyContainerControlBase *panel)
{
  switch (pcb_id)
  {
    case BROWSE_SOURCE_BUTTON_ID:
    {
      String name = wingw::file_open_dlg(NULL, "Select texture",
        "PSD-Files|*.psd|DDS-Files|*.dds|TGA-Files|*.Tga|BMP files|*.bmp|JPEG-Files|*.jpg|"
        "HDR files|*.hdr|DTX-Files|*.dtx|All files|*.*",
        "dds", srcFilename, srcFilename);
      if (name != "")
      {
        srcFilename = name;
        dstFilename = name;
        findSources();
      }
    }
    break;

    case SET_DEFAILT_BUTTON_ID:
    {
      panel->setInt(ADDRESSING_U_COMBOBOX_ID, TEXADDR_WRAP - 1);
      panel->setInt(ADDRESSING_V_COMBOBOX_ID, TEXADDR_AS_U - 1);
      panel->setInt(QUALITY_H_EDIT_ID, 0);
      panel->setInt(QUALITY_M_EDIT_ID, 1);
      panel->setInt(QUALITY_L_EDIT_ID, 2);
      updateDstFilename();
    }
    break;

    case COPY_TO_SRC_LOCATION_CHECKBOX_ID: options.copyTosrcLocation = panel->getBool(COPY_TO_SRC_LOCATION_CHECKBOX_ID); break;
  };
}

void ConverterDlg::onChange(int pcb_id, PropertyContainerControlBase *panel)
{
  switch (pcb_id)
  {
    case FORMAT_COMBOBOX_ID: updateDstFilename();
    case TYPE_COMBOBOX_ID:
    case MIPMAP_TYPE_COMBOBOX_ID:
    case MIPMAP_FILTER_COMBOBOX_ID:
    {
      options.type = (ddstexture::Converter::TextureType)panel->getInt(TYPE_COMBOBOX_ID);                   // type.getSel();
      options.format = (ddstexture::Converter::Format)panel->getInt(FORMAT_COMBOBOX_ID);                    // format.getSel();
      options.mipmapType = (ddstexture::Converter::MipmapType)panel->getInt(MIPMAP_TYPE_COMBOBOX_ID);       // mipmapType.getSel();
      options.mipmapFilter = (ddstexture::Converter::MipmapFilter)panel->getInt(MIPMAP_FILTER_COMBOBOX_ID); // mipmapFilter.getSel();
    }
    break;

    case FILES_COMBOBOX_ID:
    {
      srcFilename.setName(panel->getText(FILES_COMBOBOX_ID));
      fileSelected();
    }
    break;

    case ADDRESSING_U_COMBOBOX_ID:
    case ADDRESSING_V_COMBOBOX_ID:
    case QUALITY_H_EDIT_ID:
    case QUALITY_M_EDIT_ID:
    case QUALITY_L_EDIT_ID: updateDstFilename(); break;

    case MIPMAP_FILTER_WIDTH_FLOAT_EDIT_ID:
      options.mipmapFilterWidth = panel->getFloat(MIPMAP_FILTER_WIDTH_FLOAT_EDIT_ID); // mipmapFilterWidth.getVal();
      break;

    case MIPMAP_FILTER_BLUR_FLOAT_EDIT_ID:
      options.mipmapFilterBlur = panel->getFloat(MIPMAP_FILTER_BLUR_FLOAT_EDIT_ID); // mipmapFilterBlur.getVal();
      break;

    case MIPMAP_FILTER_GAMMA_FLOAT_EDIT_ID:
      options.mipmapFilterGamma = panel->getFloat(MIPMAP_FILTER_GAMMA_FLOAT_EDIT_ID); // mipmapFilterGamma.getVal();
      break;
  };
}
} // namespace ddstexture