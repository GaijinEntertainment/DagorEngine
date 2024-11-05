import os, argparse
import zipfile

parser = argparse.ArgumentParser()
parser.add_argument('JAVA_SRC_DIR')#1
parser.add_argument('ZIP_DST')#2

args = parser.parse_args()


def zip_folder(folder_path, output_path):
  with zipfile.ZipFile(output_path, 'w') as zipf:
    for root, dirs, files in os.walk(folder_path):
      for file in files:
        zipf.write(os.path.join(root, file), os.path.relpath(os.path.join(root, file), folder_path))


print(f'Zip Java sources: {args.JAVA_SRC_DIR} -> {args.ZIP_DST}')
zip_folder(args.JAVA_SRC_DIR, args.ZIP_DST)
