#!/bin/sh


# @author Ben Cardoen - Stijn Manhaeve -- Devs Ex Machina project.
DIR="build/Benchmark/"
DXEX="dxexmachina_devstone"
ADEVS="adevs_devstone"
ARGS=" -d 10 -w 150 -r "
ADEVS="adevs_devstone"
PARARGS=" -c 8 "
OPT=" opdevs "
CON=" cpdevs "

declare -a arr=("$DIR$DXEX$ARGS"
				"$DIR$DXEX$ARGS$PARARGS$CON"
				"$DIR$DXEX$ARGS$PARARGS$OPT"
				"$DIR$ADEVS$ARGS"
				"$DIR$ADEVS$ARGS$PARARGS$CON"
                )

BASETIME=200000
for i in "${arr[@]}";
	do
	RTIME=$BASETIME
	for j in `seq 1 4`;
	do
		echo -n "memkb of $i $RTIME== "
    	eval "/usr/bin/time -v $i -d 10 -w 150 -t $RTIME" 2>&1 | grep Maximum | awk '{print $6}'
    	RTIME=$((RTIME*2))
	done
done