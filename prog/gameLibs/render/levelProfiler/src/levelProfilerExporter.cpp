// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <fstream>
#include <sstream>
#include <EASTL/unique_ptr.h>
#include <EASTL/string.h>
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include "levelProfilerUI.h"
#include "riModule.h"
#include "levelProfilerExporter.h"
#include "levelProfilerTextureTable.h"
#include "levelProfilerRiTable.h"
#include <riGen/riGenExtra.h>

namespace levelprofiler
{

// Toast notification layout
static constexpr float TOAST_PADDING_X = 10.0f;
static constexpr float TOAST_PADDING_Y = 6.0f;
static constexpr float TOAST_CORNER_RADIUS = 4.0f;
static constexpr int TOAST_BG_COLOR = IM_COL32(0, 0, 0, 200);
static constexpr int TOAST_TEXT_COLOR = IM_COL32(255, 255, 255, 255);

// Export formatting constants
static constexpr int STYLE_VAR_COUNT = 2;        // Number of style vars to pop
static constexpr size_t ENTRY_RESERVE_SIZE = 64; // Conservative estimate for entry name + ", " + "(count)"

// --- ToastNotification implementation ---

void ToastNotification::show(const char *message, float duration_seconds)
{
  this->msg = message;
  timer = duration_seconds;
  isVisible = true;
}

void ToastNotification::update()
{
  if (isVisible)
    if ((timer -= ImGui::GetIO().DeltaTime) <= 0.0f)
      isVisible = false;
}

void ToastNotification::draw()
{
  if (!isVisible)
    return;

  ImVec2 pos = ImGui::GetMousePos();
  ImDrawList *drawList = ImGui::GetForegroundDrawList();
  ImVec2 textSize = ImGui::CalcTextSize(msg.c_str());
  ImVec2 padding(TOAST_PADDING_X, TOAST_PADDING_Y);
  ImVec2 bgMin = pos;
  ImVec2 bgMax(pos.x + textSize.x + padding.x * 2, pos.y + textSize.y + padding.y * 2);

  drawList->AddRectFilled(bgMin, bgMax, TOAST_BG_COLOR, TOAST_CORNER_RADIUS);

  drawList->AddText(ImVec2(pos.x + padding.x, pos.y + padding.y), TOAST_TEXT_COLOR, msg.c_str());
}

// --- GlobalCopyManager implementation ---

void GlobalCopyManager::setNotificationHandler(INotificationHandler *handler) { notificationHandler = handler; }

void GlobalCopyManager::registerProvider(ICopyProvider *provider, const char *tab_name)
{
  if (!provider)
    return;

  unregisterProvider(provider);
  providers.push_back({provider, ProfilerString(tab_name ? tab_name : "")});

  if (!activeProvider)
    activeProvider = provider;
}

void GlobalCopyManager::unregisterProvider(ICopyProvider *provider)
{
  if (eastl::erase_if(providers, [provider](const ProviderEntry &entry) { return entry.provider == provider; }) > 0)
  {
    if (activeProvider == provider)
      activeProvider = nullptr;

    if (!activeProvider && !providers.empty())
      activeProvider = providers.front().provider;
  }
}

void GlobalCopyManager::setActiveProvider(ICopyProvider *provider) { activeProvider = provider; }

void GlobalCopyManager::update()
{
  if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_C))
    handleGlobalCopy();
}

bool GlobalCopyManager::executeContextCopy(const CopyRequest &request)
{
  if (!activeProvider)
    return false;

  if (CopyResult result = activeProvider->handleContextCopy(request); result.success)
  {
    executeCopy(result, request);
    activeProvider->onCopyExecuted(request);
    return true;
  }

  return false;
}

eastl::vector<ProfilerString> GlobalCopyManager::getActiveContextMenuItems() const
{
  if (activeProvider)
    return activeProvider->getContextMenuItems();
  return {};
}

void GlobalCopyManager::handleGlobalCopy()
{
  if (activeProvider)
  {
    if (CopyResult result = activeProvider->handleGlobalCopy(); result.success)
      executeCopy(result, CopyRequest(CopyType::GLOBAL_HOTKEY));
  }
}

void GlobalCopyManager::executeCopy(const CopyResult &result, const CopyRequest & /* request */)
{
  ImGui::SetClipboardText(result.text.c_str());

  if (notificationHandler)
    notificationHandler->showCopyNotification(result.notificationMessage.c_str());
}

