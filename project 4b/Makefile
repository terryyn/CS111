#NAME: Yunong Ye
#EMAIL: yeyunong@hotmail.com
#ID: 004757414

.SILENT:

default:
	gcc -g -Wall -Wextra -lmraa -lm lab4b.c -o lab4b

dist:
	tar -cvzf lab4b-004757414.tar.gz lab4b.c Makefile README

clean:
	rm *.tar.gz *.o ./lab4b

check: default
	./lab4b << -EOF \n
	OFF
	EOF
