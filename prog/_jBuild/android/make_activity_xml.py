import os, argparse
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--input')
parser.add_argument('--output')
parser.add_argument('--outdir')
parser.add_argument('--ARG_LIB')
parser.add_argument('--ARG_SCR_ORI')
parser.add_argument('--ARG_MAIN_ACT')
parser.add_argument('--ARG_TYPE')
parser.add_argument('--ARG_STATE_NOT_NEED')
parser.add_argument('--ARG_ACTION')
parser.add_argument('--ARG_THEME')

args = parser.parse_args()

if args.ARG_ACTION and args.ARG_ACTION != 'none':
  args.ARG_ACTION = """<intent-filter>
    <action android:name="android.intent.action.MAIN" />
    <category android:name="android.intent.category.LAUNCHER" />
    <category android:name="android.intent.category.LEANBACK_LAUNCHER" />
  </intent-filter>"""
else:
  args.ARG_ACTION = ''

ARG_TYPE_TV_BANNER = ''

if args.ARG_TYPE == "android-TV":
  args.ARG_TYPE_TV_BANNER = '\nandroid:banner=\"@drawable/banner\"'

if args.ARG_MAIN_ACT == ".none":
  args.ARG_MAIN_ACT = 'android:name=\"android.app.NativeActivity\"'
else:
  args.ARG_MAIN_ACT = 'android:name=\"{}\"'.format(args.ARG_MAIN_ACT)

origin = open(args.input, 'r')
content = origin.readlines()

for i in range(len(content)):
  content[i] = content[i].format(
    ARG_LIB=args.ARG_LIB,
    ARG_SCR_ORI=args.ARG_SCR_ORI,
    ARG_MAIN_ACT=args.ARG_MAIN_ACT,
    ARG_TYPE_TV_BANNER=ARG_TYPE_TV_BANNER,
    ARG_STATE_NOT_NEED=args.ARG_STATE_NOT_NEED,
    ARG_ACTION=args.ARG_ACTION,
    ARG_THEME=args.ARG_THEME
    )

output = open(os.path.join(str(args.outdir), str(args.output)), 'w+')
for i in range(len(content)):
  output.write(content[i])