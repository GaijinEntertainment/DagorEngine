//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// remove macroses wich cause nameclash
#ifdef ERROR
#undef ERROR
#endif
#include <util/dag_safeArg.h>

/// Interface class for message logging management.
/// @ingroup EditorCore
/// @ingroup Log
class ILogWriter
{
public:
  /// Messages types.
  enum MessageType
  {
    NOTE,    ///< Note message
    WARNING, ///< Warning message
    ERROR,   ///< Error message
    FATAL,   ///< Fatal error message
    REMARK,  ///< Remark (low priority) message
  };

  /// Add formatted message to log.
  /// @param[in] fmt - printf-like format string (dagor format extension)
  /// @param[in] type - message type (see enum MessageType)
  /// @param[in] arg, anum - type-safe parameters to be used by format
  virtual void addMessageFmt(MessageType type, const char *fmt, const DagorSafeArg *arg, int anum) = 0;

  /// returns true when log contains ERROR/FATAL messages
  virtual bool hasErrors() const = 0;

  /// starts log messages counting
  virtual void startLog() = 0;

  /// finishes log messages counting
  virtual void endLog() = 0;

/// void addMessage(MessageType type, const char *fmt, typesafe-var-args...)
#define DSA_OVERLOADS_PARAM_DECL MessageType type,
#define DSA_OVERLOADS_PARAM_PASS type,
  DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void addMessage, addMessageFmt);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS
};
