#NAME:Yunong Ye
#EMAIL:yeyunong@hotmail.com
#ID:004757414


FILES = lab2_add.c Makefile README yield.sh *.png *.csv *.gp lab2_list.c SortedList.h SortedList.c 

.SILENT:

build:
	gcc lab2_add.c -Wall -Wextra -pthread -o lab2_add
	gcc lab2_list.c SortedList.c SortedList.h -Wall -Wextra -pthread -o lab2_list

tests:  
	chmod +x yield.sh
	./yield.sh
graphs:
	gnuplot lab2_add.gp
	gnuplot lab2_list.gp

dist:   build tests graphs
	tar -cvzf lab2a-004757414.tar.gz $(FILES)
clean:
	rm -f *.tar.gz ./lab2_add ./lab2_list *.gch *.o

lab2_list: lab2_list.o SortedList.o
	gcc -o $@ -pthread -Wall -Wextra -g -L /u/eng/class/classrzw/lib -lprofiler lab2_list.o SortedList.o

profile.gperf: lab2_list
	rm -f ./raw.gperf
	LD_PRELOAD=/u/eng/class/classrzw/lib/libprofiler.so.0 \
		CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	pprof --text ./lab2_list ./raw.gperf > profile.gperf
	pprof --list=thread_init ./lab2_list ./raw.gperf >> $@
	rm -f ./raw.gperf

