// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "canopyEditor.h"

#include "av_appwnd.h"

#include <EditorCore/ec_modelessWindowController.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <de3_interface.h>
#include <ioSys/dag_dataBlockCommentsDef.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_memIo.h>
#include <libTools/util/blkUtil.h>
#include <libTools/util/strUtil.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <perfMon/dag_cpuFreq.h>
#include <propPanel/c_common.h>
#include <propPanel/commonWindow/multiListDialog.h>
#include <propPanel/constants.h>
#include <gui/dag_imgui.h>
#include <imgui/imgui.h>
#include <winGuiWrapper/wgw_dialogs.h>

#include <ImGuiColorTextEdit/TextEditor.h>

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

namespace
{
static DataBlock canopy_ui_state;

enum
{
  ID_CANOPY_BLK_FILE = 1,
  ID_CANOPY_SHOW_FX,
  ID_CANOPY_ADD_PARAMS,
  ID_CANOPY_PARAMETERS_GROUP,
  ID_CANOPY_PARAMETERS_TEXT_GROUP,
  ID_CANOPY_PARAMETERS_TEXT,
  ID_CANOPY_CANOPY_TYPE,

  ID_CANOPY_ACTION_BASE = 1000,
  ID_CANOPY_VALUE_BASE = 20000,
};

enum CanopyTypeValue
{
  CANOPY_TYPE_BOX = 0,
  CANOPY_TYPE_TRIANGLE,
  CANOPY_TYPE_SPHERE,
};

struct AddableParamTemplate
{
  String displayPath;
  DataBlock paramBlock;
  eastl::vector<String> blockPath;
  String blockName;
  bool isBlock = false;
};

static bool has_lod_suffix(const char *suffix)
{
  if (!suffix || suffix[0] != '.' || suffix[1] != 'l' || suffix[2] != 'o' || suffix[3] != 'd')
    return false;

  const char *digits = suffix + 4;
  if (!isdigit((unsigned char)*digits))
    return false;

  for (; *digits; ++digits)
    if (!isdigit((unsigned char)*digits))
      return false;

  return true;
}

static String normalize_canopy_asset_name(const char *asset_name)
{
  String normalized = DagorAsset::fpath2asset(asset_name);
  trim(normalized);

  const char *lastDot = strrchr(normalized, '.');
  if (lastDot && has_lod_suffix(lastDot))
    normalized = String::mk_sub_str(normalized, lastDot);

  return normalized;
}

static String normalize_line_endings(const char *text, const char *line_ending)
{
  String normalized;
  const char *targetLineEnding = (line_ending && *line_ending) ? line_ending : "\n";
  for (const char *cursor = text; cursor && *cursor; ++cursor)
  {
    if (*cursor == '\r')
    {
      if (cursor[1] == '\n')
        ++cursor;
      normalized += targetLineEnding;
    }
    else if (*cursor == '\n')
    {
      normalized += targetLineEnding;
    }
    else
    {
      normalized += *cursor;
    }
  }

  return normalized;
}

static bool read_file_to_string(const char *path, String &content)
{
  content.clear();

  FullFileLoadCB loadCb(path, DF_READ | DF_IGNORE_MISSING);
  if (!loadCb.fileHandle)
    return false;

  const int dataSize = df_length(loadCb.fileHandle);
  if (dataSize < 0)
    return false;

  Tab<char> buffer(tmpmem);
  buffer.resize(dataSize + 1);
  if (loadCb.tryRead(buffer.data(), dataSize) != dataSize)
    return false;

  buffer[dataSize] = '\0';
  content.setStr(buffer.data(), dataSize);
  return true;
}

static bool write_string_to_file(const char *path, const String &content)
{
  String tempPath(0, "%s.$tmp$.XXXXXX", path);
  file_ptr_t tempFile = df_mkstemp(&tempPath[0]);
  if (!tempFile)
    return false;

  const bool writeSucceeded = df_write(tempFile, content.str(), content.length()) == content.length();
  df_flush(tempFile);
  const bool closeSucceeded = df_close(tempFile) == 0;

  if (!writeSucceeded || !closeSucceeded)
  {
    dd_erase(tempPath);
    return false;
  }

  if (dd_rename(tempPath, path))
    return true;

  dd_erase(tempPath);
  return false;
}

class DataBlockTextFlagsGuard
{
public:
  DataBlockTextFlagsGuard()
  {
    oldParseIncludesAsParams = DataBlock::parseIncludesAsParams;
    oldParseCommentsAsParams = DataBlock::parseCommentsAsParams;
    oldParseOverridesNotApply = DataBlock::parseOverridesNotApply;
    oldAllowSimpleString = DataBlock::allowSimpleString;

    DataBlock::parseIncludesAsParams = true;
    DataBlock::parseCommentsAsParams = true;
    DataBlock::parseOverridesNotApply = true;
    DataBlock::allowSimpleString = true;
  }

