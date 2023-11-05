#!/bin/bash
set -o errexit
set -o nounset
set -o pipefail

if [[ "${TRACE-0}" == "1" ]]; then
    set -o xtrace;
fi

## Install jam.
echo "[-] Downloading jam..."
echo "WARNING! At the moment, we may cause issues if you pre-installed jam."

# Check first if we are using Ubuntu/Pop!_OS. Let's support
# only Ubuntu for now. Contributions welcome.
. /etc/os-release
OS=$ID
OS_LIKE=$ID_LIKE

if [[ $OS != *"ubuntu"* ]] && [[ $OS_LIKE != *"ubuntu"* ]]; then
    echo "Error: Script does not support non-Ubuntu distributions yet."
    exit 1
fi

wget -q --show-progress "https://github.com/GaijinEntertainment/jam-G8/releases/download/2.5-G8-1.2-2023%2F05%2F04/jam-ubuntu-20.04-x64.tar.gz"
tar -xvf "jam-ubuntu-20.04-x64.tar.gz"
sudo mv jam /usr/local/bin/
rm "jam-ubuntu-20.04-x64.tar.gz"

echo "Finished downloading jam."
