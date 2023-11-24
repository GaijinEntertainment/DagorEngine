#!/bin/bash
cd prog/1stPartyLibs/quirrel/sqrat/
doxygen
cd ../../../../
if [ ! -d "$READTHEDOCS_OUTPUT/html" ]; then
  mkdir -p "$READTHEDOCS_OUTPUT/html"
fi
cp -R _docs/sqrat/html/* $READTHEDOCS_OUTPUT/html/
