import os, argparse

parser = argparse.ArgumentParser()
parser.add_argument('--input')
parser.add_argument('--output')
parser.add_argument('--lib')
parser.add_argument('--domain')
parser.add_argument('--screenOrientation')
parser.add_argument('--targetSdk')
parser.add_argument('--minSdk')
parser.add_argument('--mainActivity')
parser.add_argument('--type')
parser.add_argument('--versionCode')
parser.add_argument('--versionName')
parser.add_argument('--stateNotNeed')
parser.add_argument('--vrSupport')
parser.add_argument('--gmsAppId')
parser.add_argument('--oculusVersion')
parser.add_argument('--applicationAttribures', nargs='+', default=[])
parser.add_argument('--userPermissions', nargs='+', default=[])


args = parser.parse_args()
args.ARG_SRC_XML = args.input
args.ARG_DST_XML = args.output
args.ARG_LIB = args.lib
args.ARG_PROJ_DOMAIN = args.domain
args.ARG_SCR_ORI = args.screenOrientation
args.ARG_TGT_SDK = args.targetSdk
args.ARG_MIN_SDK = args.minSdk
args.ARG_MAIN_ACT = args.mainActivity
args.ARG_TYPE = args.type
args.ARG_VER_CODE = args.versionCode
args.ARG_VER_NAME = args.versionName
args.ARG_STATE_NOT_NEED = args.stateNotNeed
args.ARG_VR_DEVICE_SUPPORT = args.vrSupport
args.GMS_APP_ID = args.gmsAppId
args.OCULUS_VERSION = args.oculusVersion
args.ANDROID_APP_ATTR = args.applicationAttribures


tmp = args.ANDROID_APP_ATTR
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

USER_PERMISSIONS = ''.join([f'<uses-permission android:name="{p}"/>' for p in args.userPermissions])

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
    ANDROID_ACTIVITIES=ANDROID_ACTIVITIES,
    USER_PERMISSIONS=USER_PERMISSIONS
    )

output = open(str(args.ARG_DST_XML), 'w+')
for i in range(len(content)):
  output.write(content[i])