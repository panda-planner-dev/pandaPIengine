#!/bin/bash
echo "*"
echo "* Building active configurations"
echo "*"
if [ -d "build" ]; then
  rm -r build
fi
mkdir build

if [ -d "temp" ]; then
  rm -r temp
fi
mkdir temp/

cd build
mv ../src/flags.h ../src/flags-org.h

for config in ../configs/active/*.h; do
  confName="$(basename ${config%.*})"
  confName=$(echo $confName| cut -c 7-)

  echo "- $confName"
  echo "*" >> ../makeLog.txt
  echo "* $confName" >> ../makeLog.txt
  echo "*" >> ../makeLog.txt
  cp "$config" ../src/flags.h
  rm -rf *
  cmake .. &>> ../makeLog.txt
  make -j &>> ../makeLog.txt 
  mv ./src/pandaPIengine "../temp/ppro-$confName"
done

grep -i "error" ../makeLog.txt
mkdir ../temp/log-configs
mv ../makeLog.txt ../temp/log-configs/
cp ../configs/active/*.h ../temp/log-configs/
mv ../src/flags-org.h ../src/flags.h
cd ..
rm -r build
mv temp build
