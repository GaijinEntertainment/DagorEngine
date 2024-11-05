rem pushd ..
rem @start python _docs/watch.py
rem popd
@start sphinx-autobuild --port 8080  --watch source -b html -d build/doctrees . build/html
