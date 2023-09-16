#include <datacache/datacache.h>
#include "file/filebackend.h"
#include "web/webbackend.h"
#include "common/util.h"

namespace datacache
{

Backend *create(BackendType bt, const DataBlock &blk)
{
  if (bt == BT_FILE)
    return create(FileBackendConfig(blk));
  else if (bt == BT_WEB)
  {
    WebBackendConfig config(blk);
    return create(config);
  }
  return nullptr;
}

Backend *create(FileBackendConfig const &config) { return FileBackend::create(config); }

Backend *create(WebBackendConfig &config) { return WebBackend::create(config); }

}; // namespace datacache
