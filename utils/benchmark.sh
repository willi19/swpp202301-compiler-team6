#!/usr/bin/env bash

set -Eeu

compiler=$1
interp=$2
bmdir=$3
logdir=${4:-logs}

# (cd "$bmdir" && "$bmdir"/build-asms.py "$compiler")

mkdir -p "$logdir"

for asmfile in "$bmdir"/*/src/*.s; do
	testname=${asmfile##*/}
	testname=${testname%%.s}
	echo "testing $testname"
	mkdir -p "$logdir/$testname"
	for infile in "${asmfile%%/src*}"/test/input*.txt; do
		inname=${infile##*/}
		inname=${inname%%.txt}
		outname=output${inname##input}
		echo "$inname"
		diff <(cat "$infile" | "$interp" "$asmfile") "${asmfile%%/src*}/test/$outname.txt"
		mv swpp-interpreter.log "$logdir/$testname/$inname.log"
		mv swpp-interpreter-cost.log "$logdir/$testname/$inname-cost.log"
		mv swpp-interpreter-inst.log "$logdir/$testname/$inname-inst.log"
	done
	cat "$logdir/$testname"/input*-cost.log | \
		awk -F: '/^main/ {cost+=$2; cnt++} END {print "'"$testname"' average cost:", int(cost/cnt)}'
done
