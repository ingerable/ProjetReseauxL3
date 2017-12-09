/**
 * @file receiver-udp.c
 * @author Julien Montavont
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * Simple program that creates an IPv4 UDP socket and waits for the
 * reception of a string. The program takes a single parameter which
 * is the local communication port. The IPv4 addr associated to the
 * socket will be all available addr on the host (use INADDR_ANY
 * maccro).
 */
#include "dht.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

//globals
//hash table initialization
struct hash hashTable[hashTableSize];

//server table initialization
struct server serverTable[serverTableSize];

//position next to the last insert hash in the hashtable
unsigned int hashCursor = 0;
//same for the serverTable
unsigned int serverCursor = 0;

//function assigned to a thread to execute command for the server
void *serverRequest(void *s)
{
	struct server *myS = s;
	while(1)
	{
		char command[20];
		printf("%s\n","Command for server:" );
		scanf("%s",command);

		//connect command
		if(strcmp(command,"connect")==0)
		{
			//input for the connection to the server
			char ip[128];
			char port[16];
			printf("%s\n","Enter the ip of the server you want to connect");
			scanf("%s",ip);
			printf("%s\n","And the port");
			scanf("%s",port);

			buffer *b = new_buffer();
			serializeChar(b,2);
			serializeShort(b,myS->port);
			sendTo((unsigned short)atoi(port),ip,(unsigned char*)b->data,sizeof(unsigned char)+sizeof(unsigned short));
			free(b);

			//wait and send all our hash to the other server
			sleep(1);
			for (size_t i = 0; i < hashCursor; i++)
			{
				//serializeMessage
				buffer *b1 = new_buffer();
				struct message* ps = malloc(sizeof(message));
				ps->type=1;
				strcpy((char *) ps->hash,(char *)hashTable[i].hash);
				strcpy((char *) ps->ip,(char *)hashTable[i].ip);
				ps->length = strlen((char *)hashTable[i].hash);

			  serializeMessage(ps,b);
				sendTo((unsigned short)atoi(port),ip,(unsigned char*)b->data,sizeof(unsigned short)+ipSize*sizeof(unsigned char)+sizeof(hash));
				free(b1);
				free(ps);
			}
			//finally add this server to our list
			struct server *newServ = malloc(sizeof(newServ));
			newServ->port = (unsigned short)atoi(port);
			memcpy((char*)newServ->ip,&ip,ipSize);
			serverTable[serverCursor] = *newServ;
			printf("Server with ip: %s and port %u added\n",newServ->ip,newServ->port);
		}
		else if(strcmp(command,"disconnect")==0)
		{
			//input for the connection to the server
			char ip[128];
			char port[16];
			printf("%s\n","Enter the ip of the server you want to disconnect");
			scanf("%s",ip);
			printf("%s\n","And the port");
			scanf("%s",port);

			buffer *b = new_buffer();
			serializeChar(b,3);
			serializeShort(b,myS->port);
			sendTo((unsigned short)atoi(port),ip,(unsigned char*)b->data,sizeof(unsigned char)+sizeof(unsigned short));
			free(b);
			exit(1);
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	int sockfd;
	socklen_t addrlen;

	struct sockaddr_in6 my_addr;
	struct sockaddr_in6 client;

	// check the number of args on command line
	if(argc > 5)
	{
		printf("Usage: %s IP port\n", argv[0]);
		exit(-1);
	}

	// socket factory
	if((sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// init local addr structure and other params
		inet_pton(AF_INET6, argv[1], &(my_addr.sin6_addr));
	my_addr.sin6_family      = AF_INET6;
	my_addr.sin6_port        = htons(atoi(argv[2]));
	my_addr.sin6_addr 		= in6addr_any;
	addrlen                 = sizeof(struct sockaddr_in6);

	// bind addr structure with socket
	if(bind(sockfd, (struct sockaddr *) &my_addr, addrlen) == -1)
	{
		perror("bind");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	//thread creation
	pthread_t serv_thread;

	//arguments for the thread function
	struct server *myServ =malloc(sizeof(server));
	memcpy((char*)myServ->ip,(char*)argv[1],ipSize);
	myServ->port = (unsigned short) atoi(argv[2]);

	if(pthread_create(&serv_thread, NULL, serverRequest, myServ) == -1)
	{
	 perror("pthread_create");
	 return EXIT_FAILURE;
	}

	//position next to the last insert hash in the hashtable
	hashCursor = 0;
	//same for the serverTable
	serverCursor = 0;

	 /*
	 *Server waiting on a socket
	 */
	while(1)
	{
		//buffer initialization
		buffer *b = new_buffer();

		// reception de la chaine de caracteres
		if(recvfrom(sockfd, b->data, 1024*sizeof(char), 0
					, (struct sockaddr *) &client, &addrlen) == -1)
		{
			perror("recvfrom");
			close(sockfd);
			exit(EXIT_FAILURE);
		}

		//first get the type of the message
		unsigned char type = (b->data)[0];

		//initialize struct message
		struct message* ps = malloc(sizeof(message));


		//chech the type of the message//
		if(type==0)// get request
		{
			//deserialize buffer to message
			ps = unserializeMessage(b);

			//values that will be send
			unsigned short s= numberOfIp(ps->hash, hashTable);
			unsigned char *ips = malloc(sizeof(char)*s*ipSize);
			ips = ipsForHash(ps->hash, hashTable,s);

			// send the first message to tells how much ips there will be in the packet
		  if(sendto(sockfd, &s, sizeof(short), 0
		  			,  (struct sockaddr *) &client, addrlen) == -1)
		  {
		  	perror("sendto");
		  	close(sockfd);
		  	exit(EXIT_FAILURE);
		  }

			sleep(2);

			//send the second message with all the ips
			if(sendto(sockfd, ips, sizeof(char)*ipSize*s, 0
		  			,  (struct sockaddr *) &client, addrlen) == -1)
		  {
		  	perror("sendto");
		  	close(sockfd);
		  	exit(EXIT_FAILURE);
		  }
		}
		else if(type==1) // put request
		{
			//deserialize buffer to message
			ps = unserializeMessage(b);
			struct hash *h = malloc(sizeof(hash));

			//add the hash from message to our hash struct
			memcpy(h->hash,ps->hash,hashSize);

			//add the ip
			memcpy(h->ip,ps->ip,ipSize);

			//add the hash to the hashtable
			hashTable[hashCursor] = *h;

			//share the new hash with all the registered server

			for (size_t i = 0; i < serverCursor; i++)
			{
				sendTo(serverTable[i].port,(char*)serverTable[i].ip,(unsigned char*)b->data,sizeof(message));
			}
			//move the cursor to the next position
			printf("hash : %s\n",hashTable[hashCursor].hash );
			printf("ip : %s\n",hashTable[hashCursor].ip );
			hashCursor++;
		}
		else if(type==2) //connect request
		{
			struct server *s = malloc(sizeof(server));

			//get the ip adress
			struct sockaddr_in6* pV6Addr = (struct sockaddr_in6*)&client;
			struct in6_addr ipAddr = pV6Addr->sin6_addr;
			char str[INET6_ADDRSTRLEN];
			inet_ntop( AF_INET6, &ipAddr, str, INET6_ADDRSTRLEN );

			//get the port
			b->next++; //move the pointer to the port directly ( we don't care of the type of the message now)
			unsigned short port = unserializeShort(b);
			memcpy((char*)s->ip,&str,ipSize);
			s->port=port;

			//store the new server in our serverTable
			printf("Server with ip: %s and port %u added\n",s->ip,s->port);
			serverTable[serverCursor]=*s;
			serverCursor++;
		}
		else if(type==3)
		{
			struct server *s = malloc(sizeof(server));

			//get the ip adress
			struct sockaddr_in6* pV6Addr = (struct sockaddr_in6*)&client;
			struct in6_addr ipAddr = pV6Addr->sin6_addr;
			char str[INET6_ADDRSTRLEN];
			inet_ntop( AF_INET6, &ipAddr, str, INET6_ADDRSTRLEN );

			//get the port
			b->next++; //move the pointer to the port directly ( we don't care of the type of the message now)
			unsigned short port = unserializeShort(b);
			memcpy((char*)s->ip,&str,ipSize);
			s->port=port;

			//delete the server
			for (size_t i = 0; i < serverTableSize; i++)
			{
				if(strcmp((char*)serverTable[i].ip,(char*)s->ip)==0 && serverTable[i].port==s->port)
				{
					printf("server %s with port %u deleted\n",s->ip,s->port);
				}
			}
		}
		else
		{
			printf("%s\n","unknow message type");
		}

		free(b);
		free(ps);
		// close the socket
		//close(sockfd);
	}
	free(myServ);

	return 0;
}
