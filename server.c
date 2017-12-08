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

	//position next to the last insert hash in the hashtable
	int hashCursor = 0;

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

	while(1)
	{
		//buffer initialization
		buffer *b = new_buffer();

		//hash table initialization
		struct hash hashTable[hashTableSize];

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
			//move the cursor to the next position
			printf("hash : %s\n",hashTable[hashCursor].hash );
			printf("ip : %s\n",hashTable[hashCursor].ip );
			hashCursor++;
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

	return 0;
}
