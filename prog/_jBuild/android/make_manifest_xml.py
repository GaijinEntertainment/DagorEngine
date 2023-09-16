import os, argparse

parser = argparse.ArgumentParser()
parser.add_argument('ARG_SRC_XML')#1
parser.add_argument('ARG_DST_XML')#2
parser.add_argument('ARG_LIB')#3
parser.add_argument('ARG_PROJ_DOMAIN')#4
parser.add_argument('ARG_SCR_ORI')#5
parser.add_argument('ARG_TGT_SDK')#6
parser.add_argument('ARG_MIN_SDK')#7
parser.add_argument('ARG_MAIN_ACT')#8
parser.add_argument('ARG_TYPE')#9
parser.add_argument('ARG_VER_CODE')#10
parser.add_argument('ARG_VER_NAME')#11
parser.add_argument('ARG_STATE_NOT_NEED')#12
parser.add_argument('ARG_VR_DEVICE_SUPPORT')#13
parser.add_argument('GMS_APP_ID')#14
parser.add_argument('OCULUS_VERSION')#15
parser.add_argument('ANDROID_APP_ATTR', nargs='+')#16

args = parser.parse_args()

tmp = args.ANDROID_APP_ATTR[0].split(' ')
for i in range(len(tmp)):
  if(tmp[i] == ''):
    continue
  tmp[i] = tmp[i].split('=')[0] + '=\"' + tmp[i].split('=')[1] + '\"'

ANDROID_APP_ATTR = ' '.join(tmp)


if args.GMS_APP_ID != '':
  args.GMS_APP_ID = (
    """<meta-data android:name=\"com.google.android.gms.games.APP_ID\" android:value=\"@string/app_id\" />
      <meta-data android:name=\"com.google.android.gms.version\" android:value=\"@integer/google_play_services_version\"/>"""
  ).format(args.GMS_APP_ID)

if args.OCULUS_VERSION != 'none':
  args.OCULUS_VERSION = (
    """<meta-data android:name=\"com.oculus.supportedDevices\" android:value=\"{}\" />"""
  ).format(args.OCULUS_VERSION)
else:
  args.OCULUS_VERSION = ''

if args.ARG_VR_DEVICE_SUPPORT == 'yes':
  args.ARG_VR_DEVICE_SUPPORT = '\n<meta-data android:name=\"com.oculus.intent.category.VR\" android:value=\"vr_only\"/>'
else:
  args.ARG_VR_DEVICE_SUPPORT = ''

ARG_TYPE_TV_NAME = ''
ARG_TYPE_TV_BANNER = ''

if args.ARG_TYPE == "android-TV":
  args.ARG_TYPE_TV_NAME = '\n<uses-feature android:name=\"android.software.leanback\" android:required=\"false\" />'
  args.ARG_TYPE_TV_BANNER = '\nandroid:banner=\"@drawable/banner\"'

if args.ARG_MAIN_ACT == ".none":
  args.ARG_MAIN_ACT = 'android:name=\"android.app.NativeActivity\"'
else:
  args.ARG_MAIN_ACT = 'android:name=\"{}\"'.format(args.ARG_MAIN_ACT)

ANDROID_ACTIVITIES = ''
for f in os.listdir(os.path.dirname(args.ARG_DST_XML)):
  if f.endswith('.activity.xml'):
    full_path = os.path.join(os.path.dirname(args.ARG_DST_XML), f)
    ANDROID_ACTIVITIES += '\n' + open(full_path, 'r').read()


origin = open(args.ARG_SRC_XML, 'r')
content = origin.readlines()

for i in range(len(content)):
  content[i] = content[i].format(
    ARG_LIB=args.ARG_LIB,
    ARG_PROJ_DOMAIN=args.ARG_PROJ_DOMAIN,
    ARG_SCR_ORI=args.ARG_SCR_ORI,
    ARG_TGT_SDK=args.ARG_TGT_SDK,
    ARG_MIN_SDK=args.ARG_MIN_SDK,
    ARG_MAIN_ACT=args.ARG_MAIN_ACT,
    ARG_TYPE_TV_NAME=ARG_TYPE_TV_NAME,
    ARG_TYPE_TV_BANNER=ARG_TYPE_TV_BANNER,
    ARG_VER_NAME=args.ARG_VER_NAME,
    ARG_VER_CODE=args.ARG_VER_CODE,
    ARG_STATE_NOT_NEED=args.ARG_STATE_NOT_NEED,
    ARG_VR_DEVICE_SUPPORT=args.ARG_VR_DEVICE_SUPPORT,
    ANDROID_APP_ATTR=ANDROID_APP_ATTR,
    GMS_APP_ID=args.GMS_APP_ID,
    OCULUS_VERSION=args.OCULUS_VERSION,
    ANDROID_ACTIVITIES=ANDROID_ACTIVITIES
    )

output = open(str(args.ARG_DST_XML), 'w+')
for i in range(len(content)):
  output.write(content[i])