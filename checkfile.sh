#!/bin/bash

if [ "$#" -ne 1 ]; then
  echo "checkfile.sh <FileCheck path>"
  exit 1
fi

echo "--- Start FileCheck ---"
set -e

for i in `find ./checkfile -name "*.ll"` ; do
  echo $i
  timeout 60s build/swpp-compiler $i .tmp.s
  $1 $i < .tmp.s
done

echo "--- Finished FileCheck ---"