  ~DataBlockTextFlagsGuard()
  {
    DataBlock::parseIncludesAsParams = oldParseIncludesAsParams;
    DataBlock::parseCommentsAsParams = oldParseCommentsAsParams;
    DataBlock::parseOverridesNotApply = oldParseOverridesNotApply;
    DataBlock::allowSimpleString = oldAllowSimpleString;
  }

private:
  bool oldParseIncludesAsParams = false;
  bool oldParseCommentsAsParams = false;
  bool oldParseOverridesNotApply = false;
  bool oldAllowSimpleString = false;
};

static bool load_blk_text(const char *path, DataBlock &blk)
{
  DataBlockTextFlagsGuard guard;
  return blk.load(path);
}

static bool load_blk_text_from_string(const char *text, int text_length, const char *source_name, DataBlock &blk)
{
  DataBlockTextFlagsGuard guard;
  return blk.loadText(text, text_length, source_name);
}

static String build_blk_text(const DataBlock &blk, const char *line_ending)
{
  DataBlockTextFlagsGuard guard;
  DynamicMemGeneralSaveCB saveCb(tmpmem);
  blk.saveToTextStream(saveCb);
  saveCb.write("\0", 1);
  return normalize_line_endings((const char *)saveCb.data(), line_ending);
}

static bool try_parse_int64(const char *text, int64_t &value)
{
  if (is_empty_string(text))
    return false;

  errno = 0;
  char *end_ptr = nullptr;
  const long long parsedValue = strtoll(text, &end_ptr, 10);
  if (end_ptr == text || errno != 0)
    return false;

  while (*end_ptr && isspace((unsigned char)*end_ptr))
    ++end_ptr;

  if (*end_ptr != '\0')
    return false;

  value = parsedValue;
  return true;
}

static int round_to_int(float value) { return value >= 0.0f ? int(value + 0.5f) : int(value - 0.5f); }

static Tab<String> build_yes_no_values()
{
  Tab<String> values(tmpmem);
  values.push_back() = "yes";
  values.push_back() = "no";
  return values;
}

static Tab<String> build_canopy_type_values()
{
  Tab<String> values(tmpmem);
  values.push_back() = "Box";
  values.push_back() = "Triangle";
  values.push_back() = "Sphere";
  return values;
}

static bool is_canopy_type_param_name(const char *name)
{
  return strcmp(name, "canopyTriangle") == 0 || strcmp(name, "canopySphere") == 0;
}

static bool is_datablock_comment_name(const char *name) { return name && CHECK_COMMENT_PREFIX(name); }

static bool is_addable_top_level_param_name(const char *name) { return strcmp(name, "name") != 0 && !is_canopy_type_param_name(name); }

static bool remove_top_level_params_by_name(DataBlock &block, const char *name)
{
  bool changed = false;
  for (int i = block.paramCount() - 1; i >= 0; --i)
    if (strcmp(block.getParamName(i), name) == 0)
    {
      block.removeParam(i);
      changed = true;
    }

  return changed;
}

static bool remove_top_level_triangle_only_canopy_params(DataBlock &block)
{
  return remove_top_level_params_by_name(block, "forcePyramidCanopy");
}

static int get_top_level_canopy_type_value(const DataBlock &block)
{
  int canopyType = CANOPY_TYPE_BOX;
  for (int i = 0; i < block.paramCount(); ++i)
  {
    if (block.getParamType(i) != DataBlock::TYPE_BOOL || !block.getBool(i))
      continue;

    const char *name = block.getParamName(i);
    if (strcmp(name, "canopyTriangle") == 0)
      canopyType = CANOPY_TYPE_TRIANGLE;
    else if (strcmp(name, "canopySphere") == 0)
      canopyType = CANOPY_TYPE_SPHERE;
  }

  return canopyType;
}

static bool normalize_top_level_canopy_type_params(DataBlock &block)
{
  int winnerIndex = -1;
  for (int i = 0; i < block.paramCount(); ++i)
    if (is_canopy_type_param_name(block.getParamName(i)) && block.getParamType(i) == DataBlock::TYPE_BOOL && block.getBool(i))
      winnerIndex = i;

  bool changed = false;
  for (int i = block.paramCount() - 1; i >= 0; --i)
  {
    if (!is_canopy_type_param_name(block.getParamName(i)))
      continue;

    const bool keepParam = i == winnerIndex && block.getParamType(i) == DataBlock::TYPE_BOOL && block.getBool(i);
    if (!keepParam)
    {
      block.removeParam(i);
      changed = true;
    }
  }

  if (get_top_level_canopy_type_value(block) != CANOPY_TYPE_TRIANGLE)
    changed = remove_top_level_triangle_only_canopy_params(block) || changed;

  return changed;
}

static int find_top_level_param(const DataBlock &block, const char *name)
{
  for (int i = 0; i < block.paramCount(); ++i)
    if (strcmp(block.getParamName(i), name) == 0)
      return i;

  return -1;
}

static void move_param_after_name(DataBlock &block, int param_index)
{
  int nameIndex = find_top_level_param(block, "name");
  if (nameIndex < 0 || param_index == nameIndex + 1)
    return;

  while (param_index > nameIndex + 1)
  {
    block.swapParams(param_index, param_index - 1);
    --param_index;
  }
}

static void set_top_level_name_value(DataBlock &block, const char *asset_name)
{
  const int nameIndex = find_top_level_param(block, "name");
  if (nameIndex >= 0 && block.getParamType(nameIndex) == DataBlock::TYPE_STRING)
  {
    block.setStr(nameIndex, asset_name);
    return;
  }

  block.addStr("name", asset_name);
  for (int paramIndex = block.paramCount() - 1; paramIndex > 0; --paramIndex)
    block.swapParams(paramIndex, paramIndex - 1);
}

static bool set_top_level_canopy_type_value(DataBlock &block, int canopy_type)
{
  if (canopy_type == CANOPY_TYPE_BOX)
    return remove_top_level_triangle_only_canopy_params(block) | remove_top_level_params_by_name(block, "canopyTriangle") |
           remove_top_level_params_by_name(block, "canopySphere");

  bool changed = normalize_top_level_canopy_type_params(block);
  if (canopy_type != CANOPY_TYPE_TRIANGLE)
    changed = remove_top_level_triangle_only_canopy_params(block) || changed;

  const char *desiredName = canopy_type == CANOPY_TYPE_TRIANGLE ? "canopyTriangle" : "canopySphere";
  for (int i = 0; i < block.paramCount(); ++i)
  {
    if (!is_canopy_type_param_name(block.getParamName(i)))
      continue;

    if (block.getParamType(i) != DataBlock::TYPE_BOOL || !block.getBool(i) || strcmp(block.getParamName(i), desiredName) != 0)
    {
      block.changeParamName(i, desiredName);
      block.setBool(i, true);
      changed = true;
    }
    return changed;
  }

  block.addBool(desiredName, true);
  move_param_after_name(block, block.paramCount() - 1);
  return true;
}

static void create_canopy_type_row(PropPanel::ContainerPropertyControl &panel, int canopy_type)
{
  PropPanel::ContainerPropertyControl *row = panel.createContainer(-1);
  row->setUseFixedWidthColumnsWithZeroWidthStretch();
  row->setHorizontalSpaceBetweenControls(hdpi::_pxActual(0));

  const int firstChildIndex = row->getChildCount();
  static const char caption[] = "CanopyType:";
  row->createStatic(-1, caption, true, true);
  row->createCombo(ID_CANOPY_CANOPY_TYPE, "", build_canopy_type_values(), canopy_type, true, false);

  if (PropPanel::PropertyControlBase *captionControl = row->getByIndex(firstChildIndex))
    captionControl->setWidth(hdpi::_pxActual((int)(ImGui::CalcTextSize(caption).x + 2.0f)));
}

static String build_addable_param_display_path(const eastl::vector<String> &block_path, const char *name)
{
  String path;
  for (int i = 0; i < block_path.size(); ++i)
  {
    if (!path.empty())
      path += ".";
    path += block_path[i];
  }

  if (!path.empty())
    path += ".";
  path += name;
  return path;
}

static String build_addable_block_display_name(const char *name) { return String(0, "%s {BLOCK}", name); }

static bool has_addable_param_template(const eastl::vector<AddableParamTemplate> &templates, const char *display_path)
{
  for (const AddableParamTemplate &item : templates)
    if (strcmp(item.displayPath, display_path) == 0)
      return true;

  return false;
}

static bool copy_canopy_blk_param(const DataBlock &source, int param_index, DataBlock &dest)
{
  if (source.getParamType(param_index) == DataBlock::TYPE_INT64)
  {
    dest.addInt64(source.getParamName(param_index), source.getInt64(param_index));
    return true;
  }

  return blk_util::copyBlkParam(source, param_index, dest);
}

static void collect_addable_param_templates_from_dmg(const DataBlock &block, eastl::vector<String> &block_path,
  eastl::vector<AddableParamTemplate> &templates)
{
  for (int i = 0; i < block.paramCount(); ++i)
  {
    const char *name = block.getParamName(i);
    if (is_datablock_comment_name(name))
      continue;

    if (block_path.empty() && !is_addable_top_level_param_name(name))
      continue;

    const String displayPath = build_addable_param_display_path(block_path, name);
    if (has_addable_param_template(templates, displayPath))
      continue;

    templates.push_back();
    AddableParamTemplate &templateItem = templates.back();
    templateItem.displayPath = displayPath;
    templateItem.blockPath = block_path;
    copy_canopy_blk_param(block, i, templateItem.paramBlock);
  }

  for (int i = 0; i < block.blockCount(); ++i)
  {
    const DataBlock *child = block.getBlock(i);
    if (!child)
      continue;
    if (is_datablock_comment_name(child->getBlockName()))
      continue;

    if (block_path.empty())
    {
      const String displayPath = build_addable_block_display_name(child->getBlockName());
      if (!has_addable_param_template(templates, displayPath))
      {
        templates.push_back();
        AddableParamTemplate &templateItem = templates.back();
        templateItem.displayPath = displayPath;
        templateItem.blockName = child->getBlockName();
        templateItem.isBlock = true;
      }
    }

    block_path.push_back(String(child->getBlockName()));
    collect_addable_param_templates_from_dmg(*child, block_path, templates);
    block_path.pop_back();
  }
}

static void collect_addable_param_templates(const DataBlock &block, eastl::vector<AddableParamTemplate> &templates)
{
  for (int i = 0; i < block.blockCount(); ++i)
  {
    const DataBlock *child = block.getBlock(i);
    if (!child)
      continue;

    if (strcmp(child->getBlockName(), "dmg") == 0)
    {
      eastl::vector<String> blockPath;
      collect_addable_param_templates_from_dmg(*child, blockPath, templates);
    }

    collect_addable_param_templates(*child, templates);
  }
}

template <typename Block>
static Block *find_param_parent_block(Block &block, const eastl::vector<String> &block_path)
{
  Block *currentBlock = &block;
  for (const String &blockName : block_path)
  {
    auto foundBlock = currentBlock->getBlockByName(blockName);
    if (!foundBlock)
      return nullptr;

    currentBlock = foundBlock;
  }

  return currentBlock;
}

static bool has_top_level_block(const DataBlock &block, const char *name)
{
  for (int i = 0; i < block.blockCount(); ++i)
    if (const DataBlock *child = block.getBlock(i))
      if (strcmp(child->getBlockName(), name) == 0)
        return true;

  return false;
}

static bool has_param_at_path(const DataBlock &block, const eastl::vector<String> &block_path, const char *name)
{
  const DataBlock *paramParent = find_param_parent_block(block, block_path);
  if (!paramParent)
    return false;

  for (int i = 0; i < paramParent->paramCount(); ++i)
    if (strcmp(paramParent->getParamName(i), name) == 0)
      return true;

  return false;
}

static bool can_add_param_template(const DataBlock &block, const AddableParamTemplate &template_item)
{
  if (template_item.isBlock)
    return template_item.blockPath.empty() && !has_top_level_block(block, template_item.blockName);

  const DataBlock *paramParent = find_param_parent_block(block, template_item.blockPath);
  return paramParent && template_item.paramBlock.paramCount() == 1 &&
         !has_param_at_path(block, template_item.blockPath, template_item.paramBlock.getParamName(0));
}

static bool insert_addable_param_template(DataBlock &block, const AddableParamTemplate &template_item)
{
  if (template_item.isBlock)
  {
    if (has_top_level_block(block, template_item.blockName))
      return false;

    block.addNewBlock(template_item.blockName);
    return true;
  }

  if (template_item.paramBlock.paramCount() != 1)
    return false;

  const char *paramName = template_item.paramBlock.getParamName(0);
  if (has_param_at_path(block, template_item.blockPath, paramName))
    return false;

  DataBlock *paramParent = find_param_parent_block(block, template_item.blockPath);
  if (!paramParent)
    return false;

  copy_canopy_blk_param(template_item.paramBlock, 0, *paramParent);
  if (template_item.blockPath.empty())
    move_param_after_name(*paramParent, paramParent->paramCount() - 1);
  return true;
}

static bool has_available_addable_param_templates(const DataBlock &loaded_blk, const DataBlock &editable_block)
{
  eastl::vector<AddableParamTemplate> templates;
  collect_addable_param_templates(loaded_blk, templates);

  for (const AddableParamTemplate &item : templates)
    if (can_add_param_template(editable_block, item))
      return true;

  return false;
}

template <typename Block>
static Block *get_block_by_path(Block &root, const eastl::vector<int> &path)
{
  Block *block = &root;
  for (int index : path)
  {
    if (index < 0 || index >= block->blockCount())
      return nullptr;

    block = block->getBlock(index);
  }

  return block;
}

static bool try_get_top_level_real(const DataBlock &block, const char *name, float &value)
{
  const int paramIndex = find_top_level_param(block, name);
  if (paramIndex < 0)
    return false;

  if (block.getParamType(paramIndex) == DataBlock::TYPE_REAL)
  {
    value = block.getReal(paramIndex);
    return true;
  }

  if (block.getParamType(paramIndex) == DataBlock::TYPE_INT)
  {
    value = (float)block.getInt(paramIndex);
    return true;
  }

  if (block.getParamType(paramIndex) == DataBlock::TYPE_INT64)
  {
    value = (float)block.getInt64(paramIndex);
    return true;
  }

  return false;
}

static void invalidate_viewport_cache()
{
  if (EDITORCORE)
  {
    EDITORCORE->invalidateViewportCache();
    EDITORCORE->updateViewports();
  }
}

static bool try_get_top_level_string(const DataBlock &block, const char *name, String &value)
{
  const int paramIndex = find_top_level_param(block, name);
  if (paramIndex < 0 || block.getParamType(paramIndex) != DataBlock::TYPE_STRING)
    return false;

  value = block.getStr(paramIndex);
  return true;
}

static DataBlock *find_matching_dmg_block(DataBlock &block, const char *asset_name, const char *normalized_asset_name,
  eastl::vector<int> &path)
{
  for (int i = 0; i < block.blockCount(); ++i)
  {
    DataBlock *child = block.getBlock(i);
    if (!child)
      continue;

    if (strcmp(child->getBlockName(), "dmg") == 0)
    {
      for (int paramIndex = child->paramCount() - 1; paramIndex >= 0; --paramIndex)
      {
        if (child->getParamType(paramIndex) != DataBlock::TYPE_STRING || strcmp(child->getParamName(paramIndex), "name") != 0)
          continue;

        if (strcmp(child->getStr(paramIndex), asset_name) == 0)
        {
          path.push_back(i);
          return child;
        }

        if (!normalized_asset_name || !*normalized_asset_name)
          continue;

        const String normalizedChildAssetName = normalize_canopy_asset_name(child->getStr(paramIndex));
        if (!normalizedChildAssetName.empty() && strcmp(normalizedChildAssetName.str(), normalized_asset_name) == 0)
        {
          path.push_back(i);
          return child;
        }
      }
    }

    path.push_back(i);
    if (DataBlock *nestedMatch = find_matching_dmg_block(*child, asset_name, normalized_asset_name, path))
      return nestedMatch;
    path.pop_back();
  }

  return nullptr;
}

static bool has_name_token(const char *name, const char *token)
{
  if (is_empty_string(name) || is_empty_string(token))
    return false;

  const int tokenLength = (int)strlen(token);
  const char *tokenStart = name;
  while (*tokenStart)
  {
    const char *tokenEnd = tokenStart;
    while (*tokenEnd && *tokenEnd != '_')
      ++tokenEnd;

    if (tokenEnd - tokenStart == tokenLength && strncmp(tokenStart, token, tokenLength) == 0)
      return true;

    tokenStart = *tokenEnd ? tokenEnd + 1 : tokenEnd;
  }

  return false;
}

static const char *find_name_size_token(const char *name)
{
  static const char *sizeTokens[] = {"medium", "small", "large", "tall"};
  for (const char *sizeToken : sizeTokens)
    if (has_name_token(name, sizeToken))
      return sizeToken;

  return nullptr;
}

static const char *get_dmg_block_asset_name(const DataBlock &dmg_block)
{
  for (int i = dmg_block.paramCount() - 1; i >= 0; --i)
    if (dmg_block.getParamType(i) == DataBlock::TYPE_STRING && strcmp(dmg_block.getParamName(i), "name") == 0)
      return dmg_block.getStr(i);

  return nullptr;
}

static void collect_dmg_blocks_in_order(const DataBlock &block, eastl::vector<const DataBlock *> &dmg_blocks)
{
  for (int i = 0; i < block.blockCount(); ++i)
  {
    const DataBlock *child = block.getBlock(i);
    if (!child)
      continue;

    if (strcmp(child->getBlockName(), "dmg") == 0)
      dmg_blocks.push_back(child);

    collect_dmg_blocks_in_order(*child, dmg_blocks);
  }
}

static const DataBlock *find_first_dmg_block_by_type_and_size(const eastl::vector<const DataBlock *> &dmg_blocks,
  const char *type_token, const char *size_token)
{
  for (const DataBlock *dmgBlock : dmg_blocks)
  {
    const char *blockAssetName = get_dmg_block_asset_name(*dmgBlock);
    if (is_empty_string(blockAssetName) || !has_name_token(blockAssetName, type_token))
      continue;

    if (size_token && !has_name_token(blockAssetName, size_token))
      continue;

    return dmgBlock;
  }

  return nullptr;
}

static const DataBlock *find_default_dmg_template_block(const DataBlock &block, const char *normalized_asset_name)
{
  eastl::vector<const DataBlock *> dmgBlocks;
  collect_dmg_blocks_in_order(block, dmgBlocks);
  if (dmgBlocks.empty())
    return nullptr;

  const bool wantsBush = has_name_token(normalized_asset_name, "bush");
  const char *primaryTypeToken = wantsBush ? "bush" : "tree";
  const char *sizeToken = find_name_size_token(normalized_asset_name);

  if (const DataBlock *matchedBlock = find_first_dmg_block_by_type_and_size(dmgBlocks, primaryTypeToken, sizeToken))
    return matchedBlock;

  if (const DataBlock *matchedBlock = find_first_dmg_block_by_type_and_size(dmgBlocks, primaryTypeToken, nullptr))
    return matchedBlock;

  if (wantsBush)
    return find_first_dmg_block_by_type_and_size(dmgBlocks, "tree", nullptr);

  return nullptr;
}

static int find_last_top_level_dmg_index(const DataBlock &block)
{
  int index = -1;
  for (int i = 0; i < block.blockCount(); ++i)
    if (const DataBlock *child = block.getBlock(i))
      if (strcmp(child->getBlockName(), "dmg") == 0)
        index = i;

  return index;
}

static float get_canopy_save_footer_height()
{
  const ImGuiStyle &style = ImGui::GetStyle();
  return ImGui::GetFrameHeight() + style.WindowPadding.y * 2.0f;
}
} // namespace