// Export system

// CSV formatter
class LpCsvFormatter : public ILpExportFormatter
{
public:
  bool begin(const LpExportSnapshot & /*snap*/, const ILpTableExportSource & /*src*/, const LpExportTarget &target) override
  {
    if (target.type != LpExportTargetType::File || !target.filePath)
      return false;
    file.open(target.filePath);
    return file.is_open();
  }

  static eastl::string escapeCsv(const ProfilerString &in)
  {
    eastl::string out;
    out.reserve(in.size() + 4);
    for (char ch : in)
    {
      if (ch == '"')
        out.push_back('"');
      out.push_back(ch);
    }
    return out;
  }

  void writeHeader(const LpExportSnapshot &snap) override
  {
    for (size_t i = 0; i < snap.columns.size(); ++i)
    {
      file << '"' << escapeCsv(snap.columns[i].header).c_str() << '"';
      if (i + 1 < snap.columns.size())
        file << ',';
    }
    file << "\n";
  }

  void writeRow(const eastl::vector<ProfilerString> &cells) override
  {
    for (size_t i = 0; i < cells.size(); ++i)
    {
      file << '"' << escapeCsv(cells[i]).c_str() << '"';
      if (i + 1 < cells.size())
        file << ',';
    }
    file << "\n";
  }

  bool end() override
  {
    if (file.is_open())
      file.close();
    return true;
  }

  const char *formatName() const override { return "CSV"; }

private:
  std::ofstream file;
};

// Markdown formatter
class LpMarkdownFormatter : public ILpExportFormatter
{
public:
  bool begin(const LpExportSnapshot & /*snap*/, const ILpTableExportSource & /*src*/, const LpExportTarget & /*target*/) override
  {
    return true;
  }
  static eastl::string escapeMd(const ProfilerString &in)
  {
    eastl::string out;
    out.reserve(in.size() + 4);
    for (char ch : in)
    {
      if (ch == '|')
      {
        out.push_back('\\');
        out.push_back('|');
      }
      else
        out.push_back(ch);
    }
    return out;
  }
  void writeHeader(const LpExportSnapshot &snap) override
  {
    ss << "|";
    for (const auto &c : snap.columns)
      ss << " " << escapeMd(c.header).c_str() << " |";
    ss << "\n|";
    for (size_t i = 0; i < snap.columns.size(); ++i)
      ss << "---|";
    ss << "\n";
  }
  void writeRow(const eastl::vector<ProfilerString> &cells) override
  {
    ss << "|";
    for (const auto &cell : cells)
      ss << " " << escapeMd(cell).c_str() << " |";
    ss << "\n";
  }
  bool end() override
  {
    ImGui::SetClipboardText(ss.str().c_str());
    return true;
  }
  const char *formatName() const override { return "Markdown"; }

private:
  std::ostringstream ss;
};

eastl::unique_ptr<ILpExportFormatter> createExportFormatter(ExportFormat fmt)
{
  switch (fmt)
  {
    case ExportFormat::CSV_File: return eastl::unique_ptr<ILpExportFormatter>(new LpCsvFormatter());
    case ExportFormat::Markdown_Table: return eastl::unique_ptr<ILpExportFormatter>(new LpMarkdownFormatter());
    default: break;
  }
  return nullptr;
}

bool LpTableExportEngine::exportTable(const ILpTableExportSource &source, ExportFormat format, const LpExportTarget &target,
  const LpExportOptions &opts, ProfilerString &errorMsg)
{
  LpExportSnapshot snap;
  if (!source.buildSnapshot(snap))
  {
    errorMsg = "Snapshot build failed";
    return false;
  }
  if (!snap.isValid())
  {
    errorMsg = "No data";
    return false;
  }
  auto formatter = createExportFormatter(format);
  if (!formatter)
  {
    errorMsg = "Formatter creation failed";
    return false;
  }
  if (!formatter->begin(snap, source, target))
  {
    errorMsg = "Formatter begin failed";
    return false;
  }
  if (opts.includeHeader)
    formatter->writeHeader(snap);

  eastl::vector<ProfilerString> cellValues;
  cellValues.resize(snap.columns.size());
  for (size_t r = 0; r < snap.rowIndices.size(); ++r)
  {
    size_t rowIdx = snap.rowIndices[r];
    for (size_t c = 0; c < snap.columns.size(); ++c)
      source.getCell(snap.columns[c].id, rowIdx, cellValues[c]);
    formatter->writeRow(cellValues);
  }
  if (!formatter->end())
  {
    errorMsg = "Formatter end failed";
    return false;
  }
  return true;
}

