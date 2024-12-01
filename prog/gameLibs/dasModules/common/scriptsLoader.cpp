// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/dasScriptsLoader.h>
#include <util/dag_threadPool.h>
#include "../../gameLibs/ecs/scripts/das/das_ecs.h"
#include <zstd.h>

namespace bind_dascript
{
constexpr uint32_t stackSize = 16 * 1024;

das::StackAllocator &get_shared_stack()
{
  static thread_local das::StackAllocator sharedStack(stackSize);
  return sharedStack;
}

bool debugSerialization = false;
bool enableSerialization = false;
bool serializationReading = false;
bool suppressSerialization = false;
eastl::string deserializationFileName;
eastl::string serializationFileName;
das::unique_ptr<das::SerializationStorage> initSerializerStorage;
das::unique_ptr<das::SerializationStorage> initDeserializerStorage;
das::unique_ptr<das::AstSerializer> initSerializer;
das::unique_ptr<das::AstSerializer> initDeserializer;

template <typename TLoadedScript, typename TContext>
void DasScripts<TLoadedScript, TContext>::fillSharedModules(TLoadedScript &script, const das::string &fname,
  das::smart_ptr<DagFileAccess> access, das::ModuleGroup &lib_group)
{
  if (!access->storeOpenedFiles)
    return;
  DagFileAccess *globalFileAccess = !sandboxMode ? moduleFileAccess.get() : nullptr;
  das::vector<das::ModuleInfo> req;
  das::vector<das::RequireRecord> missing, circular;
  das::vector<das::RequireRecord> notAllowed;
  das::vector<das::FileInfo *> chain;
  das::das_set<das::string> dependencies;
  if (das::getPrerequisits(fname, access, req, missing, circular, notAllowed, chain, dependencies, lib_group, nullptr, 1, false))
  {
    for (das::ModuleInfo &moduleInfo : req)
    {
      if (script.filesOpened.find(moduleInfo.fileName) == script.filesOpened.end())
      {
        if (globalFileAccess)
        {
          auto globalFile = globalFileAccess->filesOpened.find(moduleInfo.fileName);
          if (globalFile != globalFileAccess->filesOpened.end())
          {
            script.filesOpened.emplace(moduleInfo.fileName, globalFile->second);
            continue;
          }
        }
        DagorStat buf;
        df_stat(moduleInfo.fileName.c_str(), &buf);
        script.filesOpened.emplace(moduleInfo.fileName, buf.mtime);
      }
    }
  }
}

void populateDummyLibGroup(das::ProgramPtr program, das::ModuleGroup *dummyLibGroup)
{
  program->thisModuleGroup = dummyLibGroup;
  program->library.foreach(
    [&](das::Module *m) {
      dummyLibGroup->addModule(m);
      return true;
    },
    "*");
  dummyLibGroup->getModules().pop_back();
}

template <typename TLoadedScript, typename TContext>
bool DasScripts<TLoadedScript, TContext>::loadScriptInternal(const das::string &fname, das::smart_ptr<DagFileAccess> access,
  AotMode aot_mode, ResolveECS resolve_ecs, LogAotErrors log_aot_errors, EnableDebugger enable_debugger)
{
  static const char *alwaysSkipSuffixes[] = {"_common.das", "_macro.das"};
  static const char *debugFiles[] = {"_console.das", "_debug.das"};
  static const char *eventFiles[] = {"_events.das"};
  if (ends_with_suffix(fname, eastl::begin(alwaysSkipSuffixes), eastl::end(alwaysSkipSuffixes)))
  {
    debug("daScript: skip module/macro file '%s'", fname.c_str());
    return true;
  }
  const bool debugScript = ends_with_suffix(fname, eastl::begin(debugFiles), eastl::end(debugFiles));
  if (!(das::is_in_aot() || das::is_in_completion()))
  {
    if (!loadDebugCode && debugScript)
    {
      debug("daScript: skip debug file '%s'", fname.c_str());
      return true;
    }
    if (!loadEvents && ends_with_suffix(fname, eastl::begin(eventFiles), eastl::end(eventFiles)))
    {
      debug("daScript: skip event file '%s'", fname.c_str());
      return true;
    }
  }

  const uint64_t loadStartTime = profile_ref_ticks();
  DebugPrinter tout;
  auto dummyLibGroup = eastl::make_unique<das::ModuleGroup>();
  processModuleGroupUserData(fname, *dummyLibGroup);
  debug("daScript: load script <%s>", fname.c_str());
  das::CodeOfPolicies policies;
  policies.aot = aot_mode == AotMode::AOT;
  const bool noLinter = (bool)::dgs_get_argv("das-no-linter");
  policies.no_unused_function_arguments = !noLinter;
  policies.no_unused_block_arguments = !noLinter;
  policies.fail_on_lack_of_aot_export = true;
  policies.no_global_variables = true;
  policies.fail_on_no_aot = false;
  policies.debugger = enable_debugger == EnableDebugger::YES;
  policies.no_unsafe = sandboxMode;
  policies.no_aliasing = true;
  policies.strict_unsafe_delete = true;
  policies.stack = 4096;

  das::ProgramPtr program;

  if (enableSerialization && serializationReading)
  {
    program = das::make_smart<das::Program>();
    if (initDeserializer)
      initDeserializer->thisModuleGroup = dummyLibGroup.get();
    if (initDeserializer && initDeserializer->serializeScript(program))
    {
      access->getFileInfo(fname); // add script file to openedFiles to support hot reload
                                  // (other openedFiles are added later in getPrerequisites function,
                                  //    but main file is not a prerequisite for itself)
      populateDummyLibGroup(program, dummyLibGroup.get());
    }
    else
    {
      dummyLibGroup->reset();                                                        // clean partially serialized modules
      program = das::compileDaScript(fname, access, tout, *dummyLibGroup, policies); // try to recompile to get better error messages
      bind_dascript::enableSerialization = false;
    }
  }

  if (!program)
  {
    program = das::compileDaScript(fname, access, tout, *dummyLibGroup, policies);
  }

  if (program)
  {
    if (program->failed())
    {
      logerr("failed to compile <%s>\n", fname.c_str());
      for (auto &err : program->errors)
      {
        G_UNUSED(err);
        logerr(das::reportError(err.at, err.what, err.extra, err.fixme, err.cerr).c_str());
        compileErrorsCount++;
      }
      das::lock_guard<das::recursive_mutex> guard(mutex);
      auto it = scripts.find(fname);
      if (it == scripts.end())
      {
        TLoadedScript script;
        script.filesOpened = eastl::move(access->filesOpened);
        fillSharedModules(script, fname, access, *dummyLibGroup);
        scripts[fname] = eastl::move(script);
      }
      else
      {
        it->second.filesOpened = eastl::move(access->filesOpened);
        fillSharedModules(it->second, fname, access, *dummyLibGroup);
      }
      storeSharedQueries(*dummyLibGroup);
      return false;
    }
    {
      das::lock_guard<das::recursive_mutex> guard(mutex);
      if (!postProcessModuleGroupUserData(fname, *dummyLibGroup))
      {
        logerr("daScript: failed to post process<%s>, internal error, use %d\n", fname.c_str(), program->use_count());
        return false;
      }
    }

    // Do not write out anything when loading is in progress (in serveral threads)
    if (enableSerialization && !serializationReading && !suppressSerialization && initSerializer)
      program->serialize(*initSerializer);

    das::shared_ptr<TContext> ctx = das::make_shared<TContext>(program->unsafe ? program->getContextStackSize() : 0);
#if DAS_HAS_DIRECTORY_WATCH
    ctx->name = fname;
#endif
    bool simulateRes = false;
    if (!program->unsafe)
    {
      // das::SharedStackGuard guard(*ctx, get_shared_stack());
      // simulateRes = program->simulate(*ctx, tout, &get_shared_stack());
      simulateRes = program->simulate(*ctx, tout);
    }
    else if (sandboxMode)
    {
      logerr("daScript: unsafe context <%s>", fname.c_str());
    }
    else
    {
      logwarn("daScript: unsafe context <%s>", fname.c_str());
      simulateRes = program->simulate(*ctx, tout);
    }
    if (!simulateRes)
    {
      if (ctx->getException())
        logerr("daScript: failed to compile, exception <%s>\n", ctx->getException());
      else
      {
        logerr("daScript: failed to compile<%s>, internal error\n", fname.c_str());
        for (auto &err : program->errors)
        {
          G_UNUSED(err);
          logerr(das::reportError(err.at, err.what, err.extra, err.fixme, err.cerr).c_str());
        }
      }
      return false;
    }
    // this should only be here when AOT is enabled
    if (aot_mode == AotMode::AOT && !program->aotErrors.empty())
    {
      linkAotErrorsCount += uint32_t(program->aotErrors.size());
      logwarn("daScript: failed to link cpp aot <%s>\n", fname.c_str());
      if (log_aot_errors == LogAotErrors::YES)
        for (auto &err : program->aotErrors)
        {
          logwarn(das::reportError(err.at, err.what, err.extra, err.fixme, err.cerr).c_str());
        }
    }
    const AotMode aotModeOverride = program->options.getBoolOption("no_aot", false) ? AotMode::NO_AOT : AotMode::AOT;
    if (debugScript)
    {
      if (aotModeOverride != AotMode::NO_AOT)
        logerr("%s debug file was loaded with enabled aot. Please disable aot using `options no_aot`"
               " or remove this error if you really need this",
          fname.c_str());
    }
    else
    {
      if (program->options.getBoolOption("gc", false))
        logerr("`%s`: options gc - is allowed only for debug/console scripts.", fname.c_str());
      if (program->options.getBoolOption("rtti", false))
        logerr("`%s`: options rtti - is allowed only for debug/console/macro scripts.", fname.c_str());
    }

    AotModeIsRequired aotModeIsRequired = !debugScript ? AotModeIsRequired::YES : AotModeIsRequired::NO;
    TLoadedScript script(eastl::move(program), eastl::move(ctx), access, aotModeOverride, aotModeIsRequired, enable_debugger);
    script.filesOpened = eastl::move(access->filesOpened);
    fillSharedModules(script, fname, access, *dummyLibGroup);

#if DAS_HAS_DIRECTORY_WATCH
    for (auto &f : script.filesOpened)
    {
      const char *realName = df_get_real_name(f.first.c_str());
      if (!realName)
        continue;
      const char *fnameExt = dd_get_fname(realName);
      char buf[512];
      if (!fnameExt || fnameExt <= realName || fnameExt - realName >= sizeof(buf))
        continue;
      memcpy(buf, realName, fnameExt - realName - 1);
      buf[fnameExt - realName - 1] = 0;
      dd_simplify_fname_c(buf);
      das::lock_guard<das::recursive_mutex> guard(mutex);
      if (dirsOpened.find_as((const char *)buf) != dirsOpened.end())
        continue;
      bool hasUpperPath = false;
      const int bufLen = strlen(buf);
      for (auto &d : dirsOpened)
      {
        if (d.first.length() >= bufLen)
          continue;
        if (strncmp(d.first.c_str(), buf, d.first.length()) == 0)
        {
          hasUpperPath = true;
          break;
        }
      }
      if (hasUpperPath)
        continue;
      WatchedFolderMonitorData *handle = add_folder_monitor(buf, 100);
      if (handle)
      {
        dirsOpened.emplace(buf, eastl::unique_ptr<WatchedFolderMonitorData, DirWatchHandleDeleter>(handle));
        debug("watch dir = %s", buf);
      }
    }
#endif

    if (script.program->thisModule->isModule)
    {
      if (!ends_with(fname, "events.das")) // we still load/unload events files to perform annotations macro
        logerr("daScript: '%s' is module. If it's file with events, rename it to <fileName>_events.das. Otherwise rename it to "
               "<fileName>_common.das or <fileName>_macro.das (macro in case it contains macroses)",
          fname.c_str());
      const int num = getPendingSystemsNum(*script.program->thisModuleGroup);
      if (num == 0)
      {
        if (!das::is_in_aot())
        {
          G_ASSERT(script.systems.empty() && script.queries.empty());
          debug("das: unload module `%s`", fname.c_str());
          script.ctx.reset();
          script.program.reset();
          return true;
        }
      }
      else
      {
        logwarn("das: unable to unload potentially unused module %s", fname.c_str());
      }
    }

    if (!script.ctx->persistent)
    {
      script.program->library.foreach(
        [&fname](das::Module *mod) {
          mod->globals.foreach([&fname](auto globVar) {
            if (globVar && globVar->used && !globVar->type->isNoHeapType())
            {
              const auto ignoreMark = globVar->annotation.find("ignore_heap_usage", das::Type::tBool);
              if (ignoreMark == nullptr || !ignoreMark->bValue)
                logerr("global variable '%s' in script <%s> requires heap memory, but heap memory is regularly cleared."
                       "Only value types/fixed arrays are supported in this case",
                  globVar->name, fname.c_str());
            }
          });
          return true;
        },
        "*");
    }


    das::lock_guard<das::recursive_mutex> guard(mutex);

    if (!script.ctx->persistent)
    {
      auto memIt = scriptsMemory.find(fname);
      if (memIt != scriptsMemory.end())
        scriptsMemory.erase(memIt);
    }
    else
    {
      // persistent
      scriptsMemory[fname] =
        ScriptMemory{(uint64_t)script.ctx->heap->getInitialSize(), (uint64_t)script.ctx->stringHeap->getInitialSize(), 0};
    }

    if (storeTemporaryScripts)
      temporaryScripts.push_back(fname);

    auto it = scripts.find(fname);
    if (it != scripts.end())
    {
      debug("daScript: unload %s", fname.c_str());
      it->second.unload();
    }

    if (!enableSerialization)
      dummyLibGroup->reset(); // cleanup submodules to reduce memory usage

    script.moduleGroup = eastl::move(dummyLibGroup);

    if (resolve_ecs == ResolveECS::YES)
      postLoadScript(script, fname, profile_ref_ticks());

    // Note: In case of serialization we cannot reset now, as the data will be needed later
    // The clean up is moved to "after the load" stage if we are reading, or after the write if we are writing.
    if (!enableSerialization && !script.program->options.getBoolOption("rtti", false) && !das::is_in_aot())
      script.program.reset(); // we don't need program to be loaded. Saves memory

    scripts[fname] = eastl::move(script);
  }
  else
  {
    logerr("internal compile error <%s>\n", fname.c_str());
    return false;
  }
  debug("daScript: file %s loaded in %d ms", fname.c_str(), profile_time_usec(loadStartTime) / 1000);
  return true;
}

template bool DasScripts<LoadedScript, EsContext>::loadScriptInternal(const das::string &fname, das::smart_ptr<DagFileAccess> access,
  AotMode aot_mode, ResolveECS resolve_ecs, LogAotErrors log_aot_errors, EnableDebugger enable_debugger);


template <typename TLoadedScript, typename TContext>
bool DasScripts<TLoadedScript, TContext>::postLoadScript(TLoadedScript &script, const das::string &fname, uint64_t load_start_time)
{
  if (!script.moduleGroup)
    return true;

  bool res = true;
  if (!processLoadedScript(script, fname, load_start_time, *script.moduleGroup, script.hasAot ? AotMode::AOT : AotMode::NO_AOT,
        script.requireAot ? AotModeIsRequired::YES : AotModeIsRequired::NO))
  {
    logerr("daScript: failed to process<%s>, internal error\n", fname.c_str());
    res = false;
  }

  if (!keepModuleGroupUserData(fname, *script.moduleGroup) && !enableSerialization)
    script.moduleGroup.reset();

  return res;
}

template bool DasScripts<LoadedScript, EsContext>::postLoadScript(LoadedScript &script, const das::string &fname,
  uint64_t load_start_time);


size_t FileSerializationStorage::writingSize() const { return df_tell(file); }

bool FileSerializationStorage::readOverflow(void *data, size_t size)
{
  const int read = df_read(file, data, size);
  return read == size;
}

void FileSerializationStorage::write(const void *data, size_t size)
{
  const int res = df_write(file, data, (int)size);
  if (res != size)
    logerr("can't write to serialization storage '%@'", fileName.c_str());
}

FileSerializationStorage::~FileSerializationStorage()
{
  df_close(file);
  file = nullptr;
}

// ----------------------------

struct FileSerializationRead final : FileSerializationStorage
{
  const int buffInSize = ZSTD_DStreamInSize();
  int readPos = 0, fileLen = 0;
  const char *fileMapping = nullptr;
  ZSTD_DCtx *cctx = ZSTD_createDCtx();
  char tempReadBuffer[ZSTD_BLOCKSIZE_MAX]; // Last member

