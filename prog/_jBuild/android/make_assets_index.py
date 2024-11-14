import os
import argparse
import fnmatch

parser = argparse.ArgumentParser()
parser.add_argument('--dir', required=True)
parser.add_argument('--output')
parser.add_argument('--masks', required=True, nargs='+', default=[])

args = parser.parse_args()


def get_files(folder_path, masks):
  result = []

  folder_path = os.path.normpath(folder_path)
  for root, dirs, files in os.walk(folder_path):
    for file in files:
      fullpath = os.path.normpath(os.path.join(root, file))
      relpath = os.path.relpath(fullpath, folder_path).replace('\\', '/')

      for mask in masks:
        if fnmatch.fnmatch(relpath, mask):
          result.append(relpath)
          break

  return result


print(f'Make assets index: {args.masks} -> {args.output}')

indexBlk = ''
for f in get_files(args.dir, args.masks):
  indexBlk += f'\nasset:t="{f}"'

if args.output:
  with open(args.output, 'w') as f:
    f.write(indexBlk)
else:
  print(indexBlk)