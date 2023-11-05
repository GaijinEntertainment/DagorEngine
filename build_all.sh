#!/bin/bash
set -o errexit
set -o nounset
set -o pipefail

if [[ "${TRACE-0}" == "1" ]]; then
    set -o xtrace;
fi

PWD="$(pwd)"

cd prog/tools
echo '[WIP] Build Dagor tools...'
./build_dagor3_cdk_mini.sh
cd $PWD

echo '[NOT PORTED] Build Dargbox vfsroms and shaders...'
echo '[NOT PORTED] Build Physics (Jolt) Test...'
echo '[NOT PORTED] Build Skies sample...'
echo '[NOT PORTED] Build GI Test sample...'
