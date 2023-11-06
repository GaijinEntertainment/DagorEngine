#!/bin/bash
set -o errexit
set -o nounset
set -o pipefail

if [[ "${TRACE-0}" == "1" ]]; then
    set -o xtrace;
fi

function error() {
    echo "" >&2
    echo "ERROR: $1" >&2
    echo "" >&2

    exit 1
}

function download_and_install_jam() {
    # Check first if we are using Ubuntu/Pop!_OS. Let's support
    # only Ubuntu for now. Contributions welcome.
    . /etc/os-release
    OS=$ID
    OS_LIKE=$ID_LIKE

    if [[ $OS != *"ubuntu"* ]] && [[ $OS_LIKE != *"ubuntu"* ]]; then
        error "Script does not support non-Ubuntu distributions yet."
    fi

    wget -q "https://github.com/GaijinEntertainment/jam-G8/releases/download/2.5-G8-1.2-2023%2F05%2F04/jam-ubuntu-20.04-x64.tar.gz"
    tar -xvf "jam-ubuntu-20.04-x64.tar.gz" > /dev/null
    sudo mv jam /usr/local/bin/
    rm "jam-ubuntu-20.04-x64.tar.gz"
}

## Get the destination directory for dev tools.
# Check the number of arguments. Must only be one,
# which is the dev tools directory.
if [[ "$#" != "1" ]]; then
    echo "" >&2
    echo "Usage: ./make_devtools.sh DEVTOOLS_DEST_DIR" >&2
    echo "example: ./make_devtools.sh /path/to/devtools" >&2
    echo "" >&2

    exit 1
fi

dest_dir=$(echo "$1" | sed 's:/*$::')

if [[ $dest_dir = *" "* ]]; then
    error "The destination directory contains spaces, which are not allowed in a file path."
fi

if [[ $dest_dir = *[![:ascii:]]* ]]; then
    error "The destination directory contains non-ASCII characters."
fi

if [[ $dest_dir != "/"* ]]; then
    error "The destination directory must be an absolute path."
fi

## Setup jam.
echo "[=] Setting up jam..."
echo -n "- Checking if jam is already installed... "
if [[ $(command -v jam) > /dev/null ]]; then
    echo "✅"
else
    echo "❌"
    echo -n "- Downloading jam since it's not installed... "

    download_and_install_jam

    echo "✅"
    echo "- Finished downloading and installing jam."

    echo -n "- Testing jam if it works... "
    if [[ $(command -v jam) > /dev/null ]]; then
        echo "✅"
        echo "- Jam works!"
    else
        echo "❌"
        error "Unable to run jam. Please check your system and/or PATH for conflicts with jam."
    fi
fi
