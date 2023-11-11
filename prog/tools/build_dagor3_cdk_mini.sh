#!/bin/bash
set -o errexit
set -o nounset
set -o pipefail

if [[ "${TRACE-0}" == "1" ]]; then
    set -o xtrace
fi

echo "Configure some build-related scripts..."
chmod +x ../_jBuild/linux64/merge_ar

echo "[NOT PORTED] Build DaEditorX..."
echo "[NOT PORTED] Build dabuild..."
echo "[NOT PORTED] Build AssetViewer..."
echo "[NOT PORTED] Build daImpostorBaker..."
echo "[NOT PORTED] Build DDsx plugins..."

echo "[WIP] Build shader compilers..."
jam -s Root=../.. -f ../3rdPartyLibs/legacy_parser/dolphin/jamfile
jam -s Root=../.. -f ../3rdPartyLibs/legacy_parser/whale/jamfile
jam -s Root=../.. -f ShaderCompiler2/jamfile-hlsl2spirv

echo "[NOT PORTED] Build utils..."
echo "[NOT PORTED] Build GUI tools..."
echo "[NOT PORTED] Build 3ds Max plugins..."
