all: httpserv

clean:
	rm httpserv

httpserv: web_server.c
	gcc -o httpserv web_server.c
