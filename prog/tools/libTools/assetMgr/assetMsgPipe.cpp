#include <assets/assetMsgPipe.h>
#include <assets/asset.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>

static String tempBuf;
static String tempBuf2;

void post_error(IDagorAssetMsgPipe &p, const char *fmt, const DagorSafeArg *arg, int anum)
{
  tempBuf.vprintf(4096, fmt, arg, anum);
  p.onAssetMgrMessage(p.ERROR, tempBuf, NULL, NULL);
}
void post_error_a(IDagorAssetMsgPipe &p, DagorAsset *a, const char *fmt, const DagorSafeArg *arg, int anum)
{
  tempBuf.vprintf(4096, fmt, arg, anum);
  p.onAssetMgrMessage(p.ERROR, tempBuf, a, NULL);
}
void post_error_f(IDagorAssetMsgPipe &p, const char *f, const char *a, const char *fmt, const DagorSafeArg *arg, int anum)
{
  tempBuf.vprintf(4096, fmt, arg, anum);
  tempBuf2.printf(260, "%s/%s", f, a);
  p.onAssetMgrMessage(p.ERROR, tempBuf, NULL, tempBuf2);
}

void post_msg(IDagorAssetMsgPipe &p, int msg_type, const char *fmt, const DagorSafeArg *arg, int anum)
{
  tempBuf.vprintf(4096, fmt, arg, anum);
  p.onAssetMgrMessage(msg_type, tempBuf, NULL, NULL);
}
void post_msg_a(IDagorAssetMsgPipe &p, int msg_type, DagorAsset *a, const char *fmt, const DagorSafeArg *arg, int anum)
{
  tempBuf.vprintf(4096, fmt, arg, anum);
  p.onAssetMgrMessage(msg_type, tempBuf, a, NULL);
}

void SimpleDebugMsgPipe::onAssetMgrMessage(int msg_type, const char *msg, DagorAsset *asset, const char *asset_src_fpath)
{
  G_ASSERT(msg_type >= NOTE && msg_type <= REMARK);
  const char *prefix[] = {"*", "WARNING: ", "ERROR: ", "\n-FATAL-: ", ""};

  updateErrCount(msg_type);

  if (asset_src_fpath)
    debug("%s%s (file %s)", prefix[msg_type], msg, asset_src_fpath);
  else if (asset)
    debug("%s%s (file %s)", prefix[msg_type], msg, asset->getSrcFilePath());
  else
    debug("%s%s", prefix[msg_type], msg);
}
