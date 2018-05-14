#!/bin/sh
for i in 1 2 4 8 12 16 24
do
    ./lab2_list --threads=$i --iterations=1000 --sync=m >> lab2_list.csv
    ./lab2_list --threads=$i --iterations=1000 --sync=s >> lab2_list.csv
done

