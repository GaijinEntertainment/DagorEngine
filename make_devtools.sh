#!/bin/bash
set -o errexit
set -o nounset
set -o pipefail

if [[ "${TRACE-0}" == "1" ]]; then
    set -o xtrace;
fi

# Bash cannot really return values. This variable is used as a
# workaround for Bash's limitations. In the long term, we should
# make `make_devtools.py` cross-platform. Note that we're just
# separating the scripts for now to ease in the porting efforts
# to Linux.
ask_return_value=0

function ask() {
    local question=$1

    for (( ; ; ))
    do
        echo ""
        read -p "$question [Y/n]: " answer
        answer="${answer,,}"

        if [[ $answer == "y" ]] || [[ $answer == "yes" ]] || [[ $answer == "" ]]; then
            ask_return_value=1
            return
        elif [[ $answer == "n" ]] || [[ $answer == "no" ]]; then
            ask_return_value=0
            return
        fi
    done
}

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

function obtain_dxc_sources() {
    local packages_dir=$1
    local dxc_version=$2

    cd $packages_dir

    git clone "https://github.com/microsoft/DirectXShaderCompiler.git"
    cd "DirectXShaderCompiler"

    git checkout "v$dxc_version"
    git submodule init
    git submodule update

    cd ../.. # Go back to previous working directory.

    local src_dir="$packages_dir/DirectXShaderCompiler"
    echo $src_dir
}

function compile_dxc_sources() {
    local dxc_src_dir=$1

    cd "$dxc_src_dir"

    mkdir -p "build/"
    cd "build/"

    cmake ".." -GNinja -DCMAKE_BUILD_TYPE=Release -C "../cmake/caches/PredefinedParams.cmake"
    ninja

    cd ../..
}

function setup_dxc_dest_dir() {
    local dxc_dest_dir=$1

    mkdir -p "$dxc_dest_dir" "$dxc_dest_dir/include" "$dxc_dest_dir/lib/linux64"
}

function extract_dxc_assets() {
    local dxc_tools_dest_dir=$1
    local dxc_src_dir=$2

    cd "$dxc_src_dir"

    ln -s "$dxc_src_dir/include/dxc" "$dxc_tools_dest_dir/include"

    cp "$dxc_src_dir/build/lib/libdxcompiler.so.3.7" "$dxc_tools_dest_dir/lib/linux64/libdxcompiler.so"
    cp "$dxc_src_dir/LICENSE.TXT" "$dxc_tools_dest_dir/LICENSE.TXT"

    cd ../..
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

mkdir -p dest_dir

packages_dir="$dest_dir/.packages"
mkdir -p "$packages_dir"

normalized_dest_dir="$(realpath $dest_dir)"

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

## Setup DXC.
DXC_VERSION=1.7.2207
dxc_dest_folder="$dest_dir/DXC-$DXC_VERSION"
echo "[=] Setting up DirectX Shader Compiler $DXC_VERSION (July 2022)..."
echo -n "- Checking if DXC $DXC_VERSION is already set up... "
if [[ -d $dxc_dest_folder ]]; then
    echo "✅"
else
    echo "❌"
    echo "- Will compile DirectX Shader Compiler from sources since Microsoft did not provide \
prebuilt packages for Linux for v$DXC_VERSION..."
    echo "- Downloading and extracting sources..."
    dxc_sources_dir=$(obtain_dxc_sources $packages_dir $DXC_VERSION)

    echo "- Compiling sources..."
    compile_dxc_sources $dxc_sources_dir

    echo "- Extracting compiled sources..."
    setup_dxc_dest_dir $dxc_dest_folder
    extract_dxc_assets $dxc_dest_folder $dxc_sources_dir

    echo "- Finished downloading and setting up DirectX Shader Compiler $DXC_VERSION (July 2022)."
fi


## Setup GDEVTOOL environment variable.
# Setting the environment variable is dependent on the shell being used.
if [[ $SHELL == "/usr/bin/bash" ]]; then
    env_file="$HOME/.bash_profile"
elif [[ $SHELL == "/usr/bin/zsh" ]]; then
    env_file="$HOME/.zshenv"
else
    error "Script does not support $SHELL yet."
fi

is_env_updated=0
if [[ -z "${GDEVTOOL:-}" ]]; then
    ask "Environment variable 'GDEVTOOL' not found. Do you want to set it?"
    if [[ $ask_return_value == "1" ]]; then
        echo -n "- Setting 'GDEVTOOL'... "
        echo "export GDEVTOOL=$normalized_dest_dir" >> $env_file
        echo "✅"

        is_env_updated=1
    fi
elif [[ $GDEVTOOL != $normalized_dest_dir ]]; then
    ask "Environment variable 'GDEVTOOL' points to another directory. Do you want to update it?"
    if [[ $ask_return_value == "1" ]]; then
        echo -n "- Updating 'GDEVTOOL' to $normalized_dest_dir... "
        echo "export GDEVTOOL=\"$normalized_dest_dir\"" >> $env_file
        echo "✅"

        is_env_updated=1
    fi
fi

if [[ ":$PATH:" != *":$normalized_dest_dir:"* ]]; then
    ask "'$normalized_dest_dir' is not found in 'PATH' variable. Do you want to add it?"
    if [[ $ask_return_value == "1" ]]; then
        echo -n "- Adding $normalized_dest_dir to PATH... "
        echo "export PATH=\"$PATH:$normalized_dest_dir\"" >> $env_file
        echo "✅"

        is_env_updated=1
    fi
fi

if [[ $is_env_updated == "1" ]]; then
    echo ""
    echo "Done. Please restart your command-line environment to apply the environment variables."
else
    echo ""
    echo "Done."
fi
