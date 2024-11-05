import os
import sys

def find_proguard_files(folder):
  return [f for f in os.listdir(folder) if f.endswith('.proguard')]

def insert_proguard_files(build_gradle_path, proguard_files):
  with open(build_gradle_path, 'r') as file:
    lines = file.readlines()

  new_lines = []
  for line in lines:
    new_lines.append(line)
    if 'minifyEnabled true' in line:
      proguard_files_line = '            proguardFiles getDefaultProguardFile("proguard-android-optimize.txt")'
      for proguard_file in proguard_files:
        proguard_files_line += f', "{proguard_file}"'
      new_lines.append(proguard_files_line + '\n')

  with open(build_gradle_path, 'w') as file:
    file.writelines(new_lines)

if __name__ == "__main__":
  if len(sys.argv) != 2:
    print("Usage: python make_proguard_gradle.py <folder>")
    sys.exit(0)

  folder = sys.argv[1]
  proguard_files = find_proguard_files(folder)
  if proguard_files == []:
    print("No proguard files found.")
    sys.exit(0)

  build_gradle_path = os.path.join(folder, 'build.gradle')

  if not os.path.exists(build_gradle_path):
    print(f"Error: Gradle file {build_gradle_path} does not exist.")
    sys.exit(0)

  insert_proguard_files(build_gradle_path, proguard_files)
  print("Proguard files have been inserted into build.gradle.")