#!/bin/sh

# Small script to reproduce memory tests.
# NOTE: valgrind effectively cripples the runtime of any benchmark, so be prepared to wait a long time for them to complete. 
# Grep oneliner originates from https://stackoverflow.com/questions/774556/peak-memory-usage-of-a-linux-unix-process

# (system) Commands needed
VALGRIND="valgrind"
GREP="grep"
SED="sed"
TAIL="tail"
SORT="sort"

# Let's do a quick check if they can be found, and try to keep it POSIX.
declare -a cmdar=($VALGRIND $GREP $SED $TAIL $SORT)
for c in "${cmdar[@]}"
do
    echo -n "$c exists ? "
    EXISTS=`command -v "$c"`
    if [ -z "$EXISTS" ]
    then
        echo \n"Script requires $c, not found"
        exit 1;
    fi
    echo "Y";
done

# Are we running in the right directory ?
BENCHMARKDIR="build/Benchmark"
if [ ! -d "build/Benchmark" ]
then
echo "Not running in top directory project, can't find $BENCHMARKDIR";
exit -1;
fi

MASSIFFILE="current.out"
RESULTSFILE="results.txt"
ENDTIME="-t 500000"

echo "!!! WARNING : Running these benchmarks can ~very~ easily exhaust all physical memory. Run this on a machine with _at least_ 8GB RAM. !!!"

echo "Format : CMD \n x Bytes max heap\n" >> $RESULTSFILE

declare -a arr=("build/Benchmark/adevs_devstone -w 40 -d 40 $ENDTIME -c 2 cpdevs" 
                "build/Benchmark/adevs_devstone -w 40 -d 40 $ENDTIME" 
                "build/Benchmark/dxexmachina_devstone -w 40 -d 40 $ENDTIME" 
                "build/Benchmark/dxexmachina_devstone -w 40 -d 40 $ENDTIME -c 2 cpdevs" 
                "build/Benchmark/dxexmachina_devstone -w 40 -d 40 $ENDTIME -c 2 opdevs" 
                "build/Benchmark/dxexmachina_phold -n 4 -s 2 -r 10 -p 0.1 $ENDTIME -c 4 opdevs" 
                "build/Benchmark/dxexmachina_phold -n 4 -s 2 -r 10 -p 0.1 $ENDTIME -c 4 cpdevs" 
#                "build/Benchmark/adevs_phold -n 4 -s 2 -r 10 -p 0.1 $ENDTIME -c 4 cpdevs" DNF
                "build/Benchmark/dxexmachina_phold -n 4 -s 2 -r 10 -p 0.1 $ENDTIME" 
                "build/Benchmark/adevs_phold -n 4 -s 2 -r 10 -p 0.1 $ENDTIME" 
                "build/Benchmark/dxexmachina_interconnect -w 20 $ENDTIME -c 2 opdevs" 
                "build/Benchmark/dxexmachina_interconnect -w 20 $ENDTIME -c 2 opdevs"
                "build/Benchmark/dxexmachina_interconnect -w 20 $ENDTIME"
                "build/Benchmark/adevs_interconnect -w 20 $ENDTIME -c 2 cpdevs"
                "build/Benchmark/adevs_interconnect -w 20 $ENDTIME"
                )

for i in "${arr[@]}"
do
    echo "Executing $i";
    echo "$i" >> $RESULTSFILE
    $VALGRIND --tool=massif --pages-as-heap=yes --massif-out-file=$MASSIFFILE $i;
    $GREP mem_heap_B $MASSIFFILE | $SED -e 's/mem_heap_B=\(.*\)/\1/' | sort -g | $TAIL -n 1 >> $RESULTSFILE; 
done

echo "Completed all benchmarks, results are in $RESULTSFILE"

