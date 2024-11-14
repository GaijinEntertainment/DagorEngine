// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shadervarGenerator.h"

#if _TARGET_PC_WIN

#include "shLog.h"
#include "globalConfig.h"
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_findFiles.h>
#include <sstream>
#include <fstream>
#include <streambuf>
#include <string_view>
#include <ranges>
#include <regex>

static std::string get_generated_shadervars_template()
{
  if (shc::config().shadervarsCodeTemplateFilename.empty())
    return "";
  std::ifstream file(shc::config().shadervarsCodeTemplateFilename);
  if (!file.good())
  {
    sh_debug(SHLOG_FATAL, "Could not open file %s", shc::config().shadervarsCodeTemplateFilename.c_str());
    return "";
  }
  std::string result((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  return result;
}

static std::pair<std::string, int> get_generated_file_name(const std::string &shader_file_name)
{
  int no = 0;
  for (auto &[matcher, replacer] : shc::config().generatedPathInfos)
  {
    no++;
    std::regex folder_matcher(matcher.c_str());
    if (std::regex_match(shader_file_name.c_str(), folder_matcher))
      return {std::regex_replace(shader_file_name, folder_matcher, replacer).c_str(), no};
  }

  std::string matchers;
  for (auto &[matcher, _] : shc::config().generatedPathInfos)
    matchers += matcher + ", ";
  sh_debug(SHLOG_FATAL, "Couldn't match shader file %s with regexps %s", shader_file_name.c_str(), matchers.c_str());
  return {"", no};
}

static std::string get_code_to_write_to_generated_file(const std::string &file_name, std::string_view code)
{
  if (!dd_file_exist(file_name.c_str()))
  {
    if (shc::config().shadervarGeneratorMode == ShadervarGeneratorMode::Check)
      sh_debug(SHLOG_ERROR, "File doesn't exist %s", file_name.c_str());
    return std::string(code);
  }

  std::ifstream file(file_name);
  std::string file_content_str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  if (file_content_str.empty())
  {
    if (shc::config().shadervarGeneratorMode == ShadervarGeneratorMode::Check)
      sh_debug(SHLOG_ERROR, "File is empty %s", file_name.c_str());
    return std::string(code);
  }

  std::string_view file_content = file_content_str;

  auto show_diff = [&]() {
    auto [file_it, code_it] = std::ranges::mismatch(file_content, code);
    size_t file_pos = std::ranges::distance(file_content.begin(), file_it);
    size_t code_pos = std::ranges::distance(code.begin(), code_it);
    std::string file_str = std::string(file_content.substr(file_pos - 1, 256));
    std::string code_str = std::string(code.substr(code_pos - 1, 256));
    sh_debug(SHLOG_NORMAL, "The first difference is found in the file in the position\n\n%s...\n\nwhile the code is\n\n%s...",
      file_str.c_str(), code_str.c_str());
  };

  if (file_content.starts_with(code.substr(0, 64)))
  {
    if (file_content == code)
      return std::string();

    if (shc::config().shadervarGeneratorMode == ShadervarGeneratorMode::Check)
    {
      sh_debug(SHLOG_ERROR, "Generated content is different from the file %s", file_name.c_str());
      show_diff();
    }
    return std::string(code);
  }

  // The file read from the disk has a header
  // Need to compare the generated code with the contents of the file without taking into account the header
  size_t pragma_once_pos = file_content.find("#pragma once");
  size_t namespace_pos = file_content.rfind("}  // namespace intervals");
  file_content = file_content.substr(pragma_once_pos, namespace_pos - pragma_once_pos);

  bool equal = code.starts_with(file_content);
  if (equal)
    return {};

  // Need to overwrite the file, but now taking into account the header
  std::stringstream str;
  str << file_content_str.substr(0, pragma_once_pos);
  str << code.substr(0, code.rfind("}  // namespace intervals"));
  str << file_content_str.substr(namespace_pos);

  if (shc::config().shadervarGeneratorMode == ShadervarGeneratorMode::Check)
  {
    sh_debug(SHLOG_ERROR, "Generated content is different from the file %s with header", file_name.c_str());
    show_diff();
  }
  return str.str();
}

static void remove_generated_file(std::string_view file_name)
{
  bool erased = dd_erase(file_name.data());
  if (!erased)
    sh_debug(SHLOG_NORMAL, "Could not remove file <%s>", file_name.data());
}

static std::string camelcasify(std::string str)
{
  for (size_t pos = 0; pos != std::string::npos; pos = str.find("_"))
  {
    if (pos)
      str.erase(pos, 1);
    str[pos] = toupper(str[pos]);
  }
  return str;
}

static std::string uppercasify(std::string str)
{
  std::ranges::transform(str, str.begin(), toupper);
  return str;
}

template <size_t N>
size_t lengthof(const char (&)[N])
{
  return N - 1;
}

static bool process_template(std::stringstream &output, std::string_view code,
  const std::vector<const ShaderGlobal::Var *> &shadervars, int shadervar_index, const std::vector<const Interval *> &intervals,
  int interval_index, int value_index)
{
  size_t pos = code.find("{{");
  if (pos == std::string::npos)
  {
    output << code;
    return true;
  }
  output << code.substr(0, pos);
  code.remove_prefix(pos);

#define BEGIN_KEY(key)                      \
  {                                         \
    const char key_word[] = "{{" #key "}}"; \
    if (code.starts_with(key_word))         \
    {
#define END_KEY()                                                                                                          \
  return process_template(output, code.substr(lengthof(key_word)), shadervars, shadervar_index, intervals, interval_index, \
    value_index);                                                                                                          \
  }                                                                                                                        \
  }
#define NEXT_KEY(key) END_KEY() BEGIN_KEY(key)
#define FOR_EACH(elem, limiter)                                                                                               \
  std::string_view for_elems_str = code.substr(lengthof(key_word), code.find("{{end for " #elem "s}}") - lengthof(key_word)); \
  code.remove_prefix(for_elems_str.length() + lengthof("{{end for " #elem "s}}"));                                            \
  for (int i = 0; i < limiter; i++)                                                                                           \
  {                                                                                                                           \
    elem##_index = i;                                                                                                         \
    process_template(output, for_elems_str, shadervars, shadervar_index, intervals, interval_index, value_index);             \
  }

  BEGIN_KEY(shadervar_name)
  output << shadervars[shadervar_index]->getName();
  NEXT_KEY(for shadervars)
  FOR_EACH(shadervar, shadervars.size())
  NEXT_KEY(template)
  output << shc::config().shadervarsCodeTemplateFilename;
  NEXT_KEY(for intervals)
  FOR_EACH(interval, intervals.size())
  NEXT_KEY(interval_name)
  output << intervals[interval_index]->getNameStr();
  NEXT_KEY(IntervalName)
  output << camelcasify(intervals[interval_index]->getNameStr());
  NEXT_KEY(for values)
  FOR_EACH(value, intervals[interval_index]->getValueCount() - 1)
  NEXT_KEY(VALUE_NAME)
  output << uppercasify(intervals[interval_index]->getValueName(value_index));
  NEXT_KEY(FIRST_VALUE_NAME)
  output << uppercasify(intervals[interval_index]->getValueName(0));
  NEXT_KEY(LAST_VALUE_NAME)
  output << uppercasify(intervals[interval_index]->getValueName(intervals[interval_index]->getValueCount() - 1));
  NEXT_KEY(value_name)
  output << intervals[interval_index]->getValueName(value_index);
  NEXT_KEY(last_value_name)
  output << intervals[interval_index]->getValueName(intervals[interval_index]->getValueCount() - 1);
  NEXT_KEY(value)
  output << int(intervals[interval_index]->getValueRange(value_index).getMax()) - 1;
  END_KEY()

  G_ASSERTF(false, "Unknown keyword %s", code.data());
  return false;
}

void ShadervarGenerator::addShadervarsAndIntervals(dag::Span<ShaderGlobal::Var> shadervars, const IntervalList &intervals)
{
  if (shc::config().shadervarGeneratorMode == ShadervarGeneratorMode::None)
    return;

  auto add_entry = [](const char *fname, const char *vname, auto &entries, auto &var) {
    char buf[DAGOR_MAX_PATH];
    const char *path = dd_get_fname_without_path_and_ext(buf, DAGOR_MAX_PATH, fname);

    if (std::ranges::find(shc::config().excludeFromGeneration, path) != shc::config().excludeFromGeneration.end())
      return;

    dd_get_fname_location(buf, fname);

    auto &entry = entries[vname];
    auto [file_name, priority] = get_generated_file_name(buf);
    if (priority > entry.priority)
      return;
    if (priority == entry.priority && file_name >= entry.generated_file_name)
      return;

    entry.priority = priority;
    entry.generated_file_name = file_name;
    entry.var = var;
  };

  for (auto &var : shadervars)
    add_entry(var.fileName.c_str(), var.getName(), mShadervars, var);

  for (int i = 0; i < intervals.getCount(); i++)
  {
    const Interval *interval = intervals.getInterval(i);
    if (!interval || interval->getFileName().empty())
      continue;

    // Skip intervals with on/off and yes/no subintervals
    if (interval->getValueCount() == 2)
    {
      using namespace std::string_view_literals;
      if (interval->getValueName(0) == "no"sv && interval->getValueName(1) == "yes"sv)
        continue;
      if (interval->getValueName(0) == "off"sv && interval->getValueName(1) == "on"sv)
        continue;
    }

    add_entry(interval->getFileName().c_str(), interval->getNameStr(), mIntervals, *interval);
  }
}

void ShadervarGenerator::generateShadervars()
{
  if (shc::config().shadervarGeneratorMode == ShadervarGeneratorMode::None)
    return;

  std::string code_template = get_generated_shadervars_template();
  if (code_template.empty())
    return;

  struct ShadervarsByFile
  {
    std::vector<const Interval *> intervals;
    std::vector<const ShaderGlobal::Var *> shadervars;
  };

  std::map<std::string, ShadervarsByFile> shadervars_by_file;

  for (auto &[name, entry] : mShadervars)
    shadervars_by_file[entry.generated_file_name].shadervars.push_back(&entry.var);

  for (auto &[name, entry] : mIntervals)
    shadervars_by_file[entry.generated_file_name].intervals.push_back(&entry.var);

  for (auto &[generated_file_name, shadervars] : shadervars_by_file)
  {
    if (shc::config().shadervarGeneratorMode == ShadervarGeneratorMode::Remove)
    {
      remove_generated_file(generated_file_name);
      continue;
    }

    std::stringstream output;
    process_template(output, code_template, shadervars.shadervars, -1, shadervars.intervals, -1, -1);

    std::string code_to_write = get_code_to_write_to_generated_file(generated_file_name, output.str());
    if (!code_to_write.empty())
    {
      if (shc::config().shadervarGeneratorMode == ShadervarGeneratorMode::Check)
      {
        sh_debug(SHLOG_FATAL, "The generated code does not match the one that has already been saved to the file");
        continue;
      }
      bool created = dd_mkpath(generated_file_name.c_str());
      G_ASSERTF(created, "Couldn't create dir for file %s", generated_file_name.c_str());

      std::ofstream file(generated_file_name);
      file << code_to_write;
    }
  }
}

#else

void ShadervarGenerator::addShadervarsAndIntervals(dag::Span<ShaderGlobal::Var>, const IntervalList &) {}

void ShadervarGenerator::generateShadervars() {}

#endif