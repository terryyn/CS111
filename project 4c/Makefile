#NAME:Yunong Ye
#EMAIL:yeyunong@hotmail.com
#ID:004757414

CC = gcc
CFLAGS = -Wall -Wextra -g -lmraa -lm -lcrypto -lssl
default:
	$(CC) $(CFLAGS) lab4c_tcp.c -o lab4c_tcp
	$(CC) $(CFLAGS) lab4c_tls.c -o lab4c_tls
clean:
	rm -f *.o *.tar.gz ./lab4c_tcp ./lab4c_tls
dist:
	tar -cvzf lab4c-004757414.tar.gz lab4c_tcp.c lab4c_tls.c Makefile README
