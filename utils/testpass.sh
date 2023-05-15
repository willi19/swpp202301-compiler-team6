#!/usr/bin/env bash

set -Eeu

pass_name=$2
pass_plugin=${3:-}

find "tests/FileCheck/$pass_name" -iname '*.ll' | while IFS=$'\n' read -r testfile; do
	echo "testing $testfile"
	if [[ -n $pass_plugin ]]; then
		"$1"/opt --load-pass-plugin="$pass_plugin" --passes="$pass_name" -S "$testfile"
	else
		"$1"/opt --passes="$pass_name" -S "$testfile"
	fi | "$1"/FileCheck "$testfile"
done
