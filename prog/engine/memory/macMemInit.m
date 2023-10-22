#include "macMemInit.h"
#import <Foundation/Foundation.h>

@interface __DagorMemStaticInit: NSObject
{
}
@end

@implementation __DagorMemStaticInit
+ (void)load                                 
{
#if DAGOR_DBGLEVEL>1
  NSLog(@"initing DagorEngine memmgr");
#endif
  dagor_force_init_memmgr();
}
@end

int pull_dagor_force_init_memmgr = 0;