DagorAsset *resolve_canopy_fx_asset(const char *name)
{
  if (is_empty_string(name))
    return nullptr;

  const int fxAssetType = DAEDITOR3.getAssetTypeId("fx");
  const int efxAssetType = DAEDITOR3.getAssetTypeId("efx");
  DagorAsset *fxAsset = fxAssetType >= 0 ? DAEDITOR3.getAssetByName(name, fxAssetType) : nullptr;
  DagorAsset *efxAsset = !fxAsset && efxAssetType >= 0 ? DAEDITOR3.getAssetByName(name, efxAssetType) : nullptr;
  DagorAsset *resolvedFxAsset = fxAsset ? fxAsset : efxAsset;
  if (resolvedFxAsset)
    return resolvedFxAsset;

  DagorAsset *genericAsset = DAEDITOR3.getAssetByName(name);
  return genericAsset && (genericAsset->getType() == fxAssetType || genericAsset->getType() == efxAssetType) ? genericAsset : nullptr;
}

CanopyEditorWindow::CanopyEditorWindow() :
  PropPanel::PanelWindowPropertyControl(0, this, nullptr, 0, 0, hdpi::Px(0), hdpi::Px(0), "Canopy Editor"),
  parametersTextEditor(new TextEditor())
{
  Tab<String> masks(tmpmem);
  masks.push_back() = "BLK files|*.blk";

  parametersTextEditor->SetPalette(TextEditor::PaletteId::Light);
  parametersTextEditor->SetShowLineNumbersEnabled(false);
  parametersTextEditor->SetLanguageDefinition(TextEditor::LanguageDefinitionId::Blk);

  createFileButton(ID_CANOPY_BLK_FILE, "Canopy BLK");
  setStrings(ID_CANOPY_BLK_FILE, masks);
  createCheckBox(ID_CANOPY_SHOW_FX, "Show FX", showFx);
  createButton(ID_CANOPY_ADD_PARAMS, "Add params");

  parametersGroup = createGroup(ID_CANOPY_PARAMETERS_GROUP, "Parameters");
  parametersGroup->setBoolValue(false);

  textEditorGroup = createGroup(ID_CANOPY_PARAMETERS_TEXT_GROUP, "Parameters Text Editor");
  textEditorGroup->setBoolValue(false);
  textEditorGroup->createCustomControlHolder(ID_CANOPY_PARAMETERS_TEXT, this);

  refreshPanel();
}

