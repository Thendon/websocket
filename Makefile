all: httpserv

clean:
	rm httpserv

httpserv: web_server.c
	gcc -g -std=c99 -o httpserv web_server.c
