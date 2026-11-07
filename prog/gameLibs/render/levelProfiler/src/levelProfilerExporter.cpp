// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <fstream>
#include <sstream>
#include <EASTL/unique_ptr.h>
#include <EASTL/string.h>
#include <imgui/imgui.h>
#include "levelProfilerExporter.h"

namespace levelprofiler
{

// Toast notification layout
static constexpr float TOAST_PADDING_X = 10.0f;
static constexpr float TOAST_PADDING_Y = 6.0f;
static constexpr float TOAST_CORNER_RADIUS = 4.0f;
static constexpr int TOAST_BG_COLOR = IM_COL32(0, 0, 0, 200);
static constexpr int TOAST_TEXT_COLOR = IM_COL32(255, 255, 255, 255);

// Export formatting constants
static constexpr int STYLE_VAR_COUNT = 2; // Number of style vars to pop

// --- ToastNotification implementation ---

void ToastNotification::showNotification(const char *text)
{
  this->msg = text;
  timer = DEFAULT_TOAST_DURATION;
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
    executeCopy(result);
    return true;
  }

  return false;
}

void GlobalCopyManager::handleGlobalCopy()
{
  if (activeProvider)
  {
    if (CopyResult result = activeProvider->handleGlobalCopy(); result.success)
      executeCopy(result);
  }
}

void GlobalCopyManager::executeCopy(const CopyResult &result)
{
  ImGui::SetClipboardText(result.text.c_str());

  notificationHandler.showNotification(result.notificationMessage.c_str());
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

bool LpTableExportEngine::exportTable(ILpTableExportSource &source, ExportFormat format, const LpExportTarget &target,
  const LpExportOptions &opts, ProfilerString &errorMsg)
{
  LpExportSnapshot snap;
  source.buildSnapshot(snap);
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

static bool exportTableView(ILpTableExportSource &src, ExportFormat fmt, const ProfilerString &filenameBase, ProfilerString &errorMsg)
{
  const bool toClipboard = fmt == ExportFormat::Markdown_Table;
  LpExportTarget target;
  target.type = toClipboard ? LpExportTargetType::Clipboard : LpExportTargetType::File;
  ProfilerString fileName;
  if (!toClipboard)
  {
    fileName = filenameBase.empty() ? ProfilerString(src.getExportTableName()) : filenameBase;
    fileName += ".csv";
    target.filePath = fileName.c_str();
  }
  LpTableExportEngine engine;
  return engine.exportTable(src, fmt, target, LpExportOptions(), errorMsg);
}

void ProfilerExporter::drawExportButton(const ProfilerTab &tab)
{
  if (ImGui::Button("Export Data"))
  {
    ProfilerString error;
    const char *msg = nullptr;
    ILpTableExportSource *source = tab.module->getExportSource();
    if (!source)
      msg = "No table";
    else if (exportTableView(*source, currentFormat, tab.name, error))
      msg = currentFormat == ExportFormat::Markdown_Table ? "Markdown copied" : "CSV saved";
    else
      msg = error.c_str();

    notificationHandler.showNotification(msg);
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