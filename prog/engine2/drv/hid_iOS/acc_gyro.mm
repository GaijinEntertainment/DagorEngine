#import <UIKit/UIKit.h>
#import <CoreMotion/coreMotion.h>
#include <debug/dag_debug.h>

namespace hid_ios
{
double acc_x = 0, acc_y = 0, acc_z = 0;
double g_acc_x = 0, g_acc_y = 0, g_acc_z = 0;
double m_acc_x = 0, m_acc_y = 0, m_acc_z = 0;
double r_rate_x = 0, r_rate_y = 0, r_rate_z = 0;
double g_base_acc_x = 0, g_base_acc_y = 0, g_base_acc_z = -1;
double g_rel_ang_x = 0, g_rel_ang_y = 0;
} // namespace hid_ios


static CMMotionManager *motion = NULL;
static CMAttitude *refAtt = NULL;

void hid_iOS_init_motion()
{
  if (motion)
    return;
  motion = [[CMMotionManager alloc] init];
  refAtt = [motion.deviceMotion.attitude retain];

  motion.accelerometerUpdateInterval = motion.gyroUpdateInterval = 0.02;
  [motion startAccelerometerUpdates];
  [motion startGyroUpdates];
  if (!motion.gyroAvailable)
  {
    [motion stopGyroUpdates];
    refAtt = nil;
    [motion release];
    motion = NULL;
    debug_ctx("no gyroscope");
  }
}

void hid_iOS_term_motion()
{
  if (motion)
  {
    [motion stopAccelerometerUpdates];
    [motion stopGyroUpdates];
    refAtt = nil;
    [motion release];
    motion = NULL;
  }
}

void hid_iOS_update_motion()
{
  if (motion)
  {
    CMAcceleration acceleration = [[motion accelerometerData] acceleration];
    using namespace hid_ios;
    const double filt_factor = 0.1;

    acc_x = acceleration.x;
    acc_y = acceleration.y;
    acc_z = acceleration.z;

    g_acc_x = acceleration.x * filt_factor + g_acc_x * (1.0 - filt_factor);
    g_acc_y = acceleration.y * filt_factor + g_acc_y * (1.0 - filt_factor);
    g_acc_z = acceleration.z * filt_factor + g_acc_z * (1.0 - filt_factor);

    m_acc_x = acceleration.x - g_acc_x;
    m_acc_y = acceleration.y - g_acc_y;
    m_acc_z = acceleration.z - g_acc_z;

    double norm;

    // compute pitch
    norm = sqrt((g_base_acc_x * g_base_acc_x + g_base_acc_z * g_base_acc_z) * (g_acc_x * g_acc_x + g_acc_z * g_acc_z));
    if (norm > 1e-6)
      g_rel_ang_y = asin((g_base_acc_x * g_acc_z - g_base_acc_z * g_acc_x) / norm) / (M_PI / 2);
    else
      g_rel_ang_y = 0;

    // compute bank
    if (g_base_acc_z < -0.70710678)
    {
      norm = sqrt((g_base_acc_y * g_base_acc_y + g_base_acc_z * g_base_acc_z) * (g_acc_y * g_acc_y + g_acc_z * g_acc_z));
      if (norm > 1e-6)
        g_rel_ang_x = asin((g_base_acc_y * g_acc_z - g_base_acc_z * g_acc_y) / norm) / (M_PI / 2);
      else
        g_rel_ang_x = 0;
    }
    else
    {
      norm = sqrt((g_base_acc_y * g_base_acc_y + g_base_acc_x * g_base_acc_x) * (g_acc_y * g_acc_y + g_acc_x * g_acc_x));
      if (norm > 1e-6)
        g_rel_ang_x = -asin((g_base_acc_y * g_acc_x - g_base_acc_x * g_acc_y) / norm) / (M_PI / 2);
      else
        g_rel_ang_x = 0;
    }

    // CMRotationRate *rate = [[motion deviceMotion] rotationRate];
    CMRotationRate rate = [[motion gyroData] rotationRate];
    r_rate_x = rate.x * 180 / M_PI;
    r_rate_y = rate.y * 180 / M_PI;
    r_rate_z = rate.z * 180 / M_PI;
  } else {
    hid_ios::g_rel_ang_x = 0;
    hid_ios::g_rel_ang_y = 0;
    hid_ios::r_rate_x = 0;
    hid_ios::r_rate_y = 0;
    hid_ios::r_rate_z = 0;
  }
}
