// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/unique_ptr.h>
#include <EASTL/type_traits.h>
#include "levelProfilerInterface.h"
#include "textureModule.h"
// #include "riModule.h"

namespace levelprofiler
{

static constexpr float DEFAULT_TOAST_DURATION = 1.5f;

template <typename T>
class span
{
public:
  using element_type = T;
  using value_type = eastl::remove_cv_t<T>;
  using size_type = size_t;
  using pointer = T *;
  using const_pointer = const T *;
  using reference = T &;
  using const_reference = const T &;

  constexpr span() noexcept : data_(nullptr), size_(0) {}

  constexpr span(pointer data, size_type size) noexcept : data_(data), size_(size) {}

  template <typename Container>
  constexpr span(Container &container) noexcept : data_(container.data()), size_(container.size())
  {}

  template <typename Container>
  constexpr span(const Container &container) noexcept : data_(container.data()), size_(container.size())
  {}

  constexpr pointer data() const noexcept { return data_; }
  constexpr size_type size() const noexcept { return size_; }
  constexpr bool empty() const noexcept { return size_ == 0; }

  constexpr reference operator[](size_type idx) const noexcept { return data_[idx]; }

  constexpr pointer begin() const noexcept { return data_; }
  constexpr pointer end() const noexcept { return data_ + size_; }

private:
  pointer data_;
  size_type size_;
};

template <typename T>
using const_span = span<const T>;

using ProfilerStringSpan = span<ProfilerString>;
using ConstProfilerStringSpan = span<const ProfilerString>;

enum class CopyType
{
  GLOBAL_HOTKEY, // Ctrl+C global copy
  CONTEXT_CELL,
  CONTEXT_ROW,
  CONTEXT_CUSTOM
};

struct CopyRequest
{
  CopyType type;
  int rowIndex = -1;
  int columnIndex = -1;
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

  virtual eastl::vector<ProfilerString> getContextMenuItems() const = 0;

  virtual void onCopyExecuted(const CopyRequest & /* request */) {}
};

class INotificationHandler
{
public:
  virtual ~INotificationHandler() = default;
  virtual void showCopyNotification(const char *text) = 0;
  virtual void showNotification(const char *text) { showCopyNotification(text); }
};

// Provides non-intrusive feedback to the user
class ToastNotification
{
public:
  void show(const char *message, float duration_seconds = DEFAULT_TOAST_DURATION);

  void update();

  void draw();

private:
  bool isVisible = false;
  float timer = 0.0f;      // Time remaining for the current notification
  ProfilerString msg = ""; // Message to be displayed
};

class ToastNotificationAdapter : public INotificationHandler
{
private:
  ToastNotification *toast;

public:
  ToastNotificationAdapter(ToastNotification *toast_notification) : toast(toast_notification) {}

  void showCopyNotification(const char *text) override
  {
    if (toast)
      toast->show(text, DEFAULT_TOAST_DURATION);
  }

  void showNotification(const char *text) override
  {
    if (toast)
      toast->show(text, DEFAULT_TOAST_DURATION);
  }

  void update()
  {
    if (toast)
      toast->update();
  }
  void draw()
  {
    if (toast)
      toast->draw();
  }
};

class GlobalCopyManager
{
private:
  struct ProviderEntry
  {
    ICopyProvider *provider;
    ProfilerString tabName;
  };

  eastl::vector<ProviderEntry> providers;
  ICopyProvider *activeProvider = nullptr;
  INotificationHandler *notificationHandler = nullptr;

public:
  void setNotificationHandler(INotificationHandler *handler);

  void registerProvider(ICopyProvider *provider, const char *tab_name);

  void unregisterProvider(ICopyProvider *provider);

  void setActiveProvider(ICopyProvider *provider);

  void update();

  bool executeContextCopy(const CopyRequest &request);

  eastl::vector<ProfilerString> getActiveContextMenuItems() const;

private:
  void handleGlobalCopy();
  void executeCopy(const CopyResult &result, const CopyRequest & /* request */);
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
  virtual bool buildSnapshot(LpExportSnapshot &out) const = 0;
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
  bool exportTable(const ILpTableExportSource &source, ExportFormat format, const LpExportTarget &target, const LpExportOptions &opts,
    ProfilerString &errorMsg);
};

class LpTextureTable;
class LpRiTable;
bool exportTextureTable(const LpTextureTable &table, ExportFormat fmt, bool toClipboard, ProfilerString &errorMsg,
  const char *filenameBaseOverride = nullptr);
bool exportRiTable(const LpRiTable &table, ExportFormat fmt, bool toClipboard, ProfilerString &errorMsg,
  const char *filenameBaseOverride = nullptr);

class ProfilerExporter
{
public:
  explicit ProfilerExporter(TextureModule *texture_module) : textureModule(texture_module) {}

  void setFormat(ExportFormat f) { currentFormat = f; }
  ExportFormat getFormat() const { return currentFormat; }

  ToastNotification &getToast() { return toast; }
  void setTextureTable(const LpTextureTable *tbl) { textureTable = tbl; }
  void setRiTable(const LpRiTable *tbl) { riTable = tbl; }
  void setNotificationHandler(INotificationHandler *handler) { notificationHandler = handler; }
  void drawExportButton();
  void drawExportMenu();
  void setFilenameBase(const ProfilerString &base) { filenameBase = base; }
  const ProfilerString &getFilenameBase() const { return filenameBase; }

  bool exportTextureTableView(const LpTextureTable *table, bool toClipboard)
  {
    if (!table)
      return false;
    ProfilerString err;
    const char *fnameBase = filenameBase.empty() ? nullptr : filenameBase.c_str();
    bool ok = exportTextureTable(*table, currentFormat, toClipboard, err, fnameBase);
    lastError = err;
    return ok;
  }

  bool exportRiTableView(const LpRiTable *table, bool toClipboard)
  {
    if (!table)
      return false;
    ProfilerString err;
    bool ok = exportRiTable(*table, currentFormat, toClipboard, err, filenameBase.c_str());
    lastError = err;
    return ok;
  }

  const ProfilerString &getLastError() const { return lastError; }

private:
  TextureModule *textureModule = nullptr;
  ExportFormat currentFormat = ExportFormat::CSV_File;
  ProfilerString lastError;
  ToastNotification toast;
  const LpTextureTable *textureTable = nullptr;
  const LpRiTable *riTable = nullptr;
  INotificationHandler *notificationHandler = nullptr;
  ProfilerString filenameBase;
};

} // namespace levelprofiler