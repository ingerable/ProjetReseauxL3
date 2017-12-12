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
#include <sys/time.h>
#include <pthread.h>

//globals

//hash table ,size,cursor
struct hash *hashTable=NULL;
unsigned int hashTableSize = 100;
unsigned int hashCursor = 0;

//server table
struct server *serverTable=NULL;
unsigned int serverTableSize = 100;
unsigned int serverCursor = 0;

//my server
struct server *global_myserv = NULL;

//wait for main thread to give it uptodate notification meanwhile timing out
void *obsoleteReceiver(void *h)
{
  struct hash *bindedHash = (struct hash*)h;
  unsigned int time=0;
  while(time<30)
  {
    if(bindedHash->uptodate==1)
    {
      printf("Up to date notification, restart timer...\n");
      bindedHash->uptodate=0;//reinit of the notification
      time=0;
    }
    else
    {
      sleep(1);
      time++;
    }
  }
  //delete hash
  deleteHash(hashTable,&hashCursor,bindedHash,&hashTableSize);
  printf("Hash %s timed out\n",bindedHash->hash);
  return 0;
}


//wait for main thread to give it ka notification meanwhile timing out
void *keepAliveReceiver(void *s)
{
  struct server *bindedServ = (struct server*)s;
  unsigned int time=0;
  while(time<timeout)
  {
    if(bindedServ->ka==1)
    {
			printf("Keep alive notification, restart timer...\n");
      bindedServ->ka=0;//reinit of the notification
      time=0;
    }
    else
    {
      sleep(1);
      time++;
    }
  }
	//delete the server
	deleteServer(serverTable,&serverCursor,bindedServ,&serverTableSize);
	printf("Server %s connection timed out\n", bindedServ->ip);
	bindedServ->ka=99;//notify sender that server is deleted from table
	return 0;
}

