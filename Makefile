all: httpserv

clean:
	rm httpserv

httpserv: web_server.c
	gcc -g -o httpserv web_server.c
