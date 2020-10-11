/*
 * server.c
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
#define BACKLOG 10
#define MAX_CLIENT BACKLOG

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

typedef struct{

	char name[20];
	SOKET sckt;
	int client_id;

}client_t;

client_t clients[MAX_CLIENT];

void SendtoClients(char *str, int client_id){

	for(int j = 0; j < MAX_CLIENT; j++){

		if(clients[j].client_id)
			if(clients[j].client_id != client_id){
				if(send(clients[j].sckt, str, strlen(str), 0) == -1){

					perror("send");
					break;
				}
			}
	}

}


void* ServerHandler(void* client_addr){

	client_t  *client = (client_t*)client_addr;
	char buff[MAX_BUFF_SIZE];
	char name[20];
	int bytenumber;

	if((bytenumber = recv(client->sckt, name, 20, 0)) == -1)
		perror("recv");

	name[bytenumber] = '\0';

	if( (bytenumber < 1) || (strlen(name) < 1) ){

		puts("Improper Name !!\n Closing the thread...");
		pthread_exit(NULL);
	}
	else{

		//strcpy(client->name, name);
		sprintf(buff, "%s has joined the room\n", name);
		printf("%s\n", buff);
		SendtoClients(buff, client->client_id);

	}

	memset(&buff, 0, sizeof buff);

	while(1) {

		if((bytenumber = recv(client->sckt, buff, MAX_BUFF_SIZE, 0)) == -1){
			perror("recv");
			break;
		}

		else if((bytenumber >= 0) && (strlen(buff) >= 0)){

			buff[bytenumber] = '\0';
			SendtoClients(buff, client->client_id);
		}

		else if( strncmp( "Exit", buff, 4 ) == 0){

			sprintf(buff, "%s has left the room", client->name);
			printf("%s", buff);
			SendtoClients(buff, client->client_id);
			break;
		}



		memset(buff, 0, sizeof buff);
	}

	close(client->sckt);
	return NULL;

}

int main(){

	//setvbuf(stdout,NULL, _IONBF, 0); // Disable the Eclipse Console Buffer

	SOKET sckt, newsckt;
	struct addrinfo addr, *serverinfo;
	struct sockaddr_storage their_addr;
	socklen_t  sin_size;
	int status;
	int index = 0;
	pthread_t tid;


#ifdef _WIN32
		WSADATA firstsock;
		//Initialise winsock
		if (WSAStartup(MAKEWORD(2,0),&firstsock) != 0)  //CHECKS FOR WINSOCK VERSION 2.0
		{
			fprintf(stderr,"WSAStartup() failed");
			exit(EXIT_FAILURE);
		}
#endif

	memset(&addr, 0, sizeof addr);
	memset(&clients, 0, sizeof clients);
	addr.ai_family = AF_UNSPEC;
	addr.ai_socktype = SOCK_STREAM;
	addr.ai_flags = AI_PASSIVE; // use my IP

	if ((status = getaddrinfo(NULL, PORT, &addr, &serverinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 1;
	}

	if ((sckt = socket(serverinfo->ai_family, serverinfo->ai_socktype,serverinfo->ai_protocol)) == -1) {
		perror("server: socket");

	}

	if (bind(sckt, serverinfo->ai_addr, serverinfo->ai_addrlen) == -1) {
		close(sckt);
		perror("server: bind");

	}

	freeaddrinfo(serverinfo);

	if (listen(sckt, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	puts("---------WELCOME TO CHATROOM SERVER!------------\n");
	puts("Server : Waiting for connections...\n");

	while(1){

		sin_size = sizeof their_addr;
		newsckt = accept(sckt, (struct sockaddr *)&their_addr, &sin_size);

		if(newsckt == -1) {
			perror("accept");
			continue;
		}

		clients[index].sckt = newsckt;
		clients[index].client_id = index;

		pthread_create(&tid, NULL, ServerHandler, &clients[index++]);

	}

	close(sckt);

	return (0);
}