//send ka message to the binded server
void *keepAliveSender(void *s)
{
	struct server *bindedServ = s;
	unsigned int time=0;
	while(bindedServ->ka!=99)//while server is in table and connected
	{
		if(time==5)
		{
			time=0;
			buffer *b = new_buffer();
			serializeChar(b,5);
			serializeShort(b,global_myserv->port);
			//send keep alive message
			sendTo((unsigned short)bindedServ->port,(char*)bindedServ->ip,(unsigned char*)b->data,sizeof(unsigned char)+sizeof(unsigned short));
			free(b);
		}
		else
		{
			sleep(1);
			time++;
		}
	}
	return 0;
}

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
				ps->type=4;
				strcpy((char *) ps->hash,(char *)hashTable[i].hash);
				strcpy((char *) ps->ip,(char *)hashTable[i].ip);
				ps->length = strlen((char *)hashTable[i].hash);

			  serializeMessage(ps,b1);
				sendTo((unsigned short)atoi(port),ip,(unsigned char*)b1->data,sizeof(char)+sizeof(unsigned short)+ipSize*sizeof(unsigned char)+sizeof(unsigned char)*ps->length);
				free(b1);
				free(ps);
			}

			//finally add this server to our list
			struct server *newServ = malloc(sizeof(newServ));
			newServ->port = (unsigned short)atoi(port);
			memcpy((char*)newServ->ip,&ip,ipSize);
			newServ->ka=0;

			//store the new server in our serverTable
			newServ = addServer(serverTable,&serverCursor,newServ,&serverTableSize);

			/*
			start the keep alive engine
			*/
			//thread creation
			pthread_t keep_aliveR;
			pthread_t keep_aliveS;

			//thread keep alive receiver
			if(pthread_create(&keep_aliveR, NULL, keepAliveReceiver, newServ) == -1)
			{
			 perror("pthread_create");
			 //return EXIT_FAILURE;
			}

			//thread keep alive sender
			if(pthread_create(&keep_aliveS, NULL, keepAliveSender, newServ) == -1)
			{
			 perror("pthread_create");
			 //return EXIT_FAILURE;
			}
		}
		else if(strcmp(command,"disconnect")==0)
		{
			//input for the connection to the server
			for (size_t i = 0; i < serverCursor; i++)
			{
				buffer *b = new_buffer();
				serializeChar(b,3);
				serializeShort(b,myS->port);
				sendTo(serverTable[i].port,(char *)serverTable[i].ip,(unsigned char*)b->data,sizeof(unsigned char)+sizeof(unsigned short));
				free(b);
			}
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
	global_myserv = myServ;

	if(pthread_create(&serv_thread, NULL, serverRequest, myServ) == -1)
	{
	 perror("pthread_create");
	 return EXIT_FAILURE;
	}

	//memory allocation
	hashTable = malloc(sizeof(struct hash)*hashTableSize);
	serverTable = malloc(sizeof(struct server)*serverTableSize);

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


		//check the type of the message//
		if(type==0)// get request
		{
			//deserialize buffer to message
			ps = unserializeMessage(b);

			//values that will be send
			unsigned short s= numberOfIp(ps->hash, hashTable,&hashTableSize);
			unsigned char *ips = malloc(sizeof(char)*s*ipSize);
			ips = ipsForHash(ps->hash, hashTable,s,&hashTableSize);

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

      //init the up to date data
      h->uptodate=0;

      if(hashExist(hashTable,h,&hashTableSize)!=0)
      {
        //add the hash to the hashtable
  			h = addHash(hashTable,&hashCursor,h,&hashTableSize);

  			//share the new hash with all the registered server( but we have to change the type of the message first to PUT request from server)
  			ps->type=4;
  			buffer *b = new_buffer();
  			serializeMessage(ps,b);

  			for (size_t i = 0; i < serverCursor; i++)
  			{
  				sendTo(serverTable[i].port,(char*)serverTable[i].ip,(unsigned char*)b->data,sizeof(message));
  			}

        /*
  			start the uptodate hash engine
  			*/
  			//thread creation
  			pthread_t obsoleteR;

  			//thread obsolete receiver
  			if(pthread_create(&obsoleteR, NULL, obsoleteReceiver, h) == -1)
  			{
  			 perror("pthread_create");
  			 return EXIT_FAILURE;
  			}
      }
      else
      {
        printf("Hash %s w/ IP %s already exists\n",h->hash,h->ip);
      }

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
			s->ka=0;

			//store the new server in our serverTable
			s = addServer(serverTable,&serverCursor,s,&serverTableSize);

			/*
			start the keep alive engine
			*/
			//thread creation
			pthread_t keep_aliveR;
			pthread_t keep_aliveS;

			//thread keep alive receiver
			if(pthread_create(&keep_aliveR, NULL, keepAliveReceiver, s) == -1)
			{
			 perror("pthread_create");
			 return EXIT_FAILURE;
			}

			//thread keep alive sender
			if(pthread_create(&keep_aliveS, NULL, keepAliveSender, s) == -1)
			{
			 perror("pthread_create");
			 return EXIT_FAILURE;
			}

		}
		else if(type==3)//a server want to disconnect
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
			deleteServer(serverTable,&serverCursor,s,&serverTableSize);

		}
		else if(type==4)//PUT request but from server (you can't treat it like a client PUT request, because there will be brodcast tempest)
		{

			//deserialize buffer to message
			ps = unserializeMessage(b);
			struct hash *h = malloc(sizeof(hash));

			//add the hash from message to our hash struct
			memcpy(h->hash,ps->hash,hashSize);

			//add the ip
			memcpy(h->ip,ps->ip,ipSize);

      if(hashExist(hashTable,h,&hashTableSize)!=0)
      {
        //add the hash to the hashtable
  			h = addHash(hashTable,&hashCursor,h,&hashTableSize);

        /*
  			start the uptodate hash engine
  			*/
  			//thread creation
        pthread_t obsoleteR;

        //thread obsolete receiver
        if(pthread_create(&obsoleteR, NULL, obsoleteReceiver, h) == -1)
        {
         perror("pthread_create");
         return EXIT_FAILURE;
        }
      }
      else
      {
        printf("Hash %s w/ IP %s already exists\n",h->hash,h->ip);
      }

		}
		else if(type==5)//Keep alive from server
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

			printf("Keep alive message from %s port : %d ...\n",s->ip,s->port);

			//keep alive advertisement for the binded server thread
			adKeepAlive(serverTable,&serverCursor,s);
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
