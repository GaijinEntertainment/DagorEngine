// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/commonWindow/dialogWindow.h>

#include <util/dag_string.h>
#include <libTools/dtx/dtx.h>
#include <libTools/util/de_TextureName.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>

namespace PropPanel
{
class ContainerPropertyControl;
}

namespace ddstexture
{
//==============================================================================
//  DDS Converter Dialog
//==============================================================================
class ConverterDlg : public PropPanel::DialogWindow
{
public:
  ConverterDlg();

  inline void setType(ddstexture::Converter::TextureType specific_type) { specificType = specific_type; }
  inline void noChangeDstName() { noChangeName = true; }
  inline ddstexture::Converter &getConverter() { return options; }
  inline const FilePathName &getSource() { return srcFilename; }

  // return converted dds file-path-name or ""
  // if dst_path_name == "" , dst placed to src folder
  DDSPathName convert(const char *src_path_name, const char *dst_path_name, bool fill_options_only = false);

protected:
  enum
  {
    STR_HEIGHT = 23,
    STR_STEP = 25,
    BORDER = 10,
    ST_OFFSET = 3,
    TEXADDR_AS_U = 5,

    // ids
    ID_BROWSE_SOURCE,
  };

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  void findSources();
  void fileSelected();
  void updateDstFilename();

  ddstexture::Converter options;
  FilePathName srcFilename, dstFilename;
  ddstexture::Converter::TextureType specificType;
  bool noChangeName;

  static const char *addressing[], *formats[], *types[], *mipmapTypes[], *mipmapFilters[];

  PropPanel::ContainerPropertyControl *mPanel;
};
} // namespace ddstexture
