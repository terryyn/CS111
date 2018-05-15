#!/bin/sh                                                                                                                                              

for i in 1 2 4 8 12 16 24
do
    ./lab2_list --threads=$i --iterations=1000 --sync=m >> lab2b_list.csv
    ./lab2_list --threads=$i --iterations=1000 --sync=s >> lab2b_list.csv
done

for i in 1 4 8 12 16
do
    for k in 1 2 4 8 16
    do
        ./lab2_list --yield=id --lists=4 --threads=$i --iterations=$k >> lab2b_list.csv
    done

    for y in 10 20 40 80
    do
        ./lab2_list --yield=id --lists=4 --threads=$i --iterations=$y --sync=m >> lab2b_list.csv
        ./lab2_list --yield=id --lists=4 --threads=$i --iterations=$y --sync=s >> lab2b_list.csv
    done
done



for i in 1 2 4 8 12
do
    for k in 4 8 16
    do
        ./lab2_list --lists=$k --threads=$i --iterations=1000 --sync=m >> lab2b_list.csv
        ./lab2_list --lists=$k --threads=$i --iterations=1000 --sync=s >> lab2b_list.csv
    done
done



for i in 1 2 4 8 12
do
    for k in 4 8 16
    do
        ./lab2_list --lists=$k --threads=$i --iterations=1000 --sync=m >> lab2b_list.csv
        ./lab2_list --lists=$k --threads=$i --iterations=1000 --sync=s >> lab2b_list.csv
    done
done
