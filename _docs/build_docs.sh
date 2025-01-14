PATH=$PATH:$GDEVTOOL/python3
PATH=$PATH:"/c/Program Files/doxygen/bin"

source .venv/Scripts/activate
echo "virtualenv activated"


sphinx-build -c . -b html source _build