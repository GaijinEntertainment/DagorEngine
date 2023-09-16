#include "files.h"

#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <fstream>
#include <string>
#include <vector>

#include <zip.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <osApiWrappers/dag_direct.h>

#include "configuration.h"
#include "log.h"


namespace breakpad
{

namespace files
{

namespace path
{

std::string normalize(const std::string &path)
{
#if _TARGET_PC_WIN
  const char sfrom = '/';
#else
  const char sfrom = '\\';
#endif

  std::string normalized = path;

  for (std::string::size_type p = normalized.find(sfrom); p != std::string::npos; p = normalized.find(sfrom, p))
    normalized[p] = separator;

  return normalized;
}


std::string directory(const std::string &path)
{
  std::string dir = path;
  dir.erase(dir.rfind(separator));
  return dir;
}


void rename_with_guid(const std::string &origin_, const std::string &guid, const char *ext)
{
  std::string origin = path::normalize(origin_);
  std::string dir = path::directory(origin);
  std::string newname = dir + separator + guid + ext;
  log << "Renaming '" << origin << "' to '" << newname << "' - " << (::dd_rename(origin.c_str(), newname.c_str()) ? "OK" : "FAILED")
      << std::endl;
}


size_t file_size(const std::string &path)
{
#if _TARGET_PC_WIN
#define STAT _stat
#else
#define STAT stat
#endif
  errno = 0;
  struct STAT st = {0};
  if (::STAT(path.c_str(), &st))
    log << "Could not stat " << path << ", errno " << errno << std::endl;

  return st.st_size;
#undef STAT
}

const char *temp_filename = ".report.tmp";
namespace ext
{
const char *archive = ".zip";
const char *dump = ".dmp";
} // namespace ext


} // namespace path


// Will alter cfg with modified paths if any.
void prepare(Configuration &cfg)
{
  if (!cfg.files.count(id::dump))
  {
    log << "Skipping archiving: no dump provided" << std::endl;
    return;
  }

  std::string oldPath = path::normalize(cfg.files[id::dump]);
  std::string dir = oldPath.substr(0, oldPath.rfind(path::separator));
  std::string newPath = dir + path::separator + "crashDump-";

  char timeStr[12];
  struct tm *t = localtime(&cfg.timestamp);
  _snprintf(timeStr, sizeof(timeStr), "%02d.%02d.%02d", t->tm_hour, t->tm_min, t->tm_sec);
  newPath.append(timeStr).append(path::ext::dump);

  if (!::dd_file_exists(oldPath.c_str()))
  {
    log << "Cannot find dump at " << oldPath << std::endl;
    if (::dd_file_exists(cfg.files[id::dump].c_str()))
    {
      log << "Found dump at " << cfg.files[id::dump] << std::endl;
      oldPath = cfg.files[id::dump];
    }
  }

  log << "Renaming '" << oldPath << "' to '" << newPath << "' - ";
  bool res = ::dd_rename(oldPath.c_str(), newPath.c_str());
  log << (res ? "OK" : "FAILED") << std::endl;
  cfg.files[id::dump] = res ? newPath : oldPath; // use normalized path

  log << "Dump file size is " << path::file_size(cfg.files[id::dump]) / 1024 << " KiB" << std::endl;


  if (cfg.sendLogs)
  {
    // We'll skip files on any errors: we can do nothing about them
    // but fallback to uncompressed uploads, hence no error handling
    std::string zipPath = dir + path::separator + "logs-" + timeStr + path::ext::archive;
    if (dd_file_exists(zipPath.c_str()))
      dd_erase(zipPath.c_str());

    log << "Archiving " << cfg.files.size() << " files as " << zipPath << std::endl;
    zip *archive = zip_open(zipPath.c_str(), ZIP_CREATE, NULL);
    size_t filesArchived = 0;

    std::vector<std::string> temp_files;
    temp_files.reserve(cfg.files.size());
    typedef std::map<std::string, std::string>::iterator map_iter;
    for (map_iter it = cfg.files.begin(); it != cfg.files.end(); ++it)
    {
      if (it->first == id::dump)
        continue; // Skip minidump, it's already processed

      std::string src = path::normalize(it->second);
      it->second = src;

      if (!archive) // Can't compress
        continue;

      temp_files.push_back(src + path::temp_filename);
      const std::string &tmp = temp_files.back();

      {
        // We copy the files to avoid race condition between the calling
        // process and sending process. Otherwise we'll get inconsistent file
        // in archive and crc error. Unfortunately we can have the file
        // truncated in report, but by the time sender is called all valuable
        // information should have already been written to logs.
        std::ifstream f(src.c_str(), std::ios::binary);
        std::ofstream dst(tmp.c_str(), std::ios::binary | std::ios::trunc);
        log << "Copying '" << src << "' to '" << tmp << "'" << std::endl;
        dst << f.rdbuf();
      }

      zip_source *s = zip_source_file(archive, tmp.c_str(), 0, 0);
      if (!s)
      {
        log << "Could not source " << tmp << std::endl;
        continue;
      }

      // Strip full path and leave only the file's name
      std::string fn = src.substr(src.rfind(path::separator) + 1);
      if (0 > zip_add(archive, fn.c_str(), s))
      {
        log << "Could not add " << fn << " to archive" << std::endl;
        continue;
      }

      filesArchived++;
    }

    zip_close(archive);

    typedef std::vector<std::string>::iterator vector_iter;
    for (vector_iter i = temp_files.begin(); i != temp_files.end(); ++i)
    {
      log << "Deleting temporary '" << *i << "'" << std::endl;
      ::remove(i->c_str());
    }

    if (filesArchived)
    {
      // Overwrite list of files to send only the archive and minidump
      // Otherwise fallback to uncompressed data
      std::map<std::string, std::string> tmp;
      tmp[id::dump] = cfg.files[id::dump];
      tmp[id::log] = zipPath;
      cfg.files.swap(tmp);
    }

    if (cfg.files.count(id::log))
      cfg.params["Notes"] = cfg.files[id::log];

    log << "Archived " << filesArchived << " files" << std::endl;
  }
  else
    log << "User refused to send logs" << std::endl;
}


void postprocess(Configuration &cfg, const std::string &guid)
{
  if (cfg.files.count(id::dump))
    path::rename_with_guid(cfg.files[id::dump], guid, path::ext::dump);

  if (cfg.files.count(id::log))
    path::rename_with_guid(cfg.files[id::log], guid, path::ext::archive);
}

} // namespace files

} // namespace breakpad
