#!/bin/bash

echo "--- Start FileCheck ---"
set -e

FILECHECK_PATH = /opt/llvm/bin/FileCheck

for i in `find ./filechecks -name "*.ll"` ; do
  timeout 60s build/swpp-compiler $i | $FILECHECK_PATH $i
done

echo "--- Finished FileCheck ---"