// ---------------- Texture table adapter ----------------

class LpTextureTableExportSource : public ILpTableExportSource
{
public:
  explicit LpTextureTableExportSource(const LpTextureTable *tbl) : table(tbl) {}

  bool buildSnapshot(LpExportSnapshot &out) const override;
  void getCell(int column_id, size_t row_index, ProfilerString &out_value) const override;
  const char *getExportTableName() const override { return "Textures"; }

private:
  const LpTextureTable *table = nullptr;
  mutable eastl::vector<ProfilerString> snapshotOrder;
  ProfilerString buildUsageValue(const ProfilerString &texName, const RIModule *riModule) const;
};

bool LpTextureTableExportSource::buildSnapshot(LpExportSnapshot &out) const
{
  if (!table)
    return false;
  TextureProfilerUI *ui = table->getProfilerUI();
  if (!ui)
    return false;
  TextureModule *texModule = ui->getTextureModule();
  if (!texModule)
    return false;
  // Use UI display order (includes current sorting) instead of raw module order
  table->buildDisplayOrderedNames(snapshotOrder);

  for (int i = 0, colCount = table->getColumnsCount(); i < colCount; ++i)
    if (table->isColumnVisible(i))
      if (auto *col = table->getColumn(i))
      {
        LpExportColumn ec;
        ec.id = i;
        ec.header = col->getName();
        out.columns.push_back(ec);
      }
  out.rowIndices.reserve(snapshotOrder.size());
  for (size_t i = 0; i < snapshotOrder.size(); ++i)
    out.rowIndices.push_back(i);
  return true;
}

ProfilerString LpTextureTableExportSource::buildUsageValue(const ProfilerString &texName, const RIModule *riModule) const
{
  if (!riModule)
    return "Non-RI";
  const auto &map = riModule->getTextureToAssetsMap();
  auto it = map.find(texName);
  if (it == map.end() || it->second.empty())
    return "Non-RI";

  const auto &instanceCounts = riModule->getRiInstanceCounts();
  ProfilerString result;
  result.reserve(it->second.size() * ENTRY_RESERVE_SIZE);
  bool first = true;

  for (const auto &assetName : it->second)
  {
    if (!first)
      result += ", ";
    first = false;
    result += assetName;

    auto countIt = instanceCounts.find(assetName);
    int count = (countIt != instanceCounts.end()) ? countIt->second : 0;

    result += "(";
    result += eastl::to_string(count);
    result += ")";
  }
  return result;
}

void LpTextureTableExportSource::getCell(int column_id, size_t row_index, ProfilerString &out_value) const
{
  out_value.clear();
  if (!table)
    return;
  TextureProfilerUI *ui = table->getProfilerUI();
  if (!ui)
    return;
  TextureModule *texModule = ui->getTextureModule();
  if (!texModule)
    return;
  if (row_index >= snapshotOrder.size())
    return;
  const ProfilerString &name = snapshotOrder[row_index];
  const auto &all = texModule->getTextures();
  auto it = all.find(name);
  if (it == all.end())
    return;
  const TextureData &td = it->second;
  RIModule *riModule = ui->getRIModule();

  switch (column_id)
  {
    case *TextureColumn::NAME: out_value = name; break;
    case *TextureColumn::FORMAT: out_value = TextureModule::getFormatName(td.info.cflg); break;
    case *TextureColumn::WIDTH: out_value = eastl::to_string(td.info.w); break;
    case *TextureColumn::HEIGHT: out_value = eastl::to_string(td.info.h); break;
    case *TextureColumn::MIPS: out_value = eastl::to_string(td.info.mipLevels); break;
    case *TextureColumn::MEM_SIZE:
    {
      float sizeMB = texModule->getTextureMemorySize(td);
      out_value = eastl::to_string(sizeMB);
      out_value += " MB";
      break;
    }
    case *TextureColumn::USAGE: out_value = buildUsageValue(name, riModule); break;
    default: break;
  }
}

