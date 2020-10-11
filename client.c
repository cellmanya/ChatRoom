/*
 * client.c
 *
 *  Created on: 9 Eki 2020
 *      Author: CELL
 */

// Standard Libraries
#define _WIN32_WINNT 0x501
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <pthread.h>

const char *PORT = "4000";
#define MAX_BUFF_SIZE 100
char clientname[20];

#if defined WIN32

#define close	closesocket
#include <ws2tcpip.h>
#include <windows.h>

#define SOKET SOCKET

#else

#include <netdb.h>
#include <unistd.h>
#include <termios.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#define SOKET int

#endif


void* SendThread(void* sckt){

	SOKET so = *(SOKET*)sckt;
	char message[MAX_BUFF_SIZE];
	char buff[MAX_BUFF_SIZE + 20];

	while(1){

		printf("> ");
		fgets(message, MAX_BUFF_SIZE, stdin);

		if( strncmp("Exit", message, 4) == 0){

			if (send(so, message, 4, 0) == -1)
				perror("send");

		break;
		}

		else{

			sprintf(buff, "%s : %s\n", clientname, message );
			if (send(so, buff, strlen(buff), 0) == -1)
				perror("send");

			}

		memset(&message, 0, sizeof message);
		memset(&buff, 0, sizeof buff);
	}

	pthread_exit(NULL);
}

void* RecvThread(void* sckt){

	SOKET so = *(SOKET*)sckt;
	char buff[MAX_BUFF_SIZE + 20];
	int bytenumber;

	while(1){

		if ((bytenumber = recv(so, buff, sizeof buff, 0)) == -1) {
			perror("recv");
			break;
		}

		else if( bytenumber >= 0 ){

			buff[bytenumber] = '\0';
			printf("%s\n", buff);

		}

		memset(&buff, 0, sizeof buff);

	}

	pthread_exit(NULL);
}



int main(){

	//setvbuf(stdout,NULL, _IONBF, 0); // Disable the Eclipse Console Buffer

	SOKET sckt;
	int status;
	struct addrinfo addr, *serverinfo;
	pthread_t send_t;
	pthread_t recv_t;

#ifdef _WIN32
		WSADATA firstsock;
		//Initialise winsock
		if (WSAStartup(MAKEWORD(2,0),&firstsock) != 0)  //CHECKS FOR WINSOCK VERSION 2.0
		{
			fprintf(stderr,"WSAStartup() failed");
			exit(EXIT_FAILURE);
		}
#endif

		printf("Please enter your name -> ");
		fgets(clientname, 20, stdin);
		str_trim_lf(clientname, strlen(clientname));


		memset(&addr, 0, sizeof addr);
		addr.ai_family = AF_UNSPEC;
		addr.ai_socktype = SOCK_STREAM;

		if ((status = getaddrinfo(INADDR_ANY, PORT, &addr, &serverinfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 1;
		}

		if ((sckt = socket(serverinfo->ai_family, serverinfo->ai_socktype,serverinfo->ai_protocol)) == -1) {
			perror("client: socket");

		}

		if (connect(sckt, serverinfo->ai_addr, serverinfo->ai_addrlen) == -1) {
			close(sckt);
			perror("client: connect");
		}

		freeaddrinfo(serverinfo);

		printf("CONNECTION ESTABLISHED");

		if(send(sckt, clientname, 20, 0) == -1)
			perror("send");


		if(pthread_create(&send_t, NULL, SendThread, &sckt) != 0){
			printf("ERROR: pthread\n");
			return EXIT_FAILURE;
		}


		if(pthread_create(&recv_t, NULL, RecvThread, &sckt) != 0){
			printf("ERROR: pthread\n");
			return EXIT_FAILURE;
		}

		close(sckt);


	return (0);
}
