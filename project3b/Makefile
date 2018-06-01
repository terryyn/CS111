#NAME: Xiwei Ma,Yunong Ye
#EMAIL: xiweimacn@163.com,yeyunong@hotmail.com
#ID: 704755732,004757414

default:
	echo "python3 lab3b.py \$1" > lab3b
	chmod +x lab3b

clean:
	rm -f *.tar.gz lab3b

dist: lab3b-004757414.tar.gz
files = lab3b.py Makefile README

lab3b-004757414.tar.gz:
	tar -zcf $@ $(files)