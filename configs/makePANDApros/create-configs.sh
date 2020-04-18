#!/bin/bash
rm ./full-configs/*.h
rm ./full-configs/makeLog.txt

for searchStr in ./search-configs/*.h; do
  for heuristic in ./heuristic-configs/*.h; do
    confName="$(basename ${searchStr%.*})-$(basename ${heuristic%.*})"
    echo "- $confName"
    echo "*" >> makeLog.txt
    echo "* $confName" >> makeLog.txt
    echo "*" >> makeLog.txt
    cat base-config/head.h > "flags-$confName.h"
    cat "$searchStr" >> "flags-$confName.h"
    cat "$heuristic" >> "flags-$confName.h"
    cat base-config/tail.h >> "flags-$confName.h"
    
    mv "flags-$confName.h" ./full-configs/
  done
done

mv makeLog.txt ./full-configs/
