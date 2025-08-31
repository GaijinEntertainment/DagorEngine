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

// Interface for data export operations
class IExporter
{
public:
  virtual ~IExporter() = default;

  virtual bool exportData(ConstProfilerStringSpan items) = 0;

  virtual const char *getFormatName() const = 0;
};

// CSV file exporter implementation
class CSVExporter : public IExporter
{
public:
  CSVExporter(TextureModule *texture_module);
  virtual ~CSVExporter() = default;

  bool exportData(ConstProfilerStringSpan items) override;
  const char *getFormatName() const override { return "CSV File"; }

private:
  TextureModule *textureModule = nullptr; // For texture data access
};

// Markdown table exporter implementation
class MarkdownExporter : public IExporter
{
public:
  MarkdownExporter(TextureModule *texture_module);
  virtual ~MarkdownExporter() = default;

  bool exportData(ConstProfilerStringSpan items) override;
  const char *getFormatName() const override { return "Markdown Table"; }

private:
  TextureModule *textureModule = nullptr; // For texture data access
};

// Manages exporters for different formats
class ProfilerExporter
{
public:
  ProfilerExporter(TextureModule *texture_module);
  ~ProfilerExporter();

  template <typename Container>
  bool exportData(const Container &items)
  {
    int index = getExporterIndex();
    if (index < 0 || static_cast<size_t>(index) >= exporters.size())
    {
      toast.show("Export error: Invalid format", DEFAULT_TOAST_DURATION);
      return false;
    }

    bool success = false;
    if (exporters[index])
      success = exporters[index]->exportData(ConstProfilerStringSpan(items));

    toast.show(success ? "Export successful" : "Export error", DEFAULT_TOAST_DURATION);
    return success;
  }

  void drawExportButton();
  void drawExportMenu();

  void setFormat(ExportFormat format);
  ExportFormat getFormat() const { return currentFormat; }

  ToastNotification &getToast() { return toast; }

private:
  TextureModule *textureModule = nullptr;
  ExportFormat currentFormat = ExportFormat::CSV_File;
  eastl::vector<eastl::unique_ptr<IExporter>> exporters;
  ToastNotification toast;

  int getExporterIndex() const;
};

} // namespace levelprofiler