# We use precisely the same cmd as RTD will use on remote to avoid problems
cd docs
python3 -m sphinx -b html -d _build/doctrees . ../build
echo ""
echo "====================================================================="
echo "Sphinx documentation built. Open ./build/index.html to see it."
