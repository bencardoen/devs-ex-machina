#!/bin/sh


# @author Ben Cardoen - Stijn Manhaeve -- Devs Ex Machina project.
DIR="build/Benchmark/"
DXEXQ="dxexmachina_devstone"
DXEXI="dxexmachina_interconnect"
DXEXP="dxexmachina_phold"
ADEVSQ="adevs_devstone"
ADEVSI="adevs_interconnect"
ADEVSP="adevs_phold"
ARGSQ=" -d 10 -w 150 -r "
ARGSI=" -w 20 "
ARGSP=" -n 4 -s 2 -r 10 -p 0.1 "
PARARGS=" -c 4 "
OPT=" opdevs "
CON=" cpdevs "

declare -a arr=("$DIR$DXEXQ$ARGSQ"
				"$DIR$DXEXQ$ARGSQ$PARARGS$CON"
				"$DIR$DXEXQ$ARGSQ$PARARGS$OPT"
				"$DIR$ADEVSQ$ARGSQ"
				"$DIR$ADEVSQ$ARGSQ$PARARGS$CON"
				"$DIR$DXEXI$ARGSI"
				"$DIR$DXEXI$ARGSI$PARARGS$CON"
				"$DIR$DXEXI$ARGSI$PARARGS$OPT"
				"$DIR$ADEVSI$ARGSI"
				"$DIR$ADEVSI$ARGSI$PARARGS$CON"
				"$DIR$DXEXP$ARGSP"
				"$DIR$DXEXP$ARGSP$PARARGS$CON"
				"$DIR$DXEXP$ARGSP$PARARGS$OPT"
				"$DIR$ADEVSP$ARGSP"
				"$DIR$ADEVSP$ARGSP$PARARGS$CON"
                )

BASETIME=500000
for i in "${arr[@]}";
	do
	for j in `seq 1 5`;
		do
		RTIME=$BASETIME
		echo -n "memkb of $i $RTIME== "
		eval "/usr/bin/time -v $i -t $RTIME" 2>&1 | grep Maximum | awk '{print $6}'
	done
done