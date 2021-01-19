# Gurbir Arora
# gurbthebruin@g.ucla.edu
# 105178554

default: client server
client: 
	gcc -g -Wall -Wextra -lz -o lab1b-client lab1b-client.c

server:
	gcc -g -Wall -Wextra -lz -o lab1b-server lab1b-server.c 

dist:
	tar -cvzf lab1b-105178554.tar.gz lab1b-client.c lab1b-server.c README Makefile

clean: 
	rm -f lab1b-105178554.tar.gz lab1b-client lab1b-server
