#!/bin/bash

if [ "$#" -ne 1 ]; then
  echo "checkfile.sh <FileCheck path>"
  exit 1
fi

echo "--- Start FileCheck ---"
set -e

for i in `find ./checkfile/simplifycfg -name "*.ll"` ; do
  echo $i
  timeout 60s build/swpp-compiler $i .tmp.s --verbose > .verbose
  python3 extract_optimized.py .verbose .tmp.ll
  $1 $i < .tmp.ll
done

echo "--- Finished FileCheck ---"
