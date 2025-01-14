import argparse
import os

parser = argparse.ArgumentParser()
parser.add_argument('TARGET_DIR')#1
parser.add_argument('SDK_DIR')#2
parser.add_argument('APK_NAMESPACE')#3
parser.add_argument('PROJECT_FILE')#4
parser.add_argument('VERSION_CODE')#5
parser.add_argument('VERSION_NAME')#6
parser.add_argument('MIN_SDK')#7
parser.add_argument('TARGET_SDK')#8
parser.add_argument('VARS', nargs='*', default=None)#other

args = parser.parse_args()
STORE_FILE = ''
KEY_ALIAS = ''
KEY_PASSWORD = ''
if(len(args.VARS) > 0):
  STORE_FILE = args.VARS[0]
if(len(args.VARS) > 1):
  KEY_ALIAS = args.VARS[1]
if(len(args.VARS) > 2):
  KEY_PASSWORD = args.VARS[2]

if args.PROJECT_FILE == "none":
  args.PROJECT_FILE=''
else:
  args.PROJECT_FILE= '\nPROJECT_FILE=' + args.PROJECT_FILE

main_prop = """\
NAMESPACE={APK_NAMESPACE}
VERSION_CODE={VERSION_CODE}
VERSION_NAME={VERSION_NAME}
MIN_SDK={MIN_SDK}
TARGET_SDK={TARGET_SDK}
PROJECT_FILE={PROJECT_FILE}
org.gradle.jvmargs=-Xmx8192m
android.useAndroidX=true
android.enableJetifier=true
""".format(
  APK_NAMESPACE=args.APK_NAMESPACE,
  PROJECT_FILE=args.PROJECT_FILE,
  VERSION_CODE=args.VERSION_CODE,
  VERSION_NAME=args.VERSION_NAME,
  MIN_SDK=args.MIN_SDK,
  TARGET_SDK=args.TARGET_SDK
  )

build_dep_prop = ''
if len(args.VARS) == 3:
  build_dep_prop = """\
RELEASE_SIGN=true
RELEASE_STORE_FILE={STORE_FILE}
RELEASE_KEY_ALIAS={KEY_ALIAS}
RELEASE_STORE_PASSWORD={KEY_PASSWORD}
RELEASE_KEY_PASSWORD={KEY_PASSWORD}\n""".format(
  STORE_FILE=STORE_FILE,
  KEY_ALIAS=KEY_ALIAS,
  KEY_PASSWORD=KEY_PASSWORD
  )
else:
  build_dep_prop = """\
STORE_FILE={STORE_FILE}
KEY_ALIAS={KEY_ALIAS}
KEY_PASSWORD={KEY_PASSWORD}
RELEASE_SIGN=false""".format(
  STORE_FILE=STORE_FILE,
  KEY_ALIAS=KEY_ALIAS,
  KEY_PASSWORD=KEY_PASSWORD
  )

build_new_prop = """\
android.defaults.buildfeatures.buildconfig=true
android.nonTransitiveRClass=false
android.suppressUnsupportedCompileSdk=35
android.nonFinalResIds=false\n"""

gradle_properties_output = main_prop + build_dep_prop + build_new_prop

local_properties = open(os.path.join(args.TARGET_DIR, 'local.properties'), 'w')
gradle_settings = open(os.path.join(args.TARGET_DIR, 'settings.gradle'), 'w')
gradle_properties = open(os.path.join(args.TARGET_DIR, 'gradle.properties'), 'w')

gradle_settings.write('// empty')
local_properties.write('sdk.dir={}'.format(args.SDK_DIR))
gradle_properties.write(gradle_properties_output)
