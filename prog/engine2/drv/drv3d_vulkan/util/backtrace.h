#pragma once
#include <util/dag_stdint.h>
class String;

// Utility for fast backtrace storage & resolve
// -caches resolved callstacks
// -maps callstack to 64bit hash to reduce callstack data storage overhead
namespace backtrace
{
// Get symbol resolved call stack data for current caller, stack data is cached internally
String get_stack();
// Get current call stack hash and store stack data internally without resolving symbols
// first ignore_frames frames are skipped in callstack
uint64_t get_hash(uint32_t ignore_frames = 0);
// Get symbol resolved call stack data for caller hash recived in get_hash, stack data is cached internally
String get_stack_by_hash(uint64_t caller_hash);
// Cleans up internal storage
void cleanup();
} // namespace backtrace
