//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_simpleString.h>

#include <libTools/util/iLogWriter.h>


/// Used to output log messages in special dialog window.
/// A color of text messages depends on their types. At the end of window
/// summary info is represented with number of errors and warnings.
/// @ingroup EditorCore
/// @ingroup Log
class CtlReportLogWriter : public ILogWriter
{
public:
  /// Constructor.
  CtlReportLogWriter() : list(tmpmem), errors(false), warnings(false) {}

  /// Add formatted message.
  virtual void addMessageFmt(MessageType type, const char *fmt, const DagorSafeArg *arg, int anum);

  /// Test whether messages list has error messages.
  /// @return @b true if messages list has error messages, @b false in other case
  virtual bool hasErrors() const { return errors; }

  virtual void startLog() {}
  virtual void endLog() {}

  /// Clear messages list.
  /// (for reusing one log writer object for several operations)
  void clearMessages();

  /// Test whether messages list has warning messages.
  /// @return @b true if messages list has warning messages,
  ///         @b false in other case
  inline bool hasWarnings() const { return warnings || errors; }

  /// Test whether messages list has messages.
  /// @return @b true if messages list is not empty, @b false in other case
  inline bool hasMessages() const { return !list.isEmpty(); }

  /// Show dialog window with messages list.
  /// @param[in] title - window title
  /// @param[in] error_msg - message displayed at the end of window if there are
  ///                        error messages in messages list
  /// @param[in] ok_msg - message displayed at the end of window if there are
  ///                     no error messages in messages list
  /// @return @b true if user pressed 'OK' button, @b false if user pressed
  ///                 'Cancel' button
  bool showReport(const char *title, const char *error_msg, const char *ok_msg);

protected:
  Tab<SimpleString> list;
  bool errors, warnings;
};
