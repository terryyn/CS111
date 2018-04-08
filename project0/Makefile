default: lab0.c
	 gcc -g -Wall -Wextra lab0.c -o lab0


check: default clean
     echo "test" > input.txt;\
     ./lab0 --input=input.txt --output=output.txt ;\
    if[ $$? -ne 0]; then \
    echo "error occurred in process" > error.txt ;\
    fi;\
    cmp input.txt output.txt;\
    if [$$? -ne 0]; then \
    echo "input and output not match" > error.txt;\
    fi;\
    rm -f output.txt ;\


    #check argument 
    ./lab0 --wrong   #wrong argument 
    if [$$? -ne 1]; then \
    echo " error handling wrong argument" > error.txt ;\
    fi;\

    #check input
     ./lab0 --input=test.txt   #test.txt not exist
     if [$$? -ne 2]; then \
     echo "error handling --input" > error.txt; \
     fi;\
     cat test.txt; \
     chmod -r test.txt;\
     ./lab0 --input=test.txt   #test.txt not readable
     if [$$? -ne 2]; then \
     echo "error handling --input" > error.txt; \
     fi;\




    #check segfault&catch
    ./lab0 --segfault --catch ;\
    if [$$? -ne 4] ; then\
    echo "error handling --segfault" > error.txt;\
    fi;\

    if [-f error.txt] ; then\
    echo "check failed" ;\
    else \
    echo "check success" ;\
    fi;\
    clean


clean:
	rm -f lab0 *.txt *.tar.gz

dist:
	tar -cvzf lab0-004757414.tar.gz lab0.c Makefile backtrace.png breakpoint.png README
