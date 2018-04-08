.SILENT:

default: lab0.c
	 gcc -g -Wall -Wextra lab0.c -o lab0

check:	clean default
	echo "test" > input.txt; \
	./lab0 --input=input.txt --output=output.txt ; \
	if [ $$? -ne 0 ] ; then \
	echo "error" > error.txt ; \
	fi ; \
	cmp input.txt output.txt ; \
	if [ $$? -ne 0 ] ; then \
	echo "input and output not matched" > error.txt ; \
	fi ; \
	rm -f output.txt

	#invalid argument
	./lab0 --sdad &>temp ; \
	if [ $$? -ne 1 ] ; then \
	echo "error handling invalid argument" > error.txt ; \
	fi ; \

	#catch test
	./lab0 --segfault --catch &>temp ; \
	if [ $$? -ne 4 ] ; then \
	echo "error catch segmentation fault" > error.txt ; \
	fi ; \

	./lab0 --input=bad &>temp ; \
	if [ $$? -ne 2 ] ; then \
	echo "error handling input " > error.txt ; \
	fi ; \




	if [ -f error.txt ] ; then \
	echo "make check failed" ; \
	else \
	echo "make check succeed" ; \
	fi ; \
	rm -f temp

clean:
	rm -f lab0 *.txt *.tar.gz

dist:
	tar -cvzf lab0-004757414.tar.gz lab0.c Makefile backtrace.png breakpoint.png README

