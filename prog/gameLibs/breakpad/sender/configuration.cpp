#include "configuration.h"
#include "files.h"
#include "lang.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "log.h"


namespace breakpad
{

const char *product_name = "ProductName";
const char *product_title = "ProductTitle";
const char *param_names[] = {"Version", "BuildID", "Comments", "StartupTime", "CrashTime", "ReleaseChannel", "Hang", "HangID", NULL};

static std::string get_dump_id(const std::string &path)
{
  size_t idEnd = path.rfind('.');
  size_t idStart = path.rfind('/') + 1;
  return path.substr(idStart, idEnd - idStart);
}

void Configuration::addComment(const std::string &k, const std::string &v)
{
  if (!v.empty())
  {
    if (!params["Comments"].empty())
      params["Comments"] += "\n";
    if (!k.empty())
      params["Comments"] += k + ": ";
    params["Comments"] += v;
  }
}


void Configuration::update(int argc, char **argv)
{
  std::string dump_path, dump_dir, dump_id;
  std::vector<std::string> urlOverrides;

  // Assumes argv[argc] == NULL
  for (int i = 1; i < argc; ++i)
  {
    if (!argv[i + 1] || argv[i + 1][0] == '-')
      continue; // Skip arguments without values

      // Skip '--' in comparison
#define ARGMATCH(s) !strncmp((s), &argv[i][2], strlen((s)))
    if (ARGMATCH("file"))
    {
      std::string path = argv[++i];
      files[id::file(path)] = path;
    }
    else if (!strcmp(argv[i], "--")) // no more args, parent follows
    {
      for (++i; argv[i] && i < argc; ++i)
        parent.emplace_back(argv[i]);
    }
    else
      for (int j = 0; param_names[j]; ++j)
        if (ARGMATCH(param_names[j]))
          params[param_names[j]] = argv[++i];
        else if (ARGMATCH("dump_dir"))
          dump_dir = argv[++i];
        else if (ARGMATCH("dump_id"))
          dump_id = argv[++i];
        else if (ARGMATCH("dump_path"))
          dump_path = argv[++i];
        else if (ARGMATCH("log"))
          files[id::log] = argv[++i];
        else if (ARGMATCH("silent"))
          silent = ('0' != *argv[++i]);
        else if (ARGMATCH("url"))
          urlOverrides.emplace_back(argv[++i]);
        else if (ARGMATCH("useragent"))
          userAgent = argv[++i];
        else if (ARGMATCH("systemid"))
          systemId = argv[++i];
        else if (ARGMATCH("uid"))
          userId = argv[++i];
        else if (ARGMATCH("crashtype"))
          crashType = argv[++i];
        else if (ARGMATCH("env"))
          stats.env = argv[++i];
        else if (ARGMATCH(product_name))
        {
          ++i;
          params[product_name] = argv[i];
          product = argv[i];
        }
        else if (ARGMATCH(product_title))
        {
          ++i;
          params[product_title] = argv[i];
          productTitle = argv[i];
        }
        else if (ARGMATCH("lang"))
          ui::set_language(argv[++i]);
#undef ARGMATCH
  }

  if (dump_path.empty() && !dump_id.empty())
  {
    std::string dir = files::path::normalize(dump_dir);
    if (!dir.empty() && dir.back() != files::path::separator)
      dir += files::path::separator;

    dump_path = dir + dump_id + ".dmp";
  }

  if (!dump_path.empty())
  {
    files[id::dump] = dump_path;
    haveDump = true;
  }

  if (productTitle.empty())
    productTitle = product;

  if (params.count("CrashTime"))
    timestamp = strtol(params["CrashTime"].c_str(), NULL, 10);
  if (!timestamp)
  {
    timestamp = time(NULL);
    params["CrashTime"] = std::to_string(timestamp);
  }

  if (params.count("Hang") && params["Hang"] != "0" && !params.count("HangID"))
  {
    if (!dump_id.empty())
      params["HangID"] = dump_id;
    else
      params["HangID"] = get_dump_id(dump_path);
  }

  addComment("System ID", systemId);
  addComment("User ID", userId);

  std::string newLogPath = dump_dir + log.getPath();
  log << "Moving log file to " << newLogPath << std::endl;
  log.move(newLogPath);

  log << "Adding " << log.getPath() << " for upload" << std::endl;
  files[id::file(log.getPath())] = log.getPath();

  if (!urlOverrides.empty())
    urls = urlOverrides;
}

bool Configuration::isValid() const { return haveDump && !urls.empty() && !files.empty(); }


static std::ostream &operator<<(std::ostream &o, const std::string &s) { return o << (s.empty() ? "<none>" : s.c_str()); }

static std::ostream &operator<<(std::ostream &o, const std::pair<std::string, std::string> &p)
{
  return o << p.first << " : '" << p.second << "'";
}

template <typename T>
static inline void _print_container(std::ostream &os, const std::string &name, T c)
{
  os << "\n" << name << ":";
  if (c.empty())
    os << " <none>";
  else
    for (typename T::const_iterator i = c.begin(); i != c.end(); ++i)
      os << "\n\t" << *i;
}


std::ostream &operator<<(std::ostream &os, const Configuration &c)
{
  typedef std::map<std::string, std::string>::const_iterator iter_t;
  std::ios_base::fmtflags flags = os.flags();
  os << std::boolalpha;

  os << "---- general ----"
     << "\nvalid: " << c.isValid() << "\nsilent: " << c.silent << "\nhas dump: " << c.haveDump << "\nlogs allowed: " << c.sendLogs
     << "\nemail allowed: " << c.allowEmail << "\nparent: ";
  for (const auto &a : c.parent)
    os << a << " ";
  os << "\n---- upload -----";
  for (const auto &url : c.urls)
    os << "\nurl: " << url;
  os << "\nUA: " << c.userAgent;

  _print_container(os, "parameters", c.params);
  _print_container(os, "files", c.files);

  os << "\n---- crash ------"
     << "\nproduct title: " << c.productTitle << "\nproduct: " << c.product << "\ntimestamp: " << c.timestamp
     << "\nsystemId: " << c.systemId << "\nuser Id: " << c.userId << "\ntype: " << c.crashType << "\n---- stats ------"
     << "\nhost: " << c.stats.host << ":" << c.stats.port << "\nenv: " << c.stats.env;

  os.flags(flags);

  return os;
}

} // namespace breakpad
