#include <startup/dag_tvosMainUi.h>
#include <osApiWrappers/appstorekit.h>

@implementation DagorGameCenterEventViewController

-(id)initWithView:(UIView*) view
{
  [super init];

  self.view = view;

  return self;
}

-(NSUInteger)supportedInterfaceOrientations
{
  return UIInterfaceOrientationMaskLandscape;
}
-(UIInterfaceOrientation)preferredInterfaceOrientationForPresentation
{
  return UIInterfaceOrientationLandscapeLeft;
}
-(BOOL)shouldAutorotate
{
  return YES;
}

- (void)showAuthenticationViewController
{
    [self presentViewController:
      [(DagorGamecenterView*)self.view getAuthenticationViewController]
                       animated:YES
                     completion:nil];
}

-(void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];

    [[NSNotificationCenter defaultCenter]
       addObserver:self
       selector:@selector(showAuthenticationViewController)
       name:PresentAuthenticationViewController
       object:nil];
 }

-(void)loadView {}

-(void)willRotateToInterfaceOrientation: (UIInterfaceOrientation)orient duration: (NSTimeInterval)duration
{
}

-(void)didRotateFromInterfaceOrientation: (UIInterfaceOrientation)fromInterfaceOrientation
{
}

- (void)dealloc
{
    [super dealloc];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}
@end
