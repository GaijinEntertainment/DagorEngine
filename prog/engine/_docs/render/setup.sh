python3 -m pip install -U virtualenv setuptools
echo "virtualenv installed"
python3 -m venv .venv
echo "virtualenv created"
source .venv/Scripts/activate
echo "virtualenv activated"
python3 -m pip install sphinx
echo "sphinx installed"
python3 -m pip install --exists-action=w --no-cache-dir -r requirements.txt
echo "requirements.txt installed"
