pushd .storage
"%GDEVTOOL%/python/python" ../mk_index.py
popd
pushd .storage-user
"%GDEVTOOL%/python/python" ../mk_index.py
popd