bool exportTextureTable(const LpTextureTable &table, ExportFormat fmt, bool toClipboard, ProfilerString &errorMsg,
  const char *filenameBaseOverride)
{
  LpTextureTableExportSource src(&table);
  LpTableExportEngine engine;
  LpExportTarget target;
  target.type = toClipboard ? LpExportTargetType::Clipboard : LpExportTargetType::File;
  ProfilerString fileName;
  if (!toClipboard)
  {
    if (filenameBaseOverride && *filenameBaseOverride)
      fileName = filenameBaseOverride;
    else
      fileName = src.getExportTableName();
    fileName += (fmt == ExportFormat::CSV_File) ? ".csv" : ".md";
    target.filePath = fileName.c_str();
  }
  LpExportOptions opts;
  opts.includeHeader = true;
  return engine.exportTable(src, fmt, target, opts, errorMsg);
}

// ---------------- RI table adapter ----------------

class LpRiTableExportSource : public ILpTableExportSource
{
public:
  explicit LpRiTableExportSource(const LpRiTable *tbl) : table(tbl) {}
  bool buildSnapshot(LpExportSnapshot &out) const override;
  void getCell(int column_id, size_t filtered_row_index, ProfilerString &out_value) const override;
  const char *getExportTableName() const override { return "RenderInstances"; }

private:
  const LpRiTable *table = nullptr;
  mutable eastl::vector<int> cachedFilteredOrder;
};

bool LpRiTableExportSource::buildSnapshot(LpExportSnapshot &out) const
{
  if (!table)
    return false;
  int colCount = table->getColumnsCount();
  for (int i = 0; i < colCount; ++i)
  {
    if (!table->isColumnVisible(i))
      continue;
    if (auto *col = table->getColumn(i))
    {
      LpExportColumn ec;
      ec.id = i;
      ec.header = col->getName();
      out.columns.push_back(ec);
    }
  }
  table->buildFilteredIndexOrder(cachedFilteredOrder);
  out.rowIndices.reserve(cachedFilteredOrder.size());
  for (size_t i = 0; i < cachedFilteredOrder.size(); ++i)
    out.rowIndices.push_back(i);
  return true;
}

void LpRiTableExportSource::getCell(int column_id, size_t filtered_row_index, ProfilerString &out_value) const
{
  out_value.clear();
  if (!table)
    return;
  RIModule *riModule = table->getRIModule();
  if (!riModule)
    return;
  const auto &vec = riModule->getRiData();

  if (filtered_row_index >= cachedFilteredOrder.size())
    return;
  int actualDataIndex = cachedFilteredOrder[filtered_row_index];
  if (actualDataIndex < 0 || static_cast<size_t>(actualDataIndex) >= vec.size())
    return;

  const RiData &ri = vec[static_cast<size_t>(actualDataIndex)];

  struct GlobalRiLayout
  {
    int lodCount;
    int dipsBase() const { return static_cast<int>(RiColumn::BASE_COUNT); }
    int trisBase() const { return dipsBase() + lodCount; }
    int distBase() const { return trisBase() + lodCount; }
    int screenBase() const { return distBase() + lodCount; }
    int shadersBase() const { return screenBase() + lodCount; }
  } layout{rendinst::RiExtraPool::MAX_LODS};
  constexpr int HEAVY_SHADERS_START_LOD = 2;

  switch (column_id)
  {
    case *RiColumn::NAME: out_value = ri.name; return;
    case *RiColumn::COUNT_ON_MAP: out_value = eastl::to_string(ri.countOnMap); return;
    case *RiColumn::BSPHERE_RAD: out_value = eastl::to_string(ri.bSphereRadius); return;
    case *RiColumn::BBOX_RAD: out_value = eastl::to_string(ri.bBoxRadius); return;
    case *RiColumn::PHYS_TRIS: out_value = eastl::to_string(ri.collision.physTriangles); return;
    case *RiColumn::TRACE_TRIS: out_value = eastl::to_string(ri.collision.traceTriangles); return;
    default: break;
  }

  const int dipsStart = layout.dipsBase();
  const int trisStart = layout.trisBase();
  const int distStart = layout.distBase();
  const int screenStart = layout.screenBase();
  const int shadersStart = layout.shadersBase();

  auto tryGetLodIndex = [&](int base, int &out_lod_index) -> bool {
    if (column_id < base || column_id >= base + layout.lodCount)
      return false;
    out_lod_index = column_id - base;
    return out_lod_index < static_cast<int>(ri.lods.size());
  };

  int lodIndex = 0;

  if (tryGetLodIndex(dipsStart, lodIndex))
  {
    out_value = eastl::to_string(ri.lods[lodIndex].drawCalls);
    return;
  }

  if (tryGetLodIndex(trisStart, lodIndex))
  {
    out_value = eastl::to_string(ri.lods[lodIndex].totalFaces);
    return;
  }

  if (tryGetLodIndex(distStart, lodIndex))
  {
    out_value = eastl::to_string(ri.lods[lodIndex].lodDistance);
    return;
  }

  if (tryGetLodIndex(screenStart, lodIndex))
  {
    out_value = eastl::to_string(ri.lods[lodIndex].screenPercent);
    return;
  }

  const int shaderCount = (layout.lodCount > HEAVY_SHADERS_START_LOD) ? (layout.lodCount - HEAVY_SHADERS_START_LOD) : 0;
  if (column_id >= shadersStart && column_id < shadersStart + shaderCount)
  {
    const int lod = (column_id - shadersStart) + HEAVY_SHADERS_START_LOD;
    if (lod >= static_cast<int>(ri.lods.size()))
      return;

    const auto &entries = ri.lods[lod].heavyShaders;
    if (entries.empty())
      return;

    out_value.reserve(entries.size() * ENTRY_RESERVE_SIZE);
    bool first = true;
    for (const auto &e : entries)
    {
      if (!first)
        out_value += ", ";
      first = false;
      out_value += e.name;
      if (e.count > 1)
      {
        out_value += "(";
        out_value += eastl::to_string(e.count);
        out_value += ")";
      }
    }
    return;
  }
}