CanopyEditorWindow::~CanopyEditorWindow() { delete parametersTextEditor; }

bool CanopyEditorWindow::canSelectAsset(const DagorAsset *next_asset)
{
  const char *nextAssetName = next_asset ? next_asset->getName() : "";
  if (strcmp(currentAssetName, nextAssetName) == 0)
    return true;

  return resolveUnsavedChanges();
}

bool CanopyEditorWindow::canClose() { return resolveUnsavedChanges(); }

void CanopyEditorWindow::onAssetSelectionChanged(const DagorAsset *asset)
{
  currentAssetName = asset ? asset->getName() : "";
  updateCurrentAssetParameters();
}

void CanopyEditorWindow::setSelectedBlkPath(const char *path)
{
  selectedBlkPath = path ? path : "";
  reloadSelectedBlk();
  updateCurrentAssetParameters();
}

int CanopyEditorWindow::saveState(DataBlock &datablk, bool by_name)
{
  refreshPanel(false);
  const int result = PanelWindowPropertyControl::saveState(datablk, by_name);
  datablk.setStr("selectedBlkPath", selectedBlkPath);
  return result;
}

int CanopyEditorWindow::loadState(DataBlock &datablk, bool by_name)
{
  const int result = PanelWindowPropertyControl::loadState(datablk, by_name);
  selectedBlkPath = datablk.getStr("selectedBlkPath", "");
  setText(ID_CANOPY_BLK_FILE, selectedBlkPath);
  reloadSelectedBlk();
  updateCurrentAssetParameters();

  return result;
}

const char *CanopyEditorWindow::getViewportFxAssetName() const
{
  return showFx && viewportFxAvailable ? viewportFxAssetName.str() : nullptr;
}

