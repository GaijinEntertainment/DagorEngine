//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_safeArg.h>
class DagorAsset;


// generic message pipe interface used in asset management
class IDagorAssetMsgPipe
{
public:
  enum
  {
    NOTE,
    WARNING,
    ERROR,
    FATAL,
    REMARK
  };

  virtual void onAssetMgrMessage(int msg_type, const char *msg, DagorAsset *asset, const char *asset_src_fpath) = 0;
  virtual int getErrorCount() = 0;
  virtual void resetErrorCount() = 0;
};


class NullMsgPipe : public IDagorAssetMsgPipe
{
public:
  void onAssetMgrMessage(int msg_type, const char *, DagorAsset *, const char *) override { updateErrCount(msg_type); }

  int getErrorCount() override { return errCount; }
  void resetErrorCount() override { errCount = 0; }

protected:
  int errCount;

  void updateErrCount(int msg_type)
  {
    if (msg_type == ERROR || msg_type == FATAL)
      errCount++;
  }
};

class SimpleDebugMsgPipe : public NullMsgPipe
{
public:
  void onAssetMgrMessage(int msg_type, const char *msg, DagorAsset *asset, const char *asset_src_fpath) override;
};


// helper routines for message composing
#define DSA_OVERLOADS_PARAM_DECL IDagorAssetMsgPipe &p,
#define DSA_OVERLOADS_PARAM_PASS p,
DECLARE_DSA_OVERLOADS_FAMILY(static inline void post_error, void post_error, post_error);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS

#define DSA_OVERLOADS_PARAM_DECL IDagorAssetMsgPipe &p, DagorAsset *asset,
#define DSA_OVERLOADS_PARAM_PASS p, asset,
DECLARE_DSA_OVERLOADS_FAMILY(static inline void post_error_a, void post_error_a, post_error_a);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS

#define DSA_OVERLOADS_PARAM_DECL IDagorAssetMsgPipe &p, const char *asset_folder_path, const char *asset_fname,
#define DSA_OVERLOADS_PARAM_PASS p, asset_folder_path, asset_fname,
DECLARE_DSA_OVERLOADS_FAMILY(static inline void post_error_f, void post_error_f, post_error_f);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS


#define DSA_OVERLOADS_PARAM_DECL IDagorAssetMsgPipe &p, int msg_type,
#define DSA_OVERLOADS_PARAM_PASS p, msg_type,
DECLARE_DSA_OVERLOADS_FAMILY(static inline void post_msg, void post_msg, post_msg);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS

#define DSA_OVERLOADS_PARAM_DECL IDagorAssetMsgPipe &p, int msg_type, DagorAsset *asset,
#define DSA_OVERLOADS_PARAM_PASS p, msg_type, asset,
DECLARE_DSA_OVERLOADS_FAMILY(static inline void post_msg_a, void post_msg_a, post_msg_a);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS
