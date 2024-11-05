PATH=$PATH:$GDEVTOOL/python3
PATH=$PATH:"/c/Program Files/doxygen/bin"

source .venv/Scripts/activate
echo "virtualenv activated"


python3 -m sphinx -b html -d _build/doctrees . build