bool CanopyEditorWindow::getViewportCanopyParams(ViewportCanopyParams &params) const
{
  params = ViewportCanopyParams();

  float canopyTopOffset = 0.1f;
  float canopyTopPart = 0.4f;
  float canopyWidthPart = 0.3f;
  float canopyOpacity = 0.0f;

  const int canopyType = get_top_level_canopy_type_value(editableDmgBlock);
  params.shape = canopyType == CANOPY_TYPE_TRIANGLE ? ViewportCanopyParams::CONE
                 : canopyType == CANOPY_TYPE_SPHERE ? ViewportCanopyParams::SPHEROID
                                                    : ViewportCanopyParams::BOX;
  params.topOffset = try_get_top_level_real(editableDmgBlock, "canopyTopOffset", canopyTopOffset) ? canopyTopOffset : 0.1f;
  params.topPart = try_get_top_level_real(editableDmgBlock, "canopyTopPart", canopyTopPart)
                     ? canopyTopPart
                     : (params.shape == ViewportCanopyParams::CONE ? 0.75f : 0.4f);
  params.widthPart = try_get_top_level_real(editableDmgBlock, "canopyWidthPart", canopyWidthPart) ? canopyWidthPart : 0.3f;

  const bool hasCanopyTopPart = find_top_level_param(editableDmgBlock, "canopyTopPart") >= 0;
  const bool hasCanopyTopOffset = find_top_level_param(editableDmgBlock, "canopyTopOffset") >= 0;
  const bool hasCanopyWidthPart = find_top_level_param(editableDmgBlock, "canopyWidthPart") >= 0;
  const bool hasCanopyOpacity = try_get_top_level_real(editableDmgBlock, "canopyOpacity", canopyOpacity);
  const bool hasCanopyDefinition =
    canopyType != CANOPY_TYPE_BOX || hasCanopyTopPart || hasCanopyTopOffset || hasCanopyWidthPart || hasCanopyOpacity;

  params.opacity = hasCanopyOpacity ? canopyOpacity : (hasCanopyDefinition ? 1.0f : 0.0f);
  params.valid = params.opacity > 0.0f;
  return params.valid;
}

void CanopyEditorWindow::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (updatingControls)
    return;

  if (pcb_id == ID_CANOPY_BLK_FILE)
  {
    if (!resolveUnsavedChanges())
    {
      refreshPanel(false);
      return;
    }

    const SimpleString newPathText = panel->getText(ID_CANOPY_BLK_FILE);
    String newPath(newPathText.str());
    if (!newPath.empty())
      newPath = ::make_path_relative(newPath, get_app().getWorkspace().getAppDir());

    if (selectedBlkPath == newPath)
      return;

    setSelectedBlkPath(newPath);
    return;
  }

  if (pcb_id == ID_CANOPY_SHOW_FX)
  {
    showFx = panel->getBool(ID_CANOPY_SHOW_FX) && viewportFxAvailable;
    invalidate_viewport_cache();
    return;
  }

  if ((pcb_id == ID_CANOPY_CANOPY_TYPE || pcb_id >= ID_CANOPY_VALUE_BASE) && flushPendingEditorTextBeforeVisualChange())
    return;

  if (pcb_id == ID_CANOPY_CANOPY_TYPE)
  {
    if (!set_top_level_canopy_type_value(editableDmgBlock, panel->getInt(ID_CANOPY_CANOPY_TYPE)))
      return;

    syncTextFromRecognizedParameters();
    rebuildParametersPanel();
    return;
  }

  if (pcb_id < ID_CANOPY_VALUE_BASE)
    return;

  const int valueIndex = pcb_id - ID_CANOPY_VALUE_BASE;
  if (valueIndex < 0 || valueIndex >= valuePaths.size())
    return;

  const ControlPath &path = valuePaths[valueIndex];
  DataBlock *block = getEditableBlock(path.blockPath);
  if (!block || path.itemIndex < 0 || path.itemIndex >= block->paramCount())
    return;

  switch (block->getParamType(path.itemIndex))
  {
    case DataBlock::TYPE_STRING: block->setStr(path.itemIndex, panel->getText(pcb_id)); break;
    case DataBlock::TYPE_INT: block->setInt(path.itemIndex, panel->getInt(pcb_id)); break;
    case DataBlock::TYPE_REAL: block->setReal(path.itemIndex, panel->getFloat(pcb_id)); break;
    case DataBlock::TYPE_POINT2: block->setPoint2(path.itemIndex, panel->getPoint2(pcb_id)); break;
    case DataBlock::TYPE_POINT3: block->setPoint3(path.itemIndex, panel->getPoint3(pcb_id)); break;
    case DataBlock::TYPE_POINT4: block->setPoint4(path.itemIndex, panel->getPoint4(pcb_id)); break;
    case DataBlock::TYPE_IPOINT2:
    {
      const Point2 value = panel->getPoint2(pcb_id);
      block->setIPoint2(path.itemIndex, IPoint2(round_to_int(value.x), round_to_int(value.y)));
      break;
    }
    case DataBlock::TYPE_IPOINT3:
    {
      const Point3 value = panel->getPoint3(pcb_id);
      block->setIPoint3(path.itemIndex, IPoint3(round_to_int(value.x), round_to_int(value.y), round_to_int(value.z)));
      break;
    }
    case DataBlock::TYPE_IPOINT4:
    {
      const Point4 value = panel->getPoint4(pcb_id);
      block->setIPoint4(path.itemIndex,
        IPoint4(round_to_int(value.x), round_to_int(value.y), round_to_int(value.z), round_to_int(value.w)));
      break;
    }
    case DataBlock::TYPE_BOOL: block->setBool(path.itemIndex, panel->getInt(pcb_id) == 0); break;
    case DataBlock::TYPE_E3DCOLOR: block->setE3dcolor(path.itemIndex, panel->getColor(pcb_id)); break;
    case DataBlock::TYPE_MATRIX: block->setTm(path.itemIndex, panel->getMatrix(pcb_id)); break;
    case DataBlock::TYPE_INT64:
    {
      int64_t parsedValue = 0;
      if (!try_parse_int64(panel->getText(pcb_id), parsedValue))
        return;

      block->setInt64(path.itemIndex, parsedValue);
      break;
    }
    default: return;
  }

  syncTextFromRecognizedParameters();
}

void CanopyEditorWindow::onClick(int pcb_id, PropPanel::ContainerPropertyControl *)
{
  if ((pcb_id == ID_CANOPY_ADD_PARAMS || pcb_id >= ID_CANOPY_ACTION_BASE) && flushPendingEditorTextBeforeVisualChange())
    return;

  if (pcb_id == ID_CANOPY_ADD_PARAMS)
  {
    if (!canEditParameters())
      return;

    eastl::vector<AddableParamTemplate> availableTemplates;
    collect_addable_param_templates(loadedBlk, availableTemplates);

    Tab<String> availableParams(tmpmem);
    for (const AddableParamTemplate &item : availableTemplates)
      if (can_add_param_template(editableDmgBlock, item))
        availableParams.push_back(item.displayPath);

    if (availableParams.empty())
      return;

    Tab<String> selectedParams(tmpmem);
    PropPanel::MultiListDialog selectParams("List of parameters", hdpi::_pxScaled(300), hdpi::_pxScaled(400), availableParams,
      selectedParams);
    if (selectParams.showDialog() != PropPanel::DIALOG_ID_OK || selectedParams.empty())
      return;

    eastl::vector<const AddableParamTemplate *> selectedTemplates;
    for (int i = 0; i < selectedParams.size(); ++i)
      for (const AddableParamTemplate &item : availableTemplates)
        if (strcmp(item.displayPath, selectedParams[i]) == 0)
        {
          selectedTemplates.push_back(&item);
          break;
        }

    bool changed = false;
    for (int i = selectedTemplates.size() - 1; i >= 0; --i)
      if (selectedTemplates[i]->blockPath.empty())
        changed = insert_addable_param_template(editableDmgBlock, *selectedTemplates[i]) || changed;

    for (const AddableParamTemplate *item : selectedTemplates)
      if (!item->blockPath.empty())
        changed = insert_addable_param_template(editableDmgBlock, *item) || changed;

    if (!changed)
      return;

    syncTextFromRecognizedParameters();
    rebuildParametersPanel();
    return;
  }

  if (pcb_id < ID_CANOPY_ACTION_BASE)
    return;

  const int actionIndex = pcb_id - ID_CANOPY_ACTION_BASE;
  if (actionIndex < 0 || actionIndex >= actionPaths.size())
    return;

  const ControlPath &path = actionPaths[actionIndex];
  DataBlock *block = getEditableBlock(path.blockPath);
  if (!block || path.itemIndex < 0)
    return;

  if (path.isBlock)
  {
    if (path.itemIndex >= block->blockCount())
      return;
    block->removeBlock(path.itemIndex);
  }
  else
  {
    if (path.itemIndex >= block->paramCount())
      return;
    block->removeParam(path.itemIndex);
  }

  syncTextFromRecognizedParameters();
  rebuildParametersPanel();
}

