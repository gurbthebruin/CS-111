# Gurbir Arora
# gurbthebruin@g.ucla.edu
# 105178554


default:  	
	gcc -g -lmraa -lm -Wall -Wextra -o lab4b lab4b.c
check:
	chmod +x ./smoketest.sh
	./smoketest.sh

dist:
	tar -czvf lab4b-105178554.tar.gz Makefile README lab4b.c smoketest.sh

clean:
	rm -rf *.tar.gz
