import os
import sys
import zipfile

if len(sys.argv) < 2:
    print(f'Usage: script output_zip_name [directives...]', file=sys.stderr)
    sys.exit(1)

with zipfile.ZipFile(sys.argv[1], 'w', zipfile.ZIP_DEFLATED) as zf:
    for directive in sys.argv[2:]:
        directive.replace('\\', '/')
        if 'f:' == directive[:2]:
            zf.write(directive[2:], arcname=os.path.basename(directive[2:]))
        elif 'd:' == directive[:2]:
            for root, dirs, files in os.walk(directive[2:]):
                for file in files:
                    full_path = os.path.join(root, file)
                    arc_path = os.path.join(os.path.basename(root), file)
                    zf.write(full_path, arcname=arc_path)
        else:
            print(f'Invalid directive "{directive}". Must start with f: for files or d: for directives', file=sys.stderr)
            sys.exit(1)
