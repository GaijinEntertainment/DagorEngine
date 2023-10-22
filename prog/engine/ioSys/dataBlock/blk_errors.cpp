#include "blk_shared.h"
#include <util/dag_string.h>

bool DataBlock::allowSimpleString = false;

bool DataBlock::strongTypeChecking = false;
bool DataBlock::singleBlockChecking = false;
bool DataBlock::allowVarTypeChange = false;
bool DataBlock::fatalOnMissingFile = true;
bool DataBlock::fatalOnLoadFailed = true;
bool DataBlock::fatalOnBadVarType = true;
bool DataBlock::fatalOnMissingVar = true;

bool DataBlock::parseIncludesAsParams = false;  // def= false;
bool DataBlock::parseOverridesNotApply = false; // def= false;
bool DataBlock::parseOverridesIgnored = false;  // def= false;
bool DataBlock::parseCommentsAsParams = false;  // def= false;

static thread_local DataBlock::IErrorReporterPipe *tls_reporter = nullptr;
static void issue_error_unhappy_path(bool do_fatal, int line, const char *errText);

#define issue_error(robust_load, do_fatal, line, ...)                         \
  do                                                                          \
  {                                                                           \
    if (EASTL_UNLIKELY(!robust_load || tls_reporter))                         \
      issue_error_unhappy_path(do_fatal, line, String(0, __VA_ARGS__).str()); \
  } while (0)

void DataBlock::issue_error_missing_param(const char *pname, int type) const
{
  issue_error(shared->blkRobustOps(), fatalOnMissingVar, __LINE__,
    "BLK param missing: block='%s', param='%s' in file <%s> (req type: %s)", getBlockName(), pname, resolveFilename(),
    dblk::resolve_type(type));
}
void DataBlock::issue_error_missing_file(const char *fname, const char *desc) const
{
  issue_error(shared->blkRobustLoad(), fatalOnMissingFile, __LINE__, "%s: '%s'", desc, fname);
}
void DataBlock::issue_error_load_failed(const char *fname, const char *desc) const
{
  if (desc)
    issue_error(false, !shared->blkRobustLoad() && fatalOnMissingFile, __LINE__, "%s, '%s'", desc, fname);
  else
    issue_error(shared->blkRobustLoad(), fatalOnMissingFile, __LINE__, "%s, '%s'", "BLK read error", fname);
}
void DataBlock::issue_error_load_failed_ver(const char *fname, unsigned req_ver, unsigned file_ver) const
{
  issue_error(false, !shared->blkRobustLoad() && fatalOnMissingFile, __LINE__, "BLK wrong format: %d, expected %d, '%s'", file_ver,
    req_ver, fname);
}
void DataBlock::issue_error_parsing(const char *fname, int curLine, const char *msg, const char *curLineP) const
{
  issue_error(shared->blkRobustLoad(), DataBlock::fatalOnLoadFailed, __LINE__, "BLK error '%s',%d: %s:\n\n%s\n",
    fname ? fname : "<unknown>", curLine, msg, (curLineP && *curLineP) ? curLineP : "unknown");
}
void DataBlock::issue_error_bad_type(const char *pname, int type_new, int type_prev, const char *fname) const
{
  issue_error(shared->blkRobustLoad(), fatalOnBadVarType, __LINE__,
    "BLK param '%s' (type %s) already exists with type %s in file <%s>, block '%s'", pname, dblk::resolve_type(type_new),
    dblk::resolve_type(type_prev), fname, getBlockName());
}
void DataBlock::issue_error_bad_type(int pnid, int type_new, int type_prev) const
{
  issue_error_bad_type(getName(pnid), type_new, type_prev, resolveFilename());
}
void DataBlock::issue_error_bad_value(const char *pname, const char *value, int type, const char *fname, int line) const
{
  issue_error(shared->blkRobustLoad(), fatalOnLoadFailed, __LINE__, "BLK invalid %s (type %s) value in line %d of '%s': '%s'", pname,
    dblk::resolve_type(type), line, fname, value);
}
void DataBlock::issue_error_bad_type_get(int bnid, int pnid, int type_get, int type_data) const
{
  if (type_get == TYPE_INT && type_data == TYPE_INT64)
  {
    int64_t v = getInt64ByNameId(pnid, 0);
    issue_error(shared->blkRobustOps(), false, __LINE__, "BLK getInt() for int64=0x%X%08X: block='%s', param='%s'\nin file '%s'",
      unsigned(v >> 32), unsigned(v), getName(bnid), getName(pnid), resolveFilename());
    return;
  }

  issue_error(shared->blkRobustOps(), strongTypeChecking, __LINE__,
    "BLK param wrong type: block='%s', param='%s'\nQueried <%s(%s)> but type is <%s(%s)>\nin file '%s'", getName(bnid), getName(pnid),
    dblk::resolve_type(type_get), dblk::resolve_short_type(type_get), dblk::resolve_type(type_data),
    dblk::resolve_short_type(type_data), resolveFilename());
}
void DataBlock::issue_deprecated_type_change(int pnid, int type_new, int type_prev) const
{
  if (allowVarTypeChange || (type_prev == TYPE_INT && type_new == TYPE_INT64) || (type_prev == TYPE_INT64 && type_new == TYPE_INT))
    issue_error(shared->blkRobustOps(), false, __LINE__,
      "BLK deprecated type change for param '%s' from %s to %s (even if allowVarTypeChange=true) in block '%s'\n in file '%s'",
      getName(pnid), dblk::resolve_type(type_prev), dblk::resolve_type(type_new), getBlockName(), resolveFilename());
  else
    issue_error_bad_type(getName(pnid), type_new, type_prev, resolveFilename());
}

void DataBlock::issue_warning_huge_string(const char *pname, const char *value, const char *fname, int line) const
{
  if (!shared->blkRobustLoad())
    logwarn("BLK parsed string for param '%s' is really long (%d bytes) in line %d of '%s': '%s'", pname, strlen(value), line, fname,
      value);
  G_UNUSED(pname);
  G_UNUSED(value);
  G_UNUSED(fname);
  G_UNUSED(line); // for case of DAGOR_DBGLEVEL<1 without forced logs
}

static void issue_error_unhappy_path(bool do_fatal, int line, const char *errText)
{
  if (tls_reporter)
    return tls_reporter->reportError(errText, do_fatal);
  if (do_fatal)
    _core_fatal(__FILE__, line, true, errText);
  else
  {
    __log_set_ctx(__FILE__, line, LOGLEVEL_ERR);
    logmessage(LOGLEVEL_ERR, errText);
  }
}

DataBlock::InstallReporterRAII::InstallReporterRAII(DataBlock::IErrorReporterPipe *rep)
{
  prev = tls_reporter;
  if (rep)
    tls_reporter = rep;
}
DataBlock::InstallReporterRAII::~InstallReporterRAII() { tls_reporter = prev; }

const char *DataBlock::resolveFilename(bool file_only) const
{
  const char *s = shared->getSrc();
#if DAGOR_DBGLEVEL > 0
  if (file_only && s && strncmp(s, "BLK\n", 4) == 0)
    return "unknown";
#endif
  G_UNUSED(file_only);
  return (s && *s) ? s : (this == &emptyBlock ? "empty" : "unknown");
}

bool DataBlock::isValid() const { return shared->blkValid(); }

#define EXPORT_PULL dll_pull_iosys_datablock_errors
#include <supp/exportPull.h>
