// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gpuMemoryDumper/gpuMemoryDumper.h>
#include <drv/3d/dag_texture.h>
#include <3d/dag_resourceDump.h>
#include <util/dag_console.h>
#include <generic/dag_initOnDemand.h>
#include <drv/3d/dag_commands.h>
#include <util/dag_string.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>


class GPUDebugProcessor : public console::ICommandProcessor
{
public:
  GPUDebugProcessor() : console::ICommandProcessor(1000) {}
  void destroy() {}
  virtual bool processCommand(const char *argv[], int argc)
  {
    int found = 0;
    CONSOLE_CHECK_NAME("debug", "dumpGpuMem", 1, 2)
    {
      if (argc == 1)
      {
        gpu_mem_dumper::saveDump("gpuDump");
      }
      else if (argc == 2)
      {
        gpu_mem_dumper::saveDump(argv[1]);
      }
      else
      {
        console::print("Incorrect usage, too many params were given");
      }
    }
    return found;
  }
};

static int dumpNum = 0;
static InitOnDemand<GPUDebugProcessor> gpuMemConsole;

inline const char *skipChars(const char *text, int skip)
{
  if (strlen(text) > skip)
    return text + skip;

  return text;
}

inline const char *getFormatTypeInString(uint32_t formatFlags)
{
#define FORMAT_TO_NAME(fmt) \
  case fmt: return skipChars(#fmt, 7);

  switch (formatFlags & 0xFF000000U)
  {
    FORMAT_TO_NAME(TEXFMT_A8R8G8B8);
    FORMAT_TO_NAME(TEXFMT_A2R10G10B10);
    FORMAT_TO_NAME(TEXFMT_A2B10G10R10);
    FORMAT_TO_NAME(TEXFMT_A16B16G16R16);
    FORMAT_TO_NAME(TEXFMT_A16B16G16R16F);
    FORMAT_TO_NAME(TEXFMT_A32B32G32R32F);
    FORMAT_TO_NAME(TEXFMT_G16R16);
    FORMAT_TO_NAME(TEXFMT_G16R16F);
    FORMAT_TO_NAME(TEXFMT_G32R32F);
    FORMAT_TO_NAME(TEXFMT_R16F);
    FORMAT_TO_NAME(TEXFMT_R32F);
    FORMAT_TO_NAME(TEXFMT_DXT1);
    FORMAT_TO_NAME(TEXFMT_DXT3);
    FORMAT_TO_NAME(TEXFMT_DXT5);
    FORMAT_TO_NAME(TEXFMT_R32G32UI);
    FORMAT_TO_NAME(TEXFMT_L16);
    FORMAT_TO_NAME(TEXFMT_A8);
    FORMAT_TO_NAME(TEXFMT_R8);
    FORMAT_TO_NAME(TEXFMT_A1R5G5B5);
    FORMAT_TO_NAME(TEXFMT_A4R4G4B4);
    FORMAT_TO_NAME(TEXFMT_R5G6B5);
    FORMAT_TO_NAME(TEXFMT_A16B16G16R16S);
    FORMAT_TO_NAME(TEXFMT_A16B16G16R16UI);
    FORMAT_TO_NAME(TEXFMT_A32B32G32R32UI);
    FORMAT_TO_NAME(TEXFMT_ATI1N);
    FORMAT_TO_NAME(TEXFMT_ATI2N);
    FORMAT_TO_NAME(TEXFMT_R8G8B8A8);
    FORMAT_TO_NAME(TEXFMT_R32UI);
    FORMAT_TO_NAME(TEXFMT_R32SI);
    FORMAT_TO_NAME(TEXFMT_R11G11B10F);
    FORMAT_TO_NAME(TEXFMT_R9G9B9E5);
    FORMAT_TO_NAME(TEXFMT_R8G8);
    FORMAT_TO_NAME(TEXFMT_R8G8S);
    FORMAT_TO_NAME(TEXFMT_BC6H);
    FORMAT_TO_NAME(TEXFMT_BC7);
    FORMAT_TO_NAME(TEXFMT_R8UI);
    FORMAT_TO_NAME(TEXFMT_R16UI);
    FORMAT_TO_NAME(TEXFMT_DEPTH24);
    FORMAT_TO_NAME(TEXFMT_DEPTH16);
    FORMAT_TO_NAME(TEXFMT_DEPTH32);
    FORMAT_TO_NAME(TEXFMT_DEPTH32_S8);
    FORMAT_TO_NAME(TEXFMT_ASTC4);
    FORMAT_TO_NAME(TEXFMT_ASTC8);
    FORMAT_TO_NAME(TEXFMT_ASTC12);
    FORMAT_TO_NAME(TEXFMT_ETC2_RG);
    FORMAT_TO_NAME(TEXFMT_ETC2_RGBA);
    FORMAT_TO_NAME(TEXFMT_PSSR_TARGET);
    default: return "ERR_TYPE";
  }
}


inline void tryToCloseFile(file_ptr_t file_handle)
{
  if (file_handle)
  {
    int tries = 0;
    while (df_close(file_handle) == -1 && ++tries < 100)
      ;

    if (tries == 100)
      logerr("Failed to close file during GPU dump!");
  }
  else
    logerr("Failed to open file during GPU dump!");
}

inline void tryToWrite(file_ptr_t file_handle, const char *buffer)
{
  if (df_write(file_handle, buffer, strlen(buffer)) == -1)
    logerr("Failed to write %s.");
}

inline void addToPrint(String &output, int param)
{
  if (param != -1)
    output.aprintf(12, "%d", param);
  output.append(";");
}

inline void addToPrint(String &output, bool param) { output.aprintf(2, "%d;", (int)param); }

inline void addToPrint(String &output, uint64_t param)
{
  if (param != (uint64_t)-1)
    output.aprintf(20, "%llu", param);
  output.append(";");
}

inline void addToPrint(String &output, const char *param)
{
  if (strlen(param) != 0)
    output.append(param);
  output.append(";");
}

inline void writeObject(file_ptr_t file, const ResourceDumpTexture &tex)
{
  String texLine;

  const uint32_t &cflag = tex.flags;

  const char *sample = cflag & TEXCF_SAMPLECOUNT_2   ? "TEXCF_SAMPLECOUNT_2"
                       : cflag & TEXCF_SAMPLECOUNT_4 ? "TEXCF_SAMPLECOUNT_4"
                       : cflag & TEXCF_SAMPLECOUNT_8 ? "TEXCF_SAMPLECOUNT_8"
                                                     : "";

  addToPrint(texLine, tex.name.c_str());                       // Name
  addToPrint(texLine, tex.memSize);                            // Size
  addToPrint(texLine, tex.getTypeInString());                  // Type
  addToPrint(texLine, tex.resX);                               // X
  addToPrint(texLine, tex.resY);                               // Y
  addToPrint(texLine, tex.depth);                              // Depth
  addToPrint(texLine, tex.layers);                             // Layers
  addToPrint(texLine, tex.mip);                                // Mip
  addToPrint(texLine, tex.mip);                                // Mip
  addToPrint(texLine, getFormatTypeInString(tex.formatFlags)); // Format
  addToPrint(texLine, tex.isColor);                            // Color
  addToPrint(texLine, tex.gpuAddress);                         // Gpu Address
  addToPrint(texLine, tex.heapId);                             // Heap Id
  addToPrint(texLine, tex.heapOffset);                         // Heap Offset
  addToPrint(texLine, tex.heapNamePS.c_str());                 // Heap name when on PS
  addToPrint(texLine, 0 < (cflag & TEXCF_RTARGET));            // TEXCF_RTARGET
  addToPrint(texLine, 0 < (cflag & TEXCF_UNORDERED));          // TEXCF_UNORDERED
  addToPrint(texLine, 0 < (cflag & TEXCF_VARIABLE_RATE));      // TEXCF_VARIABLE_RATE
  addToPrint(texLine, 0 < (cflag & TEXCF_UPDATE_DESTINATION)); // TEXCF_UPDATE_DESTINATION
  addToPrint(texLine, 0 < (cflag & TEXCF_SYSTEXCOPY));         // TEXCF_SYSTEXCOPY
  addToPrint(texLine, 0 < (cflag & TEXCF_DYNAMIC));            // TEXCF_DYNAMIC
  addToPrint(texLine, 0 < (cflag & TEXCF_READABLE));           // TEXCF_READABLE
  addToPrint(texLine, 0 < (cflag & TEXCF_WRITEONLY));          // TEXCF_WRITEONLY
  addToPrint(texLine, 0 < (cflag & TEXCF_LOADONCE));           // TEXCF_LOADONCE
  addToPrint(texLine, 0 < (cflag & TEXCF_MAYBELOST));          // TEXCF_MAYBELOST
  addToPrint(texLine, sample);                                 // SAMPLECOUNT
#if _TARGET_C1 | _TARGET_C2






#elif _TARGET_XBOX
  addToPrint(texLine, 0 < (cflag & TEXCF_CPU_CACHED_MEMORY)); // TEXCF_CPU_CACHED_MEMORY
  addToPrint(texLine, 0 < (cflag & TEXCF_LINEAR_LAYOUT));     // TEXCF_LINEAR_LAYOUT
  addToPrint(texLine, 0 < (cflag & TEXCF_ESRAM_ONLY));        // TEXCF_ESRAM_ONLY
  addToPrint(texLine, 0 < (cflag & TEXCF_MOVABLE_ESRAM));     // TEXCF_MOVABLE_ESRAM
  texLine.append(";");                                        // TEXCF_TC_COMPATIBLE
  texLine.append(";");                                        // TEXCF_RT_COMPRESSED
#else
  texLine.append(";"); // TEXCF_CPU_CACHED_MEMORY
  texLine.append(";"); // TEXCF_LINEAR_LAYOUT,
  texLine.append(";"); // TEXCF_ESRAM_ONLY,
  texLine.append(";"); // TEXCF_MOVABLE_ESRAM,
  texLine.append(";"); // TEXCF_TC_COMPATIBLE,
  texLine.append(";"); // TEXCF_RT_COMPRESSED,
#endif
  addToPrint(texLine, 0 < (cflag & TEXCF_SIMULTANEOUS_MULTI_QUEUE_USE)); // TEXCF_SIMULTANEOUS_MULTI_QUEUE_USE
  addToPrint(texLine, 0 < (cflag & TEXCF_SRGBWRITE));                    // TEXCF_SRGBWRITE
  addToPrint(texLine, 0 < (cflag & TEXCF_GENERATEMIPS));                 // TEXCF_GENERATEMIPS
  addToPrint(texLine, 0 < (cflag & TEXCF_CLEAR_ON_CREATE));              // TEXCF_CLEAR_ON_CREATE
  addToPrint(texLine, 0 < (cflag & TEXCF_TILED_RESOURCE));               // TEXCF_TILED_RESOURCE
  addToPrint(texLine, 0 < (cflag & TEXCF_TRANSIENT));                    // TEXCF_TRANSIENT

  texLine.erase(texLine.end() - 1);
  texLine.append("\n");

  tryToWrite(file, texLine.c_str());
}

inline void writeObject(file_ptr_t file, const ResourceDumpBuffer &buff)
{
  const int &scflag = buff.flags;

  String buffLine;

  addToPrint(buffLine, buff.name.c_str());                              // Name
  addToPrint(buffLine, buff.memSize);                                   // Size
  addToPrint(buffLine, buff.sysCopySize);                               // sysCopySize
  addToPrint(buffLine, buff.currentDiscard);                            // CurrentDiscardIdx
  addToPrint(buffLine, buff.currentDiscardOffset);                      // currentDiscardOffset
  addToPrint(buffLine, buff.totalDiscards);                             // totalDiscards
  addToPrint(buffLine, buff.id);                                        // ID
  addToPrint(buffLine, buff.gpuAddress);                                // Gpu Address
  addToPrint(buffLine, buff.offset);                                    // Offset
  addToPrint(buffLine, buff.heapId);                                    // CPU Address
  addToPrint(buffLine, buff.mapAddress);                                // Heap ID
  addToPrint(buffLine, buff.heapOffset);                                // Heap Offset
  addToPrint(buffLine, buff.heapNamePS.c_str());                        // Heap name when on PS
  addToPrint(buffLine, 0 < (scflag & SBCF_USAGE_SHADER_BINDING_TABLE)); // SBCF_USAGE_SHADER_BINDING_TABLE
  addToPrint(buffLine,
    0 < (scflag & SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE)); // SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE
  addToPrint(buffLine, 0 < (scflag & SBCF_DYNAMIC));                        // SBCF_DYNAMIC
  addToPrint(buffLine, 0 < (scflag & SBCF_ZEROMEM));                        // SBCF_ZEROMEM
  addToPrint(buffLine, 0 < (scflag & SBCF_INDEX32));                        // SBCF_INDEX32
  addToPrint(buffLine, 0 < (scflag & SBCF_FRAMEMEM));                       // SBCF_FRAMEMEM
  addToPrint(buffLine, 0 < (scflag & SBCF_ALIGN16));                        // SBCF_ALIGN16
  addToPrint(buffLine, 0 < (scflag & SBCF_CPU_ACCESS_WRITE));               // SBCF_CPU_ACCESS_WRITE
  addToPrint(buffLine, 0 < (scflag & SBCF_CPU_ACCESS_READ));                // SBCF_CPU_ACCESS_READ
  addToPrint(buffLine, 0 < (scflag & SBCF_BIND_VERTEX));                    // SBCF_BIND_VERTEX
  addToPrint(buffLine, 0 < (scflag & SBCF_BIND_INDEX));                     // SBCF_BIND_INDEX
  addToPrint(buffLine, 0 < (scflag & SBCF_BIND_CONSTANT));                  // SBCF_BIND_CONSTANT
  addToPrint(buffLine, 0 < (scflag & SBCF_BIND_SHADER_RES));                // SBCF_BIND_SHADER_RES
  addToPrint(buffLine, 0 < (scflag & SBCF_BIND_UNORDERED));                 // SBCF_BIND_UNORDERED
  addToPrint(buffLine, 0 < (scflag & SBCF_MISC_DRAWINDIRECT));              // SBCF_MISC_DRAWINDIRECT
  addToPrint(buffLine, 0 < (scflag & SBCF_MISC_ALLOW_RAW));                 // SBCF_MISC_ALLOW_RAW
  addToPrint(buffLine, 0 < (scflag & SBCF_MISC_STRUCTURED));                // SBCF_MISC_STRUCTURED
#if _TARGET_XBOX
  addToPrint(buffLine, 0 < (scflag & SBCF_MISC_ESRAM_ONLY)); // SBCF_MISC_ESRAM_ONLY
  buffLine.erase(buffLine.end() - 1);
  buffLine.append("\n");
#else
  buffLine.aprintf(2, ";\n");
#endif

  tryToWrite(file, buffLine.c_str());
}

inline void writeObject(file_ptr_t file, const ResourceDumpRayTrace &rt, int id)
{
  String rtLine;

  addToPrint(rtLine, id);            // id
  addToPrint(rtLine, rt.memSize);    // Size
  addToPrint(rtLine, rt.gpuAddress); // Gpu Address
  addToPrint(rtLine, rt.heapId);     // Heap ID
  addToPrint(rtLine, rt.heapOffset); // Heap Offset
  if (rt.canDifferentiate)
  {
    addToPrint(rtLine, rt.top); // Heap ID
    rtLine.erase(rtLine.end() - 1);
    rtLine.append("\n");
  }
  rtLine.append("\n");

  tryToWrite(file, rtLine.c_str());
}


namespace gpu_mem_dumper
{

void saveDump(const char *dir_name)
{
  String fn;

  const char *texHeader =
    "Name;Size;Kind;X;Y;Depth;Layers;MipLevels;Format;Color;Gpu_Address;Heap_ID;Heap_Offset;Heap_Name;TEXCF_RTTARGET;TEXCF_UNORDERED;"
    "TEXCF_"
    "VARIABLE_RATE;TEXCF_UPDATE_DESTINATION;TEXCF_SYSTEXCOPY;TEXCF_DYNAMIC;TEXCF_READABLE;TEXCF_READONLY;TEXCF_WRITEONLY;TEXCF_"
    "LOADONCE;TEXCF_MAYBELOST;TEXCF_SYSMEM;TEXCF_SAMPLECOUNT;TEXCF_CPU_CACHED_MEMORY;TEXCF_LINEAR_LAYOUT;TEXCF_ESRAM_ONLY;TEXCF_"
    "MOVABLE_ESRAM;TEXCF_TC_COMPATIBLE;TEXCF_RT_COMPRESSED;TEXCF_SIMULTANEOUS_MULTI_QUEUE_USE;TEXCF_SRGBWRITE;TEXCF_SRGBREAD;TEXCF_"
    "GENERATEMIPS;TEXCF_CLEAR_ON_CREATE;TEXCF_TILED_RESOURCE;TEXCF_TRANSIENT\n";
  const char *buffHeader =
    "Name;Size;SystemCopySize;CurrentDiscardIdx;CurrentDiscardOffset;TotalDiscards;ID;GPU_Address;Offset;CPU_Address;Heap_ID;Heap_"
    "offset;Heap_Name;SBCF_USAGE_SHADER_BINDING_TABLE;SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE;SBCF_DYNAMIC;SBCF_"
    "ZEROMEM;SBCF_"
    "INDEX32;SBCF_FRAMEMEM;SBCF_USAGE_READ_BACK;SBCF_ALIGN16;SBCF_CPU_ACCESS_WRITE;SBCF_CPU_ACCESS_READ;SBCF_BIND_VERTEX;SBCF_BIND_"
    "INDEX;SBCF_BIND_CONSTANT;SBCF_BIND_SHADER_RES;SBCF_BIND_UNORDERED;SBCF_MISC_DRAWINDIRECT;SBCF_MISC_ALLOW_RAW;SBCF_MISC_"
    "STRUCTURED;SBCF_MISC_ESRAM_ONLY\n";
  const char *rtHeader = "id;Size;GPU_Address;Heap_ID;Heap_Offset;Top\n";

#if _TARGET_C1 || _TARGET_C2

#endif
  fn.aprintf(64, "gpuDumps/%s/", dir_name);

  dd_mkpath(fn);

  Tab<ResourceDumpInfo> dumpContainer;

  if (d3d::driver_command(Drv3dCommand::GET_RESOURCE_STATISTICS, &dumpContainer))
  {

    file_ptr_t texFile = df_open(String(96, "%sTextures_%d.csv", fn, dumpNum), DF_CREATE | DF_READ | DF_WRITE);
    file_ptr_t buffFile = df_open(String(96, "%sBuffers_%d.csv", fn, dumpNum), DF_CREATE | DF_READ | DF_WRITE);

    file_ptr_t rtFile = nullptr;
    bool haveRT = false;
    if (dumpContainer[dumpContainer.size() - 1].index() == 2)
    {
      rtFile = df_open(String(96, "%sRayTracingAccelerationStructure_%d.csv", fn, dumpNum), DF_CREATE | DF_READ | DF_WRITE);
      haveRT = true;
    }

    if (!texFile || !buffFile || (haveRT && !rtFile))
    {
      tryToCloseFile(texFile);
      tryToCloseFile(buffFile);

      if (haveRT)
      {
        tryToCloseFile(rtFile);
      }

      return;
    }

    tryToWrite(texFile, texHeader);
    tryToWrite(buffFile, buffHeader);

    if (haveRT)
      tryToWrite(rtFile, rtHeader);

    int id = 0;

    for (const ResourceDumpInfo &info : dumpContainer)
    {
      switch (info.index())
      {
        case 0:
        {
          writeObject(texFile, eastl::get<ResourceDumpTexture>(info));
          break;
        }
        case 1:
        {
          writeObject(buffFile, eastl::get<ResourceDumpBuffer>(info));
          break;
        }
        case 2:
        {
          writeObject(rtFile, eastl::get<ResourceDumpRayTrace>(info), ++id);
          break;
        }
      }
    }

    if (texFile)
    {
      df_flush(texFile);
      tryToCloseFile(texFile);
    }
    if (buffFile)
    {
      df_flush(buffFile);
      tryToCloseFile(buffFile);
    }
    if (rtFile)
    {
      df_flush(rtFile);
      tryToCloseFile(rtFile);
    }
    dumpNum++;
  }
  else
  {
    console::print("Gpu Dump is not implemented on this driver yet");
  }
}
void init()
{
  gpuMemConsole.demandInit();
  add_con_proc(gpuMemConsole);
}

}; // namespace gpu_mem_dumper
