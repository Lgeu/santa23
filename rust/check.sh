#!/bin/bash

echo "check cube"
RAINBOW_IDS=$(cat ./cube_rainbow_ids.txt)
for i in {150..283}
do
    echo "Running $i"
    if [[ $RAINBOW_IDS == *"$i"* ]]; then
        head -n 1 ../cpp/solution_rainbow/$i\_best.txt > ./tools/out/0$i.txt && \
            ./target/release/vis ./tools/in/0$i.txt ./tools/out/0$i.txt
    else
        head -n 1 ../cpp/solution_three_order/$i\_best.txt > ./tools/out/0$i.txt && \
            ./target/release/vis ./tools/in/0$i.txt ./tools/out/0$i.txt
    fi
done


echo "check wreath"
for i in {329..337}
do
    echo "Running $i"
    tail -n 1 ../cpp/result/wreath/$i.txt > ./tools/out/0$i.txt
    ./target/release/vis ./tools/in/0$i.txt ./tools/out/0$i.txt
done