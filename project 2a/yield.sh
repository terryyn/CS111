#!/bin/sh
for i in 2 4 8 12
do
    for k in 10 20 40 80 100 1000 10000 100000
    do
	./lab2_add --yield --threads=$i --iterations=$k >> lab2_add.csv
    done
done


for j in {2..8}
do
    for t in 100 1000 10000 100000
    do
	./lab2_add --threads=$j --iterations=$t >> lab2_add.csv
    done
done


for temp in 10 20 40 100 1000 10000 100000
do
    ./lab2_add --iterations=$temp >> lab2_add.csv
done    

for thread in 2 4 8 12
do
    ./lab2_add --yield --sync=m --iterations=10000 --threads=$thread >> lab2_add.csv
    ./lab2_add --yield --sync=s --iterations=1000 --threads=$thread >> lab2_add.csv
    ./lab2_add --yield --sync=c --iterations=10000 --threads=$thread >> lab2_add.csv
done

for thread in 1 2 4 8 12
do
    ./lab2_add --iterations=10000 --threads=$thread >> lab2_add.csv
    ./lab2_add --sync=m --iterations=10000 --threads=$thread >> lab2_add.csv
    ./lab2_add --sync=c --iterations=10000 --threads=$thread >> lab2_add.csv
    ./lab2_add --sync=s --iterations=10000 --threads=$thread >> lab2_add.csv
done

for iterations in 10 100 1000 10000 20000
do
    ./lab2_list --iterations=$iterations >> lab2_list.csv
done

for temp in 2 4 8 12
do
    for temp2 in 1 10 100 1000
    do
	./lab2_list --threads=$temp --iterations=$temp2 >> lab2_list.csv
    done
    for temp3 in 1 2 4 8 16 32
    do
	./lab2_list --threads=$temp --iterations=$temp3 --yield=i >> lab2_list.csv
        ./lab2_list --threads=$temp --iterations=$temp3 --yield=d >> lab2_list.csv
        ./lab2_list --threads=$temp --iterations=$temp3 --yield=il >> lab2_list.csv
        ./lab2_list --threads=$temp --iterations=$temp3 --yield=dl >> lab2_list.csv
    done
done    
	
	
	
for yield in i d il dl
do
    ./lab2_list --threads=12 --iterations=32 --yield=$yield --sync=m >> lab2_list.csv
    ./lab2_list --threads=12 --iterations=32 --yield=$yield --sync=s >> lab2_list.csv
done    

for thread in 1 2 4 8 12 16 24
do
    ./lab2_list --threads=$thread --iterations=1000 --sync=m >> lab2_list.csv
    ./lab2_list --threads=$thread --iterations=1000 --sync=s >> lab2_list.csv
done    