void CanopyEditorWindow::updateImgui()
{
  const bool canSaveParameters = canEditParameters() && dirty;
  const float footerHeight = get_canopy_save_footer_height();
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
  ImGui::BeginChild("##canopy_editor_scroll_region", ImVec2(0.0f, -footerHeight), false);
  PropPanel::ContainerPropertyControl::updateImgui();
  ImGui::Dummy(ImVec2(0.0f, 0.0f));
  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::Separator();
  if (!canSaveParameters)
    ImGui::BeginDisabled();

  if (ImGui::Button("Save parameters to BLK", ImVec2(-FLT_MIN, 0.0f)))
    saveCurrentParameters();

  if (!canSaveParameters)
    ImGui::EndDisabled();

  if (pendingTextParse && get_time_msec() - lastTextChangeMs >= 500)
    applyEditorText(true);
}

void CanopyEditorWindow::customControlUpdate(int id)
{
  if (id != ID_CANOPY_PARAMETERS_TEXT)
    return;

  parametersTextEditor->SetReadOnlyEnabled(!canEditParameters());
  ImFont *monoFont = imgui_get_mono_font();

  if (!canEditParameters())
    ImGui::BeginDisabled();

  if (monoFont)
    ImGui::PushFont(monoFont, 0.0f);

  const String editorId(0, "##canopy_parameters_text_editor_%d", id);
  parametersTextEditor->Render(editorId, ImGui::IsWindowFocused(), false, ImVec2(-FLT_MIN, (float)hdpi::_pxS(160)), true);
  const bool editorHovered = ImGui::IsItemHovered();

  if (monoFont)
    ImGui::PopFont();

  if (editorHovered && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    ImGui::OpenPopup("canopy_parameters_text_context_menu");

  if (ImGui::BeginPopup("canopy_parameters_text_context_menu"))
  {
    if (ImGui::MenuItem("Copy Text", nullptr, false, parametersTextEditor->AnyCursorHasSelection()))
      parametersTextEditor->Copy();

    const bool canPasteText = canEditParameters() && ImGui::GetClipboardText() != nullptr;
    if (ImGui::MenuItem("Paste Text", nullptr, false, canPasteText))
      parametersTextEditor->Paste();

    ImGui::EndPopup();
  }

  if (!canEditParameters())
    ImGui::EndDisabled();

  const eastl::string widgetText = parametersTextEditor->GetText();
  if (strcmp(textEditorWidgetText, widgetText.c_str()) != 0)
    onTextEditorValueChanged(widgetText.c_str());
}

void CanopyEditorWindow::reloadSelectedBlk()
{
  loadedFileText.clear();
  loadedBlk.reset();
  lineEnding = "\r\n";
  currentDmgBlockPath.clear();

  const String resolvedBlkPath = getResolvedBlkPath();
  if (resolvedBlkPath.empty())
    return;

  if (!read_file_to_string(resolvedBlkPath, loadedFileText))
  {
    wingw::message_box(wingw::MBS_EXCL | wingw::MBS_OK, "Canopy parameters", "Failed to read canopy BLK from '%s'.",
      resolvedBlkPath.str());
    return;
  }

  if (strstr(loadedFileText, "\r\n") == nullptr)
    lineEnding = "\n";

  if (!load_blk_text(resolvedBlkPath, loadedBlk))
  {
    wingw::message_box(wingw::MBS_EXCL | wingw::MBS_OK, "Canopy parameters", "Failed to parse canopy BLK from '%s'.",
      resolvedBlkPath.str());
    loadedFileText.clear();
  }
}

void CanopyEditorWindow::updateCurrentAssetParameters()
{
  currentDmgBlockPath.clear();
  editableDmgBlock.reset();
  savedParametersText.clear();
  editorParametersText.clear();
  viewportFxAssetName.clear();
  viewportFxAvailable = false;
  showFx = false;
  dirty = false;
  pendingTextParse = false;

  String normalizedAssetName;
  if (!loadedFileText.empty() && !currentAssetName.empty())
  {
    normalizedAssetName = normalize_canopy_asset_name(currentAssetName);
    eastl::vector<int> matchedPath;
    if (
      DataBlock *matchedDmgBlock = find_matching_dmg_block(loadedBlk, currentAssetName.str(), normalizedAssetName.str(), matchedPath))
    {
      currentDmgBlockPath = matchedPath;
      editableDmgBlock.setFrom(matchedDmgBlock);
      normalize_top_level_canopy_type_params(editableDmgBlock);
      savedParametersText = build_blk_text(editableDmgBlock, lineEnding);
      editorParametersText = savedParametersText;
    }
    else
    {
      const char *savedAssetName = normalizedAssetName.empty() ? currentAssetName.str() : normalizedAssetName.str();
      if (const DataBlock *templateDmgBlock = find_default_dmg_template_block(loadedBlk, savedAssetName))
        editableDmgBlock.setFrom(templateDmgBlock);
      else
        editableDmgBlock.changeBlockName("dmg");

      set_top_level_name_value(editableDmgBlock, savedAssetName);
      normalize_top_level_canopy_type_params(editableDmgBlock);
      editorParametersText = build_blk_text(editableDmgBlock, lineEnding);
    }
  }

  dirty = strcmp(editorParametersText, savedParametersText) != 0;
  updateViewportFxState();
  refreshPanel();
  invalidate_viewport_cache();
}

void CanopyEditorWindow::refreshPanel(bool rebuild_parameters_panel)
{
  updatingControls = true;
  setText(ID_CANOPY_BLK_FILE, selectedBlkPath);
  setBool(ID_CANOPY_SHOW_FX, showFx);
  updateControlStates();
  updatingControls = false;
  syncTextEditorWidget();

  if (rebuild_parameters_panel)
    rebuildParametersPanel();
}

void CanopyEditorWindow::rebuildParametersPanel()
{
  if (!parametersGroup)
    return;

  actionPaths.clear();
  valuePaths.clear();

  updatingControls = true;
  parametersGroup->clear();

  if (canEditParameters())
  {
    eastl::vector<int> blockPath;
    rebuildParametersPanel(*parametersGroup, editableDmgBlock, blockPath);
  }

  updatingControls = false;
}

void CanopyEditorWindow::rebuildParametersPanel(PropPanel::ContainerPropertyControl &panel, DataBlock &block,
  eastl::vector<int> &block_path)
{
  const bool isTopLevelPanel = block_path.empty();
  bool canopyTypeInserted = false;
  for (int i = 0; i < block.paramCount(); ++i)
  {
    const char *paramName = block.getParamName(i);
    if (is_datablock_comment_name(paramName))
      continue;

    if (isTopLevelPanel && is_canopy_type_param_name(paramName))
      continue;

    actionPaths.push_back(ControlPath{false, i, block_path});
    const int actionId = ID_CANOPY_ACTION_BASE + actionPaths.size() - 1;

    PropPanel::ContainerPropertyControl *row = panel.createExtensible(actionId, true, "delete", "Remove parameter");
    row->setIntValue(1 << PropPanel::EXT_BUTTON_SINGLE_ACTION);

    valuePaths.push_back(ControlPath{false, i, block_path});
    const int valueId = ID_CANOPY_VALUE_BASE + valuePaths.size() - 1;

    switch (block.getParamType(i))
    {
      case DataBlock::TYPE_STRING:
      {
        row->setUseFixedWidthColumnsWithZeroWidthStretch();
        row->setHorizontalSpaceBetweenControls(hdpi::_pxActual(0));

        const int firstChildIndex = row->getChildCount();
        String caption(0, "%s:", paramName);
        row->createStatic(-1, caption, true, true);
        row->createEditBox(valueId, "", block.getStr(i), true, false);

        if (PropPanel::PropertyControlBase *captionControl = row->getByIndex(firstChildIndex))
          captionControl->setWidth(hdpi::_pxActual((int)(ImGui::CalcTextSize(caption).x + 2.0f)));
        break;
      }
      case DataBlock::TYPE_INT: row->createEditInt(valueId, paramName, block.getInt(i)); break;
      case DataBlock::TYPE_REAL: row->createEditFloat(valueId, paramName, block.getReal(i), 3); break;
      case DataBlock::TYPE_POINT2: row->createPoint2(valueId, paramName, block.getPoint2(i), 3); break;
      case DataBlock::TYPE_POINT3: row->createPoint3(valueId, paramName, block.getPoint3(i), 3); break;
      case DataBlock::TYPE_POINT4: row->createPoint4(valueId, paramName, block.getPoint4(i), 3); break;
      case DataBlock::TYPE_IPOINT2:
        row->createPoint2(valueId, paramName, Point2((float)block.getIPoint2(i).x, (float)block.getIPoint2(i).y), 0);
        break;
      case DataBlock::TYPE_IPOINT3:
        row->createPoint3(valueId, paramName,
          Point3((float)block.getIPoint3(i).x, (float)block.getIPoint3(i).y, (float)block.getIPoint3(i).z), 0);
        break;
      case DataBlock::TYPE_IPOINT4:
        row->createPoint4(valueId, paramName,
          Point4((float)block.getIPoint4(i).x, (float)block.getIPoint4(i).y, (float)block.getIPoint4(i).z,
            (float)block.getIPoint4(i).w),
          0);
        break;
      case DataBlock::TYPE_BOOL:
      {
        row->setUseFixedWidthColumnsWithZeroWidthStretch();
        row->setHorizontalSpaceBetweenControls(hdpi::_pxActual(0));

        const int firstChildIndex = row->getChildCount();
        String caption(0, "%s:", paramName);
        row->createStatic(-1, caption, true, true);
        row->createCombo(valueId, "", build_yes_no_values(), block.getBool(i) ? 0 : 1, true, false);

        if (PropPanel::PropertyControlBase *captionControl = row->getByIndex(firstChildIndex))
          captionControl->setWidth(hdpi::_pxActual((int)(ImGui::CalcTextSize(caption).x + 2.0f)));
        break;
      }
      case DataBlock::TYPE_E3DCOLOR: row->createColorBox(valueId, paramName, block.getE3dcolor(i)); break;
      case DataBlock::TYPE_MATRIX: row->createMatrix(valueId, paramName, block.getTm(i), 3); break;
      case DataBlock::TYPE_INT64:
      {
        String valueText(0, "%lld", (long long)block.getInt64(i));
        row->createEditBox(valueId, paramName, valueText);
        break;
      }
      default: break;
    }

    if (isTopLevelPanel && !canopyTypeInserted && strcmp(paramName, "name") == 0)
    {
      create_canopy_type_row(panel, get_top_level_canopy_type_value(editableDmgBlock));
      canopyTypeInserted = true;
    }
  }

  for (int i = 0; i < block.blockCount(); ++i)
  {
    DataBlock *child = block.getBlock(i);
    if (!child)
      continue;
    if (is_datablock_comment_name(child->getBlockName()))
      continue;

    actionPaths.push_back(ControlPath{true, i, block_path});
    const int actionId = ID_CANOPY_ACTION_BASE + actionPaths.size() - 1;
    PropPanel::ContainerPropertyControl *group = panel.createExtGroup(actionId, child->getBlockName(), "delete", "Remove block");
    group->setIntValue(1 << PropPanel::EXT_BUTTON_SINGLE_ACTION);
    group->setBoolValue(false);
    block_path.push_back(i);
    rebuildParametersPanel(*group, *child, block_path);
    block_path.pop_back();
  }

  if (isTopLevelPanel && !canopyTypeInserted)
    create_canopy_type_row(panel, get_top_level_canopy_type_value(editableDmgBlock));
}

void CanopyEditorWindow::applyEditorText(bool rebuild_parameters_panel)
{
  pendingTextParse = false;

  const String resolvedBlkPath = getResolvedBlkPath();
  const char *sourceName = resolvedBlkPath.empty() ? "<canopy_editor>" : resolvedBlkPath.str();

  DataBlock parsedBlock;
  if (!load_blk_text_from_string(editorParametersText, editorParametersText.length(), sourceName, parsedBlock))
    return;

  editableDmgBlock.setFrom(&parsedBlock);
  const bool canopyTypeNormalized = normalize_top_level_canopy_type_params(editableDmgBlock);
  if (canopyTypeNormalized)
  {
    editorParametersText = build_blk_text(editableDmgBlock, lineEnding);
    syncTextEditorWidget();
  }

  updateViewportFxState();

  if (rebuild_parameters_panel)
    rebuildParametersPanel();

  updatingControls = true;
  setInt(ID_CANOPY_CANOPY_TYPE, get_top_level_canopy_type_value(editableDmgBlock));
  setBool(ID_CANOPY_SHOW_FX, showFx);
  updatingControls = false;

  updateDirtyState();
  invalidate_viewport_cache();
}

bool CanopyEditorWindow::flushPendingEditorTextBeforeVisualChange()
{
  if (!pendingTextParse)
    return false;

  applyEditorText(true);
  return true;
}

void CanopyEditorWindow::updateControlStates()
{
  const bool canEdit = canEditParameters();
  const bool canEditVisual = canEdit && !pendingTextParse;
  setEnabledById(ID_CANOPY_SHOW_FX, viewportFxAvailable && canEditVisual);
  setEnabledById(ID_CANOPY_ADD_PARAMS, canEditVisual && has_available_addable_param_templates(loadedBlk, editableDmgBlock));
  setEnabledById(ID_CANOPY_PARAMETERS_GROUP, canEditVisual);
  setEnabledById(ID_CANOPY_PARAMETERS_TEXT_GROUP, canEdit);
}

void CanopyEditorWindow::syncTextFromRecognizedParameters()
{
  pendingTextParse = false;
  normalize_top_level_canopy_type_params(editableDmgBlock);
  editorParametersText = build_blk_text(editableDmgBlock, lineEnding);
  syncTextEditorWidget();
  updateViewportFxState();

  updatingControls = true;
  setInt(ID_CANOPY_CANOPY_TYPE, get_top_level_canopy_type_value(editableDmgBlock));
  setBool(ID_CANOPY_SHOW_FX, showFx);
  updatingControls = false;

  updateDirtyState();
  invalidate_viewport_cache();
}

void CanopyEditorWindow::updateViewportFxState()
{
  const bool hadViewportFxAvailable = viewportFxAvailable;
  const String previousViewportFxAssetName = viewportFxAssetName;

  viewportFxAssetName.clear();
  viewportFxAvailable = false;

  String fxName;
  if (!try_get_top_level_string(editableDmgBlock, "fx", fxName))
  {
    showFx = false;
    return;
  }

  trim(fxName);
  if (fxName.empty())
  {
    showFx = false;
    return;
  }

  if (!resolve_canopy_fx_asset(fxName))
  {
    showFx = false;
    return;
  }

  viewportFxAssetName = fxName;
  viewportFxAvailable = true;
  if (!hadViewportFxAvailable || previousViewportFxAssetName != viewportFxAssetName)
    showFx = true;
}

void CanopyEditorWindow::discardUnsavedChanges()
{
  editorParametersText = savedParametersText;
  dirty = false;
  pendingTextParse = false;
  applyEditorText(true);
  refreshPanel(false);
}

bool CanopyEditorWindow::saveCurrentParameters()
{
  if (pendingTextParse)
    applyEditorText(true);

  const String resolvedBlkPath = getResolvedBlkPath();
  if (resolvedBlkPath.empty())
    return false;

  DataBlock itemsToSave;
  itemsToSave.setFrom(&editableDmgBlock);
  normalize_top_level_canopy_type_params(itemsToSave);
  const String normalizedAssetName = normalize_canopy_asset_name(currentAssetName);
  const char *savedAssetName = normalizedAssetName.empty() ? currentAssetName.str() : normalizedAssetName.str();
  set_top_level_name_value(itemsToSave, savedAssetName);

  DataBlock *currentBlock = currentDmgBlockPath.empty() ? nullptr : get_block_by_path(loadedBlk, currentDmgBlockPath);
  if (currentBlock)
  {
    currentBlock->setFrom(&itemsToSave);
    currentBlock->changeBlockName("dmg");
  }
  else
  {
    const int lastDmgIndex = find_last_top_level_dmg_index(loadedBlk);
    loadedBlk.addNewBlock(&itemsToSave, "dmg");
    for (int blockIndex = loadedBlk.blockCount() - 1; lastDmgIndex >= 0 && blockIndex > lastDmgIndex + 1; --blockIndex)
      loadedBlk.swapBlocks(blockIndex, blockIndex - 1);
  }

  const String updatedFileText = build_blk_text(loadedBlk, lineEnding);
  if (!write_string_to_file(resolvedBlkPath, updatedFileText))
  {
    wingw::message_box(wingw::MBS_EXCL | wingw::MBS_OK, "Canopy parameters", "Failed to write canopy parameters to '%s'.",
      resolvedBlkPath.str());
    return false;
  }

  reloadSelectedBlk();
  updateCurrentAssetParameters();
  dirty = false;
  pendingTextParse = false;
  refreshPanel();
  return true;
}

bool CanopyEditorWindow::resolveUnsavedChanges()
{
  if (!dirty)
    return true;

  const int dialogResult = wingw::message_box(wingw::MBS_QUEST | wingw::MBS_YESNOCANCEL, "Canopy parameters",
    "You have changed canopy parameters. Do you want to save the changes to the BLK file?");
  if (dialogResult == wingw::MB_ID_YES)
    return saveCurrentParameters();
  if (dialogResult == wingw::MB_ID_NO)
  {
    discardUnsavedChanges();
    return true;
  }

  return false;
}

namespace
{
class CanopyEditorWindowController final : public IModelessWindowController
{
public:
  const char *getWindowId() const override { return "canopy_editor"; }

  void releaseWindow() override
  {
    save_current_canopy_editor_window_state();
    if (get_app().getCanopyEditorWindow())
      get_app().showCanopyEditor(false);
  }

  void showWindow(bool show = true) override
  {
    if (show)
    {
      if (!get_app().getCanopyEditorWindow())
        get_app().showCanopyEditor(true);

      if (CanopyEditorWindow *window = get_app().getCanopyEditorWindow())
        load_canopy_editor_window_state(*window);
    }
    else if (get_app().getCanopyEditorWindow())
    {
      save_current_canopy_editor_window_state();
      get_app().showCanopyEditor(false);
    }
  }

  bool isWindowShown() const override { return get_app().getCanopyEditorWindow() != nullptr; }
};

static CanopyEditorWindowController canopy_editor_window_controller;
} // namespace

IModelessWindowController *get_canopy_editor_window_controller() { return &canopy_editor_window_controller; }

void load_canopy_editor_settings(const DataBlock &blk)
{
  canopy_ui_state.clearData();
  if (const DataBlock *uiState = blk.getBlockByName("canopyEditorUiState"))
    canopy_ui_state.setFrom(uiState);

  if (CanopyEditorWindow *window = get_app().getCanopyEditorWindow())
    window->loadState(canopy_ui_state, /*by_name = */ true);
}

void save_canopy_editor_settings(DataBlock &blk)
{
  save_current_canopy_editor_window_state();
  blk.addNewBlock(&canopy_ui_state, "canopyEditorUiState");
}

void load_canopy_editor_window_state(CanopyEditorWindow &window) { window.loadState(canopy_ui_state, /*by_name = */ true); }

void save_current_canopy_editor_window_state()
{
  if (CanopyEditorWindow *window = get_app().getCanopyEditorWindow())
  {
    canopy_ui_state.clearData();
    window->saveState(canopy_ui_state, /*by_name = */ true);
  }
}

bool CanopyEditorWindow::canEditParameters() const { return !loadedFileText.empty() && !currentAssetName.empty(); }

void CanopyEditorWindow::updateDirtyState()
{
  dirty = strcmp(editorParametersText, savedParametersText) != 0;
  updateControlStates();
}

void CanopyEditorWindow::syncTextEditorWidget()
{
  const String normalizedText = normalize_line_endings(editorParametersText, "\n");
  if (strcmp(textEditorWidgetText, normalizedText) == 0)
    return;

  textEditorWidgetText = normalizedText;
  parametersTextEditor->SetText(textEditorWidgetText.str());
}

void CanopyEditorWindow::onTextEditorValueChanged(const char *text)
{
  textEditorWidgetText = text ? text : "";
  editorParametersText = normalize_line_endings(textEditorWidgetText, lineEnding);
  pendingTextParse = true;
  lastTextChangeMs = get_time_msec();
  updateDirtyState();
}

DataBlock *CanopyEditorWindow::getEditableBlock(const eastl::vector<int> &path) { return get_block_by_path(editableDmgBlock, path); }

const DataBlock *CanopyEditorWindow::getEditableBlock(const eastl::vector<int> &path) const
{
  return get_block_by_path(editableDmgBlock, path);
}

String CanopyEditorWindow::getResolvedBlkPath() const
{
  if (selectedBlkPath.empty())
    return String();

  if (selectedBlkPath.length() > 1 && selectedBlkPath[1] == ':')
    return selectedBlkPath;

  String resolvedPath(0, "%s%s", get_app().getWorkspace().getAppDir(), selectedBlkPath.str());
  if (!dd_file_exists(resolvedPath) && dd_file_exists(selectedBlkPath))
    return selectedBlkPath;

  return resolvedPath;
}
