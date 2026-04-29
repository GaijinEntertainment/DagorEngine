import os
import argparse
import fnmatch

def find_files(directory, pattern):
    for root, dirs, files in os.walk(directory):
        for basename in files:
            if fnmatch.fnmatch(basename, pattern):
                yield os.path.join(root, basename)

parser = argparse.ArgumentParser(description='Java source preprocessor - replaces {DEFINE} placeholders in .java files')
parser.add_argument('PATH', help='Directory to search for .java files')
parser.add_argument('--define', metavar='KEY=VALUE', action='append', default=[],
                    help='Replace {KEY} with VALUE (can be repeated)')
parser.add_argument('--external-libs', nargs='*', default=None, metavar='LIB',
                    help='Generate System.loadLibrary() calls for {EXTERNAL_LIBS}')
parser.add_argument('--debug', default=None,
                    help='Set {DEBUG} to true/false (pass yes or no)')

args = parser.parse_args()

defines = {}

# Handle --define KEY=VALUE pairs
for d in args.define:
    if '=' in d:
        key, value = d.split('=', 1)
        defines[key] = value
        print('Injecting {} = {}'.format(key, value))
    else:
        print('Warning: ignoring malformed --define {!r} (expected KEY=VALUE)'.format(d))

# Handle --external-libs: generates System.loadLibrary() code for {EXTERNAL_LIBS}
if args.external_libs is not None:
    libs = [lib for lib in args.external_libs if lib and lib != 'no']
    if libs:
        print('replacing EXTERNAL_LIBS in java files')
        load_libs = '\n  try { \n'
        for lib in libs:
            load_libs += '    System.loadLibrary("{}");\n'.format(lib)
        load_libs += '  } catch(java.lang.UnsatisfiedLinkError e) { }\n'
        defines['EXTERNAL_LIBS'] = load_libs
    else:
        defines['EXTERNAL_LIBS'] = ''

# Handle --debug: maps yes/no to true/false for {DEBUG}
if args.debug is not None:
    defines['DEBUG'] = 'false' if args.debug in ('', 'no') else 'true'

if not defines:
    print('No defines specified, nothing to do')
else:
    for filename in find_files(args.PATH, '*.java'):
        with open(filename, 'r') as f:
            content = f.readlines()

        for i in range(len(content)):
            line = content[i]
            for key, value in defines.items():
                new_line = line.replace('{' + key + '}', value)
                if new_line != line:
                    print('Found {} in FILE {}'.format(key, filename))
                    line = new_line
            content[i] = line

        with open(filename, 'w+') as f:
            f.writelines(content)
