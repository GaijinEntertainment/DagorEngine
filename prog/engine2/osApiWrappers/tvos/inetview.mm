#include <startup/dag_tvosMainUi.h>
#include <debug/dag_debug.h>

namespace apple
{
  void setInetView(DagorInetView* view);
}

using namespace apple;

@implementation DagorInetView
- (id)initWithFrame:(CGRect)frame
{
  if (self = [super initWithFrame: frame])
  {
    apple::setInetView(self);
  }

  return self;
}

-(void)downloadFileFromNet:(const char*)url callback:(Kit::Network::IDownloadCallback*) cb
{
  //download the file in a seperate thread.
  NSString *urlToDownload = [[NSString alloc] initWithUTF8String:url];
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
    ^{
      NSURL  *inet_url = [NSURL URLWithString:urlToDownload];
      NSData *urlData = [NSData dataWithContentsOfURL:inet_url];

      if (urlData)
      {
        debug("[HTTPR]: Received answer for url = %s", url);
        //saving is done on main thread
        dispatch_async(dispatch_get_main_queue(), ^{
            if (cb)
              cb->onRequestDone(true, 200, (const char*)[urlData bytes], [urlData length]);
        });
      }
      else
      {
        debug("[HTTPR]: Failed request for url = %s", url);
        dispatch_async(dispatch_get_main_queue(), ^{
          if (cb)
            cb->onRequestDone(false, 0, nullptr, 0);
        });
      }
    }
  );
}

@end


namespace apple
{

static DagorInetView* inetview = nullptr;
void setInetView(DagorInetView* view)
{
  inetview = view;
}

void Kit::Network::downloadFile(const char* url, Kit::Network::IDownloadCallback* cb)
{
  if (inetview)
     [inetview downloadFileFromNet : url
                          callback : cb];
}

}//end namespace apple
