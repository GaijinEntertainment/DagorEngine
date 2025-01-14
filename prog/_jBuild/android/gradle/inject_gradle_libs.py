import os
import sys

def find_libs_files(folder):
  return [os.path.join(folder, f) for f in os.listdir(folder) if f.endswith('.libs')]

def insert_libs_files(build_gradle_path, libs_files):
  with open(build_gradle_path, 'r') as file:
    lines = file.readlines()

  dependencies_index = -1
  for i, line in enumerate(lines):
    if 'dependencies {' in line:
      dependencies_index = i
      break

  if dependencies_index == -1:
    print("Error: 'dependencies' section not found in build.gradle.")
    return

  existing_lines = set(lines)

  new_lines = []
  for lib_file in libs_files:
    with open(lib_file, 'r') as file:
      for line in file:
        if line.strip() and line not in existing_lines:
          print(f"inserting {line.strip()} to project.gradle")
          new_lines.append("    implementation '"+ line.strip() + "'\n")
          existing_lines.add(line)

  lines[dependencies_index + 1:dependencies_index + 1] = new_lines

  with open(build_gradle_path, 'w') as file:
    file.writelines(lines)

if __name__ == "__main__":
  if len(sys.argv) != 2:
    print("Usage: python inject_gradle_libs.py <folder>")
    sys.exit(0)

  folder = sys.argv[1]
  libs_files = find_libs_files(folder)
  if libs_files == []:
    print("No libs files found.")
    sys.exit(0)

  project_gradle_path = os.path.join(folder, 'project.gradle')

  if not os.path.exists(project_gradle_path):
    print(f"Error: Gradle file {project_gradle_path} does not exist.")
    sys.exit(0)

  insert_libs_files(project_gradle_path, libs_files)
