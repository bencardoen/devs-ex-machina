#!/bin/bash

###############################################################################
# Generic build script.
#
# This file is part of the DEVS Ex Machina project.
# Copyright 2014 - 2015 University of Antwerp
# https://www.uantwerpen.be/en/
# Licensed under the EUPL V.1.1
# A full copy of the license is in COPYING.txt, or can be found at
# https://joinup.ec.europa.eu/community/eupl/og_page/eupl
#      Author: Stijn Manhaeve, Ben Cardoen
#
# Given sourcecode & CMake , runs CMake with preconfigured options.
###############################################################################

OUT_DIR="./bmarkdata/pdevs"
EXEC_NORM="./binary/nonpdevs"
EXEC_1THREADSLEEP="./binary/pdevs1"
EXEC_4THREAD="./binary/pdevs4"
EXEC_16THREAD="./binary/pdevs16"
EXEC_4THREADSLEEP="./binary/pdevs4sleep"
EXEC_16THREADSLEEP="./binary/pdevs16sleep"
BMARKDIR="./build/Benchmark"

mkdir -p "./binary"

if [ ! -f "$EXEC_NORM" ]; then
	if [ -d "$BMARKDIR" ]; then
		rm -r "$BMARKDIR"
	fi
	./setup.sh -b dxexmachina_devstone
	cp build/Benchmark/dxexmachina_devstone $EXEC_NORM
fi

if [ ! -f "$EXEC_4THREAD" ]; then
	if [ -d "$BMARKDIR" ]; then
		rm -r "$BMARKDIR"
	fi
	./setup.sh -x "-DPDEVS=ON -DPDEVS_THREADS=4 -DPDEVS_LOAD=0" -b dxexmachina_devstone
	cp build/Benchmark/dxexmachina_devstone $EXEC_4THREAD
fi

if [ ! -f "$EXEC_16THREAD" ]; then
	if [ -d "$BMARKDIR" ]; then
		rm -r "$BMARKDIR"
	fi
	./setup.sh -x "-DPDEVS=ON -DPDEVS_THREADS=16 -DPDEVS_LOAD=0" -b dxexmachina_devstone
	cp build/Benchmark/dxexmachina_devstone $EXEC_16THREAD
fi

if [ ! -f "$EXEC_1THREADSLEEP" ]; then
	if [ -d "$BMARKDIR" ]; then
		rm -r "$BMARKDIR"
	fi
	./setup.sh -x "-DPDEVS=ON -DPDEVS_THREADS=1 -DPDEVS_LOAD=5" -b dxexmachina_devstone
	cp build/Benchmark/dxexmachina_devstone $EXEC_1THREADSLEEP
fi


if [ ! -f "$EXEC_4THREADSLEEP" ]; then
	if [ -d "$BMARKDIR" ]; then
		rm -r "$BMARKDIR"
	fi
	./setup.sh -x "-DPDEVS=ON -DPDEVS_THREADS=4 -DPDEVS_LOAD=5" -b dxexmachina_devstone
	cp build/Benchmark/dxexmachina_devstone $EXEC_4THREADSLEEP
fi

if [ ! -f "$EXEC_16THREADSLEEP" ]; then
	if [ -d "$BMARKDIR" ]; then
		rm -r "$BMARKDIR"
	fi
	./setup.sh -x "-DPDEVS=ON -DPDEVS_THREADS=16 -DPDEVS_LOAD=5" -b dxexmachina_devstone
	cp build/Benchmark/dxexmachina_devstone $EXEC_16THREADSLEEP
fi