bool exportRiTable(const LpRiTable &table, ExportFormat fmt, bool toClipboard, ProfilerString &errorMsg,
  const char *filenameBaseOverride)
{
  LpRiTableExportSource src(&table);
  LpTableExportEngine engine;
  LpExportTarget target;
  target.type = toClipboard ? LpExportTargetType::Clipboard : LpExportTargetType::File;
  ProfilerString fileName;
  if (!toClipboard)
  {
    if (filenameBaseOverride && *filenameBaseOverride)
      fileName = filenameBaseOverride;
    else
      fileName = src.getExportTableName();
    fileName += (fmt == ExportFormat::CSV_File) ? ".csv" : ".md";
    target.filePath = fileName.c_str();
  }
  LpExportOptions opts;
  opts.includeHeader = true;
  return engine.exportTable(src, fmt, target, opts, errorMsg);
}

void ProfilerExporter::drawExportButton()
{
  if (ImGui::Button("Export Data"))
  {
    const LpTextureTable *texTbl = textureTable;
    const LpRiTable *riTbl = riTable;
    if (texTbl || riTbl)
    {
      bool toClipboard = (currentFormat == ExportFormat::Markdown_Table);
      bool ok = true;
      if (texTbl)
        ok = exportTextureTableView(texTbl, toClipboard);
      else if (riTbl)
        ok = exportRiTableView(riTbl, toClipboard);
      if (!ok)
      {
        const char *msg = lastError.empty() ? "Export error" : lastError.c_str();
        if (notificationHandler)
          notificationHandler->showNotification(msg);
        else
          toast.show(msg);
      }
      else
      {
        const char *msg = toClipboard ? "Markdown copied" : "CSV saved";
        if (notificationHandler)
          notificationHandler->showNotification(msg);
        else
          toast.show(msg);
      }
    }
    else
    {
      const char *msg = "No table";
      if (notificationHandler)
        notificationHandler->showNotification(msg);
      else
        toast.show(msg);
    }
  }

  ImGui::SameLine(0, 0);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
  if (ImGui::ArrowButton("##ExportOptions", ImGuiDir_Down))
    ImGui::OpenPopup("ExportOptionsPopup");
  ImGui::PopStyleVar(STYLE_VAR_COUNT);
}

void ProfilerExporter::drawExportMenu()
{
  if (ImGui::BeginPopup("ExportOptionsPopup"))
  {
    if (ImGui::Selectable("CSV File", currentFormat == ExportFormat::CSV_File))
      currentFormat = ExportFormat::CSV_File;
    if (ImGui::Selectable("Markdown Table", currentFormat == ExportFormat::Markdown_Table))
      currentFormat = ExportFormat::Markdown_Table;
    ImGui::EndPopup();
  }
}

} // namespace levelprofiler