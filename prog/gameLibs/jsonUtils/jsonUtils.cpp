#include <ioSys/dag_fileIo.h>
#include <jsonUtils/jsonUtils.h>
#include <jsonUtils/decodeJwt.h>
#include <EASTL/string.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/error/en.h>
#include <rapidJsonUtils/rapidJsonUtils.h>
#include <memory/dag_mem.h>
#include <errno.h>


namespace jsonutils
{

OptionalJsonDocument load_json_from_file(const char *path, eastl::string /*out*/ *err_str)
{
  auto fail = [&](const char *format, auto &&...args) {
    if (err_str)
      err_str->sprintf(format, eastl::forward<decltype(args)>(args)...);
    return eastl::nullopt;
  };

  if (path == nullptr)
    return fail("path=null");

  if (!::dd_file_exist(path))
    return fail("dd_file_exist fail");

  FullFileLoadCB cb(path);
  if (!cb.fileHandle)
    return fail("fileHandle fail");

  int size = ::df_length(cb.fileHandle);
  if (size < 0)
    return fail("df_length=%d fail", size);

  char *buffer = (char *)::memalloc(size, tmpmem);
  int readSize = cb.tryRead(buffer, size);

  if (readSize != size)
  {
    memfree(buffer, tmpmem);
    return fail("tryRead(%d bytes)=%d fail", size, readSize);
  }

  rapidjson::Document doc;
  rapidjson::ParseResult ok = doc.Parse(buffer, size);
  ::memfree(buffer, tmpmem);

  if (!ok)
    return fail("parse fail: %s char pos:%d (size=%d)", rapidjson::GetParseError_En(ok.Code()), ok.Offset(), size);

  return doc;
}


OptionalJsonDocument load_json_from_file(const char *path, const char *who, bool log_on_success)
{
  G_ASSERT(who != nullptr);

  errno = 0;
  eastl::string errorMessage;
  OptionalJsonDocument json = load_json_from_file(path, &errorMessage);
  const bool fail = !json || !errorMessage.empty();

  if (path == nullptr)
    path = "NULL";
  if (fail)
  {
    logwarn("%s: %s(%s) fail: %s (errno=%d)", who, __func__, path, errorMessage.c_str(), int(errno));
  }
  else if (log_on_success)
  {
    debug("%s: %s(%s): type=#%x", who, __func__, path, json->GetType());
  }
  return json;
}

bool save_json_to_file(const rapidjson::Value &json, const char *path, eastl::string /*out*/ *err_str)
{
  auto fail = [&](const char *format, auto &&...args) {
    if (err_str)
      err_str->sprintf(format, eastl::forward<decltype(args)>(args)...);
    return false;
  };

  if (path == nullptr)
    return fail("path = null");

#if _TARGET_C3



#else
  ::dd_mkpath(path);
#endif

  file_ptr_t h = ::df_open(path, DF_WRITE | DF_CREATE);
  if (!h)
    return fail("df_open fail");

  rapidjson::StringBuffer text = jsonutils::stringify<true>(json);
  const int size = text.GetSize();

  LFileGeneralSaveCB cb(h);
  const int writeSize = cb.tryWrite(text.GetString(), size);
  ::df_close(h);

  if (writeSize != size)
    return fail("tryWrite(%d bytes)=%d fail", size, writeSize);

  return true;
}

} // namespace jsonutils