EXTRA_ARGS=$1
echo "Setting extra arguments to '$1'"
REPEAT=5
mkdir -p $OUT_DIR
echo "Executing benchmarks"
perf stat -r $REPEAT -o "$OUT_DIR/normal_seq.txt" $EXEC_NORM $EXTRA_ARGS
perf stat -r $REPEAT -o "$OUT_DIR/normal_con.txt" $EXEC_NORM $EXTRA_ARGS cpdevs -c 16
perf stat -r $REPEAT -o "$OUT_DIR/normal_opt.txt" $EXEC_NORM $EXTRA_ARGS opdevs -c 16
perf stat -r $REPEAT -o "$OUT_DIR/pdevs_seq.txt" $EXEC_16THREAD $EXTRA_ARGS
perf stat -r $REPEAT -o "$OUT_DIR/pdevs_con.txt" $EXEC_4THREAD $EXTRA_ARGS cpdevs -c 4
perf stat -r $REPEAT -o "$OUT_DIR/pdevs_opt.txt" $EXEC_4THREAD $EXTRA_ARGS opdevs -c 4
perf stat -r $REPEAT -o "$OUT_DIR/npdevs_sleep_seq.txt" $EXEC_1THREADSLEEP $EXTRA_ARGS
perf stat -r $REPEAT -o "$OUT_DIR/npdevs_sleep_con.txt" $EXEC_1THREADSLEEP $EXTRA_ARGS cpdevs -c 16
perf stat -r $REPEAT -o "$OUT_DIR/npdevs_sleep_opt.txt" $EXEC_1THREADSLEEP $EXTRA_ARGS opdevs -c 16
perf stat -r $REPEAT -o "$OUT_DIR/pdevs_sleep_seq.txt" $EXEC_16THREADSLEEP $EXTRA_ARGS
perf stat -r $REPEAT -o "$OUT_DIR/pdevs_sleep_con.txt" $EXEC_4THREADSLEEP $EXTRA_ARGS cpdevs -c 4
perf stat -r $REPEAT -o "$OUT_DIR/pdevs_sleep_opt.txt" $EXEC_4THREADSLEEP $EXTRA_ARGS opdevs -c 4

perf stat -r $REPEAT -o "$OUT_DIR/normal_seq_r.txt" $EXEC_NORM $EXTRA_ARGS -r
perf stat -r $REPEAT -o "$OUT_DIR/normal_con_r.txt" $EXEC_NORM $EXTRA_ARGS cpdevs -c 16 -r
perf stat -r $REPEAT -o "$OUT_DIR/normal_opt_r.txt" $EXEC_NORM $EXTRA_ARGS opdevs -c 16 -r
perf stat -r $REPEAT -o "$OUT_DIR/npdevs_sleep_seq_r.txt" $EXEC_1THREADSLEEP $EXTRA_ARGS -r
perf stat -r $REPEAT -o "$OUT_DIR/npdevs_sleep_con_r.txt" $EXEC_1THREADSLEEP $EXTRA_ARGS cpdevs -c 16 -r
perf stat -r $REPEAT -o "$OUT_DIR/npdevs_sleep_opt_r.txt" $EXEC_1THREADSLEEP $EXTRA_ARGS opdevs -c 16 -r
perf stat -r $REPEAT -o "$OUT_DIR/pdevs_seq_r.txt" $EXEC_16THREAD $EXTRA_ARGS -r
perf stat -r $REPEAT -o "$OUT_DIR/pdevs_con_r.txt" $EXEC_4THREAD $EXTRA_ARGS cpdevs -c 4 -r
perf stat -r $REPEAT -o "$OUT_DIR/pdevs_opt_r.txt" $EXEC_4THREAD $EXTRA_ARGS opdevs -c 4 -r
perf stat -r $REPEAT -o "$OUT_DIR/pdevs_sleep_seq_r.txt" $EXEC_16THREADSLEEP $EXTRA_ARGS -r
perf stat -r $REPEAT -o "$OUT_DIR/pdevs_sleep_con_r.txt" $EXEC_4THREADSLEEP $EXTRA_ARGS cpdevs -c 4 -r
perf stat -r $REPEAT -o "$OUT_DIR/pdevs_sleep_opt_r.txt" $EXEC_4THREADSLEEP $EXTRA_ARGS opdevs -c 4 -r

