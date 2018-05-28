#NAME: Xiwei Ma,Yunong Ye
#EMAIL: xiweimacn@163.com,yeyunong@hotmail.com
#ID: 704755732,004757414

CC = gcc
CFlAGS = -Wall -Wextra -g

default: lab3a

lab3a: 
	$(CC) -Wall -Wextra -g lab3a.c -o $@

dist: lab3a-704755732.tar.gz
files = lab3a.c Makefile README EXT2_header.h

lab3a-704755732.tar.gz:
	tar -zcf $@ $(files)

clean:
	rm -f *.tar.gz lab3a *.csv
