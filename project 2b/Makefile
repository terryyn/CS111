#NAME:Yunong Ye
#EMAIL:yeyunong@hotmail.com
#ID:004757414


FILES = Makefile README test.sh *.png *.csv plot.gp lab2_list.c SortedList.h SortedList.c profile.out

.SILENT:

all: build

build: temp lab2_list

tests:  build
        chmod +x test.sh
        ./test.sh

graphs:
        gnuplot plot.gp

dist:   tests graphs
        tar -cvzf lab2b-004757414.tar.gz $(FILES)
clean:
        rm -f *.tar.gz ./lab2_list *.o

lab2_list: clean lab2_list.o SortedList.o
        gcc -o $@ -pthread -g lab2_list.o SortedList.o

profile.gperf: lab2_list temp
        rm -f ./raw.gperf
        LD_PRELOAD=/u/eng/class/classrzw/bin/lib/libprofiler.so.0 CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
        pprof --text ./lab2_list ./raw.gperf > profile.gperf
        pprof --list=thread_init ./lab2_list ./raw.gperf >> profile.gperf
        mv profile.gperf profile.out
        rm -f ./raw.gperf

profile: profile.gperf

temp:
        gcc -Wall -Wextra -g -pthread lab2_list.c SortedList.c -o lab2_list

