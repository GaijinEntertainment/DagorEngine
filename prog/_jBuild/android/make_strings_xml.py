import os, argparse

parser = argparse.ArgumentParser()
parser.add_argument('ARG_TARGET_STRINGS_XML')#1
parser.add_argument('ARG_APP_ICON_NAME')#2

args = parser.parse_args()

if args.ARG_APP_ICON_NAME == '':
  args.ARG_APP_ICON_NAME = 'Sample Dagor'

local_folder = os.path.dirname(os.path.realpath(__file__))
origin = open(os.path.join(local_folder, "strings.template"), 'r')
content = origin.readlines()

for i in range(len(content)):
  content[i] = content[i].format(ARG_APP_ICON_NAME=args.ARG_APP_ICON_NAME)

target_folder = os.path.dirname(os.path.abspath(args.ARG_TARGET_STRINGS_XML))

if not os.path.exists(target_folder):
  os.makedirs(target_folder)

output = open(str(args.ARG_TARGET_STRINGS_XML), 'w+')
for i in range(len(content)):
  output.write(content[i])

