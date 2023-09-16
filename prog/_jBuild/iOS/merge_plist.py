import os, argparse, subprocess, sys

parser = argparse.ArgumentParser()
parser.add_argument('ARG_MAIN_PLIST')#1
parser.add_argument('ARG_ADD_PLISTS', nargs='*')#2

args = parser.parse_args()

print('Merging plist files into {}'.format(args.ARG_MAIN_PLIST))

if(not os.path.exists(args.ARG_MAIN_PLIST)):
  print('Main Plist file is not found - {}'.format(args.ARG_MAIN_PLIST))
  sys.exit(0)

addPlistCount = len(args.ARG_ADD_PLISTS)
for i in range(addPlistCount):
  if (not os.path.exists(args.ARG_MAIN_PLIST)):
    print('Additional Plist file is not found - {}'.format(args.ARG_ADD_PLISTS[i]))
    sys.exit(0)
  print('Merging additional Plist file - {}'.format(args.ARG_ADD_PLISTS[i]))
  cmd = '/usr/libexec/PlistBuddy -x -c "Merge {}" {}'.format(args.ARG_ADD_PLISTS[i],args.ARG_MAIN_PLIST)
  os.system(cmd)

