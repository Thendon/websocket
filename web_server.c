/*
  Web Server
*/

#include <unistd.h>    // fuer read, write etc.
#include <stdlib.h>     // fuer exit
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_SOCK 10
#define URI_SIZE 255
#define CHUNK_SIZE 255
#define MAX_PATHLENGTH 255

// Vorwaertsdeklarationen intern
void scan_request(int sockfd, char* rootDir);
void send_error(int sockfd, const char* version, const int code, const char* text);
void send_file(int sockfd, const char* path, const char* version);
void err_abort(char *str);

int main(int argc, char *argv[]) {
  if(argc < 3){
    err_abort("Missing Arguments! Syntax: httpserv [rootDir] [port]");
  }
  // Argumente prüden
  char* rootDir = argv[1];
  int port = atoi(argv[2]);
  if(port <= 1024){
    err_abort("Port reserved!");
  }
  printf("starting server on port: %d, rootDir: %s\n", port, rootDir);

	// Deskriptoren, Adresslaenge, Prozess-ID
	int sockfd, newsockfd, alen, pid;
	int reuse = 1;

	// Socket Adressen
	struct sockaddr_in cli_addr, srv_addr;

	// TCP-Socket erzeugen
	if((sockfd=socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		err_abort("Kann Stream-Socket nicht oeffnen!");
	}void send_file(int sockfd, const char* path, const char* version);

  if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse))<0){
		err_abort("Kann Socketoption nicht setzen!");
	}

	// Binden der lokalen Adresse damit Clients uns erreichen
	memset((void *)&srv_addr, '\0', sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	srv_addr.sin_port = htons(port);
	if( bind(sockfd, (struct sockaddr *)&srv_addr,
		sizeof(srv_addr)) < 0 ) {
		err_abort("Kann lokale Adresse nicht binden, laeuft fremder Server?");
	}

	// Warteschlange fuer TCP-Socket einrichten
	listen(sockfd,5);
	printf("Web Server: bereit ...\n");

	for(;;){
		alen = sizeof(cli_addr);

		// Verbindung aufbauen
		newsockfd = accept(sockfd,(struct sockaddr *)&cli_addr,&alen);
		if(newsockfd < 0){
			err_abort("Fehler beim Verbindungsaufbau!");
		}

		// fuer jede Verbindung einen Kindprozess erzeugen
		if((pid = fork()) < 0){
			err_abort("Fehler beim Erzeugen eines Kindprozesses!");
		}else if(pid == 0){
			close(sockfd);
			scan_request(newsockfd, rootDir);
			exit(0);
		}
		close(newsockfd);
	}
}

/*
scan_request: Lesen von Daten vom Socket und an den Client zuruecksenden
*/
void scan_request(int sockfd, char* rootDir){
	int n;
  int headerSize = 1;
  char requestBuffer[URI_SIZE];
	char* request = (char*)malloc(sizeof(char)*URI_SIZE);
  char path[MAX_PATHLENGTH];

	char* header;
  char* version;

	for(;;){
  	n = read(sockfd,requestBuffer,URI_SIZE);
    sprintf( request, "%s%s", request, requestBuffer);
  	if(n==0){
  		printf("Header empty\n");
  	}else if(n < 0){
  		err_abort("Fehler beim Lesen des Sockets!");
  	}else if(n < URI_SIZE || requestBuffer[n-1] == '\0'){
      printf("Request komplett (%d)!\n", headerSize);
      break;
    }
    headerSize++;
    request = realloc(request, headerSize * sizeof(char)*URI_SIZE);
  }

	header = strtok(request, " ");
  printf("command: %s\n", header);
	if( strcmp(header,"GET") == 0 ){
    sprintf(path, "%s%s", rootDir, strtok(NULL, " "));
    version = strtok(NULL, "\n");
    printf("path: %s\nversion: %s\n", path, version);

    send_html(sockfd, path, version);
	} else if ( strcmp(header,"POST") == 0 ){
		//send_file
	} else {
		err_abort("Header invalid!\n");
	}
	//}
}

void send_error(int sockfd, const char* version, const int code, const char* text){
  char response[URI_SIZE];

  sprintf(response, "HTTP/1.1 %d %s\r\n\r\n%s\r\n\r\n", code, text, text);
  int n = strlen(response);

  printf("sending: %s\n", response);
  if(write(sockfd, response, n) != n){
    err_abort("Fehler beim Schreiben des Sockets!");
  }
}

void send_html(int sockfd, const char* path, const char* version){
  FILE* file = fopen( path, "r" );
  if( file == NULL ){
    send_error(sockfd, version, 404, "Not Found");
    return;
  }
  fclose( file );
}

/*
Ausgabe von Fehlermeldungen
*/
void err_abort(char *str){
	fprintf(stderr," TCP Echo-Server: %s\n",str);
	fflush(stdout);
	fflush(stderr);
	exit(1);
}
