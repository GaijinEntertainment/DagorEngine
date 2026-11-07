// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/unique_ptr.h>
#include "levelProfilerInterface.h"

namespace levelprofiler
{

static constexpr float DEFAULT_TOAST_DURATION = 1.5f;

enum class CopyType
{
  CONTEXT_CELL,
  CONTEXT_ROW,
  CONTEXT_CUSTOM
};

struct CopyRequest
{
  CopyType type;
  ProfilerString customData;

  CopyRequest(CopyType copy_type) : type(copy_type) {}
};

struct CopyResult
{
  ProfilerString text;
  ProfilerString notificationMessage;
  bool success = false;

  CopyResult() = default;
  CopyResult(const ProfilerString &copy_text, const ProfilerString &notification = "Copied to clipboard") :
    text(copy_text), notificationMessage(notification), success(true)
  {}
};

class ICopyProvider
{
public:
  virtual ~ICopyProvider() = default;

  // For global hotkey copy (Ctrl+C)
  virtual CopyResult handleGlobalCopy() const = 0;

  virtual CopyResult handleContextCopy(const CopyRequest &request) const = 0;
};

class INotificationHandler
{
public:
  virtual ~INotificationHandler() = default;
  virtual void showNotification(const char *text) = 0;
};

// Provides non-intrusive feedback to the user
class ToastNotification : public INotificationHandler
{
public:
  void showNotification(const char *text) override;

  void update();

  void draw();

private:
  bool isVisible = false;
  float timer = 0.0f;      // Time remaining for the current notification
  ProfilerString msg = ""; // Message to be displayed
};

class GlobalCopyManager
{
private:
  ICopyProvider *activeProvider = nullptr;
  INotificationHandler &notificationHandler;

public:
  explicit GlobalCopyManager(INotificationHandler &notification_handler) : notificationHandler(notification_handler) {}

  void setActiveProvider(ICopyProvider *provider);

  void update();

  bool executeContextCopy(const CopyRequest &request);

private:
  void handleGlobalCopy();
  void executeCopy(const CopyResult &result);
};

struct LpExportColumn
{
  int id = -1;
  ProfilerString header;
};

struct LpExportSnapshot
{
  eastl::vector<LpExportColumn> columns;
  eastl::vector<size_t> rowIndices;
  bool isValid() const { return !columns.empty() && !rowIndices.empty(); }
};

class ILpTableExportSource
{
public:
  virtual ~ILpTableExportSource() = default;
  // Brings the table rows up to date, then lists the exported columns and rows.
  virtual void buildSnapshot(LpExportSnapshot &out) = 0;
  // filtered_row_index is an entry of the last buildSnapshot rowIndices; valid until the table is drawn or refreshed again.
  virtual void getCell(int column_id, size_t filtered_row_index, ProfilerString &out_value) const = 0;
  virtual const char *getExportTableName() const = 0;
};

enum class LpExportTargetType
{
  File,
  Clipboard
};

struct LpExportTarget
{
  LpExportTargetType type = LpExportTargetType::Clipboard;
  const char *filePath = nullptr;
};

struct LpExportOptions
{
  bool includeHeader = true;
};

class ILpExportFormatter
{
public:
  virtual ~ILpExportFormatter() = default;
  virtual bool begin(const LpExportSnapshot &snap, const ILpTableExportSource &src, const LpExportTarget &target) = 0;
  virtual void writeHeader(const LpExportSnapshot &snap) = 0;
  virtual void writeRow(const eastl::vector<ProfilerString> &cells) = 0;
  virtual bool end() = 0;
  virtual const char *formatName() const = 0;
};

eastl::unique_ptr<ILpExportFormatter> createExportFormatter(ExportFormat fmt);

class LpTableExportEngine
{
public:
  bool exportTable(ILpTableExportSource &source, ExportFormat format, const LpExportTarget &target, const LpExportOptions &opts,
    ProfilerString &errorMsg);
};

class ProfilerExporter
{
public:
  explicit ProfilerExporter(INotificationHandler &notification_handler) : notificationHandler(notification_handler) {}

  void drawExportButton(const ProfilerTab &tab);
  void drawExportMenu();

private:
  ExportFormat currentFormat = ExportFormat::CSV_File;
  INotificationHandler &notificationHandler;
};

} // namespace levelprofiler