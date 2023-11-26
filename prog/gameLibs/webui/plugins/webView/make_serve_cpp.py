import sys
import os

f = open(sys.argv[1], 'wt')

srcFiles = sys.argv[2:]
f.write('#include <webui/httpserver.h>\n')
f.write('#include <webui/helpers.h>\n')
f.write('\n')

funtionTemplate = """static void serve_file_{name}(webui::RequestInfo *params) {{
  static uint8_t inlinedData[] = {{
#include "{name}.inl"
  }};
  webui::html_response_raw(params->conn, "{mime}", (const char *)inlinedData, sizeof(inlinedData) - 1);
}}
"""

extToMimeType = {
  '.html': 'text/html',
  '.js': 'text/javascript',
  '.css': 'text/css',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.woff2': 'font/woff2',
  '.ttf': 'font/ttf',
  '.ico': 'image/x-icon',
}

rootDir = os.path.dirname(__file__)
dstDir = os.path.abspath(os.path.dirname(sys.argv[1]))
stringifyPy = os.path.normpath(os.path.join(rootDir, '..', '..', '..', '..', '_jBuild', '_scripts', 'stringify.py'))

for src in srcFiles:
  name = os.path.basename(src).replace('.', '_').replace('-', '_')
  path = os.path.join(rootDir, src)
  ext = os.path.splitext(src)[1]
  mime = extToMimeType.get(ext, 'text/plain')
  os.system(f'{sys.executable} {stringifyPy} --array {path} {dstDir}/{name}.inl')
  f.write(funtionTemplate.format(**locals()))

f.write('webui::HttpPlugin webui::webview_files_http_plugins[] = {\n')
for src in srcFiles:
  name = os.path.basename(src).replace('.', '_').replace('-', '_')
  f.write('  {{ "{0}", NULL, NULL, serve_file_{1} }},\n'.format(src, name))
f.write('  {NULL}\n')
f.write('};\n')

f.close()
