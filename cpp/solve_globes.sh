#!/bin/bash

if [ "$#" -ne 6 ]; then
    echo "usage: $0 start_idx end_idx beam_width max_cost max_depth max_conjugate_depth"
    exit 1
fi

start=$1
end=$2

for (( i=start; i<=end; i++ ))
do
    ./solve.out $i $3 $4 $5 $6
done
