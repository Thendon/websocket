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

#include <time.h>
#include <sys/stat.h>

#define MAX_SOCK 10
#define URI_SIZE 255
#define CHUNK_SIZE 255
#define MAX_PATHLENGTH 255
#define DATE_SIZE 32
#define DATE_FORMAT "%a, %e %b %G %T GTM+1"

// Vorwaertsdeklarationen intern
void scan_request(int sockfd, char* rootDir);
void send_error(int sockfd, const char* version, const int code, const char* text);
void generate_timeString(time_t timer, char* output);
void generate_okHeader(const char* filepath, char* output);
void send_post_calculation(int sockfd, int num1, int num2);
void send_html(int sockfd, const char* path, const char* version);
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
      //close(newsockfd);
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
  char requestBuffer[URI_SIZE+1];
	char* request = (char*)malloc(sizeof(char)*URI_SIZE);

	for(;;){
  	n = read(sockfd,requestBuffer,URI_SIZE);
    requestBuffer[n] = '\0';
    sprintf( request, "%s%s", request, requestBuffer);

    //printf("reqBuffer part %d\n%s\n\n", headerSize, requestBuffer);

  	if(n==0){
      err_abort("Header empty\n");
  	}else if(n < 0){
  		err_abort("Fehler beim Lesen des Sockets!");
  	}else if(n < URI_SIZE /*|| strstr(request,"\r\n\r\n")*/){
      printf("Request komplett (%d chunks)!\n", headerSize);
      break;
    }
    headerSize++;
    request = realloc(request, headerSize * sizeof(char)*URI_SIZE);
  }

  //printf("received request:\n%s\n", request);

  char* header = request;
  char* body = strstr(request, "\r\n\r\n");
  header[body - header] = '\0';
  body = &body[4];

  printf("header:\n%s\n", header);
  printf("body:\n%s\n", body);

	char* command = strtok(header, " ");
  printf("command: %s\n", command);
	if( strcmp(command,"GET") == 0 ){
    char path[MAX_PATHLENGTH];
    sprintf(path, "%s%s", rootDir, strtok(NULL, " "));
    char* version;
    version = strtok(NULL, "\n");
    printf("path: %s\nversion: %s\n", path, version);

    free(request);

    send_html(sockfd, path, version);
	} else if ( strcmp(command,"POST") == 0 ){
    strtok(body, "=");
    int num1 = atoi(strtok(NULL, "&"));
    strtok(NULL, "=");
    int num2 = atoi(strtok(NULL, "&"));

    free(request);

    send_post_calculation(sockfd, num1, num2);
	} else {
    free(request);
		err_abort("Header invalid!\n");
	}
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

void generate_timeString(time_t timer, char* output){
  struct tm* tm_info = localtime(&timer);
  strftime(output, DATE_SIZE, DATE_FORMAT, tm_info);
}

void generate_okHeader(const char* filepath, char* output){
  time_t timer;
  time(&timer);
  char now[DATE_SIZE];
  generate_timeString(timer, now);

  char modified[DATE_SIZE];
  if(filepath != NULL){
    struct stat attr;
    stat(filepath, &attr);
    generate_timeString(attr.st_mtime, modified);
  }
  else {
    memcpy(modified, now, sizeof(modified));
  }

  if(strstr(filepath, ".png"))
  {
    sprintf(
    output,
    "HTTP/1.1 200 OK\nDate: %s\nLast-Modified: %s\nContent-Language: de\nContent-Type: image/png; charset=utf-8\r\n\r\n",
    now,
    modified
    );
  }
  else
  {
    sprintf(
      output,
      "HTTP/1.1 200 OK\nDate: %s\nLast-Modified: %s\nContent-Language: de\nContent-Type: text/html; charset=utf-8\r\n\r\n",
      now,
      modified
    );
  }
}

void send_post_calculation(int sockfd, int num1, int num2){
  char response[CHUNK_SIZE+1];
  memset(response, '\0', CHUNK_SIZE+1);
  generate_okHeader( NULL, response );

  sprintf(
    response,
    "%s<HTML><BODY><center><h1> Ergebnis: %d</h1></center></BODY></HTML>",
    response,
    num1 * num2
  );

  if(write(sockfd, response, strlen(response)) != strlen(response)){
    err_abort("Fehler beim Schreiben des Sockets!");
  }
}

void send_html(int sockfd, const char* path, const char* version){
  FILE* file = fopen( path, "r" );
  if( file == NULL ){
    send_error(sockfd, version, 404, "Not Found");
    return;
  }

  char response[CHUNK_SIZE+1];
  memset(response, '\0', CHUNK_SIZE+1);
  generate_okHeader( path, response );

  int n;
  for(int i = 0;;i++){
    //if first response sub headerbytes
    int resLength = (int)strlen(response);
    if( resLength > 0 ){
      printf("\nsprintf\n");
      //char chunk[CHUNK_SIZE-resLength+1];
      //n = fread(chunk, 1, CHUNK_SIZE-resLength, file);
      //chunk[n] = '\0';

      //response[resLength] = *chunk; 
      //sprintf(response, "%s%s", response, chunk);
      //n += resLength;
      n=resLength;
    }
    else{
      printf("\nSPRINTF\n");
      n = fread(response, 1, CHUNK_SIZE, file);
    }

    printf("response (i: %d l: %d):\n%s\n\n", i, n,response);

    if(write(sockfd, response, n) != n){
      err_abort("Fehler beim Schreiben des Sockets!");
    }

    //break if chunk is not filled on cap
    if( n < CHUNK_SIZE && resLength == 0){
			printf("HTML-Transfer vollendet!\n");
      break;
    }
    //clear response for next chunk
    memset(response, '\0', CHUNK_SIZE+1);
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