  FileSerializationRead(file_ptr_t file_, const das::string &name);
  virtual size_t writingSize() const override { return 0; }
  virtual bool readOverflow(void *data, size_t size) override;
  virtual void write(const void *data, size_t size) override;
  virtual ~FileSerializationRead() override;
};


FileSerializationRead::FileSerializationRead(file_ptr_t file_, const das::string &name) : //-V730
  FileSerializationStorage(file_, name)
{
  fileMapping = (const char *)df_mmap(file, &fileLen);
}

FileSerializationRead::~FileSerializationRead()
{
  df_unmap(fileMapping, fileLen);
  ZSTD_freeDCtx(cctx);
}

bool FileSerializationRead::readOverflow(void *data, size_t size)
{
  char *dataPtr = (char *)data;

  while (size)
  {
    if (buffer.empty())
    {
      int toRead = min(fileLen - readPos, buffInSize);
      if (toRead <= 0)
        return false;
      ZSTD_inBuffer input = {fileMapping + readPos, (size_t)toRead, 0};
      readPos += toRead; //-V1026 ignore signed overflow possibility
      buffer.clear();
      while (input.pos < input.size) // To consider: decompress directly to `buffer` instead of `tempReadBuffer`
      {
        ZSTD_outBuffer output = {tempReadBuffer, sizeof(tempReadBuffer), 0};
        const size_t ret = ZSTD_decompressStream(cctx, &output, &input);
        if (ZSTD_isError(ret)) // decompression error, maybe previous format file
        {
          return false;
        }
        auto bSize = buffer.size();
        buffer.resize_noinit(bSize + output.pos);
        memcpy(buffer.data() + bSize, tempReadBuffer, output.pos);
      }
    }

    const size_t bytesToRead = eastl::min(size, buffer.size() - bufferPos);
    if (bytesToRead == 0)
    {
      return false;
    }
    memcpy(dataPtr, buffer.data() + bufferPos, bytesToRead);
    bufferPos += bytesToRead;
    dataPtr += bytesToRead;
    size -= bytesToRead;

    if (bufferPos == buffer.size())
    {
      buffer.resize(0);
      bufferPos = 0;
    }
  }
  return true;
}

void FileSerializationRead::write(const void *, size_t) { logerr("can't write to read-only serialization storage"); }
// ----------------------------


struct FileSerializationWrite final : FileSerializationStorage
{
  das::vector<char> writeBuffer;
  size_t totalSize = 0;
  ZSTD_CCtx *cctx = nullptr;

