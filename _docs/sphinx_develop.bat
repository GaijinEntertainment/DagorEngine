pushd ..
@start sphinx-autobuild --port 8080  -b html -d _docs/build/doctrees _docs/source _docs/build/html
@start python _docs/watch.py
popd
