#import <Foundation/Foundation.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>

namespace debug_internal
{

static NSFileHandle *_log_file = nil;

NSString* get_cache_dir_path()
{
  NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
  return [paths objectAtIndex: 0];
}

const char *get_logfilename_for_sending_tvos()
{
  NSString *log_file_path = [get_cache_dir_path() stringByAppendingPathComponent:@"log0.txt"];
  return [log_file_path cStringUsingEncoding:NSASCIIStringEncoding];
}

const char *get_cachedlog_filename_tvos(int index)
{
  NSString *filePath = [NSString stringWithFormat:@"%@/log%d.txt", get_cache_dir_path(), index];
  return [filePath cStringUsingEncoding:NSASCIIStringEncoding];
}

void force_close_log()
{
  if (_log_file)
  {
    auto finishStr = "--------------- END OF LOG ------------------\n";
    NSData *wdata = [NSData dataWithBytes:finishStr length:strlen(finishStr)];
    [_log_file seekToEndOfFile];
    [_log_file writeData: wdata];

    [_log_file closeFile];
    _log_file = nil;
  }
}

void advance_previous_logs()
{
  //get current cache directory path
  NSString *cacheDirectory = get_cache_dir_path();
  NSFileManager *filemgr = [NSFileManager defaultManager];

  //save last 4 log files, because we can have crashes or other necessary log
  //rename logs with log3.txt -> log4.txt, log2.txt -> log3.txt, etc...
  NSError *err = nil;

  //remove oldest file log4.txt, because log3.txt couldnot moves if it exist
  NSString *oldestLogPath = [NSString stringWithFormat:@"%@/log4.txt", cacheDirectory];
  [filemgr removeItemAtPath:oldestLogPath error:&err];

  for (int i=3; i >= 0; --i)
  {
    NSString *filePath = [NSString stringWithFormat:@"%@/log%d.txt", cacheDirectory, i];
    NSLog(@"[TVOS] rename old log file %@", filePath);
    BOOL fileExists = [filemgr fileExistsAtPath:filePath];

    if (fileExists)
    {
      NSString *newFilePath = [NSString stringWithFormat:@"%@/log%d.txt", cacheDirectory, i+1];
      BOOL result = [filemgr moveItemAtPath:filePath toPath:newFilePath error:&err];
      if (!result)
        NSLog(@"[TVOS] Rename file error: %@", err);
    }
  }

  //remove last log0.txt, because debug_log_tvos() will append new messages
  NSString *lastLogPath = [cacheDirectory stringByAppendingPathComponent:@"log0.txt"];
  [filemgr removeItemAtPath:lastLogPath error:nil];
}

void debug_log_tvos(const char *data)
{
  //create new log file on application start, save path to log for future use this session
  if (_log_file == nil)
  {
    advance_previous_logs();

    //!!! BE AWARE/TVOS SPECIFIC !!!
    //find the current cache directory, we use caches directory because OS
    //saves files here some time after application die, instead tmp directory
    //will clear once it close. But caches dir not guaranted that logs will be
    //saved along application live, it will be removed just OS want to do it
    NSString *cache_dir_path = get_cache_dir_path();
    NSString *log_file_path = [cache_dir_path stringByAppendingPathComponent:@"log0.txt"];

    NSData *dbuffer = [@"PREFIX: " dataUsingEncoding: NSASCIIStringEncoding];
    NSFileManager *filemgr = [NSFileManager defaultManager];
    [filemgr createFileAtPath: log_file_path contents: dbuffer attributes:nil];

    _log_file = [NSFileHandle fileHandleForWritingAtPath: log_file_path];
  }

  if (_log_file == nil)
      return;

  int len = strlen(data);
  NSData *wdata = [NSData dataWithBytes:data length:len];
  [_log_file seekToEndOfFile];
  [_log_file writeData: wdata];

  static unsigned int last_time_flush = 0;
  if (::get_time_msec() - last_time_flush > 5000)
  {
    last_time_flush = ::get_time_msec();
    //syncronizeFile is alias for flush()
    //An invocation of this method does not return until memory is flushed and it slow on tvOS
    [_log_file synchronizeFile];
  }
}

}//end namespace debug_internal