  FileSerializationWrite(file_ptr_t file_, const das::string &name);
  virtual size_t writingSize() const override;
  virtual bool readOverflow(void *data, size_t size) override;
  virtual void write(const void *data, size_t size) override;
  void flush(ZSTD_EndDirective end_mode);
  virtual ~FileSerializationWrite() override;
};


FileSerializationWrite::FileSerializationWrite(file_ptr_t file_, const das::string &name) : FileSerializationStorage(file_, name)
{
  cctx = ZSTD_createCCtx();
  ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, 1);
  ZSTD_CCtx_setParameter(cctx, ZSTD_c_checksumFlag, 1);

  size_t const buffInSize = ZSTD_CStreamInSize();
  size_t const buffOutSize = ZSTD_CStreamOutSize();
  buffer.resize(buffInSize);
  writeBuffer.resize_noinit(buffOutSize);
}

size_t FileSerializationWrite::writingSize() const { return totalSize; }

bool FileSerializationWrite::readOverflow(void *, size_t)
{
  logerr("can't read from write-only serialization storage");
  return false;
}

void FileSerializationWrite::write(const void *data, size_t size)
{
  char *dataPtr = (char *)data;
  while (size)
  {
    const size_t bytesToWrite = eastl::min(size, buffer.size() - bufferPos);
    memcpy(buffer.data() + bufferPos, dataPtr, bytesToWrite);
    bufferPos += bytesToWrite;
    dataPtr += bytesToWrite;
    size -= bytesToWrite;
    totalSize += bytesToWrite;
    if (bufferPos == buffer.size())
      flush(ZSTD_e_continue);
  }
}

void FileSerializationWrite::flush(ZSTD_EndDirective end_mode)
{
  int finished;
  do
  {
    ZSTD_inBuffer input = {buffer.data(), (size_t)bufferPos, 0};
    ZSTD_outBuffer output = {writeBuffer.data(), writeBuffer.size(), 0};
    size_t const remaining = ZSTD_compressStream2(cctx, &output, &input, end_mode);
    G_ASSERT(!ZSTD_isError(remaining));
    df_write(file, writeBuffer.data(), (int)output.pos);
    bufferPos = 0;
    finished = end_mode == ZSTD_e_end ? (remaining == 0) : (input.pos == input.size);
  } while (!finished);
}

FileSerializationWrite::~FileSerializationWrite()
{
  flush(ZSTD_e_end);
  ZSTD_freeCCtx(cctx);
  cctx = nullptr;
}

das::SerializationStorage *create_file_read_serialization_storage(file_ptr_t file, const das::string &name)
{
  return new FileSerializationRead(file, name);
}
das::SerializationStorage *create_file_write_serialization_storage(file_ptr_t file, const das::string &name)
{
  return new FileSerializationWrite(file, name);
}

} // namespace bind_dascript
