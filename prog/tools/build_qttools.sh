#!/bin/bash
# Copyright (C) 2023-present WD Studios Corp. All Rights Reserved.
# Building Qt tools...

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

QT_PATH="${QT_DIR:-${QtDir}}"
QT_DIR_ARGS=""
if [ -n "$QT_PATH" ]; then
    QT_DIR_ARGS="-s QtDir=\"$QT_PATH\""
    echo "Using Qt from $QT_PATH"
fi

# BlkEditor
echo "Building BlkEditor..."
jam $QT_DIR_ARGS -s Root=../.. -f BlkEditor/Jamfile
if [ $? -ne 0 ]; then
    echo "Build failed."
    exit 1
fi

echo "Build completed successfully."
exit 0
