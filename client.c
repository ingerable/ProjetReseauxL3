/**
 * @file sender-udp.c
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
 * Simple program that creates an IPv4 UDP socket and sends a string
 * to a remote host. The string, IPv4 addr and port number of the
 * remote host are passed as command line parameters as follow:
 * ./pg_name IPv4_addr port_number string
 */

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
#include "client.h"
#include "dht.h"

int main(int argc, char **argv)
{
    int sockfd;
    socklen_t addrlen;
    struct sockaddr_in6 dest;

    // check the number of args on command line
	if(argc > 6 || argc < 5)
    {
        printf("USAGE: %s IP PORT COMMANDE HASH [IP]\n", argv[0]);
        exit(-1);
    }

    //declare and initialize the struct message
    struct message* ps = malloc(sizeof(message));

    //add the type to our message
    if(strcmp(argv[3],"get")==0)
    {
      ps->type=0;
    }
    else if(strcmp(argv[3],"put")==0)
    {
      ps->type=1;
      //add the ip to our struct message
      strcpy((char *) ps->ip,argv[5]);
    }

	// socket factory
	if((sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}

  // init remote addr structure and other params
	dest.sin6_family = AF_INET6;
	dest.sin6_port   = htons(atoi(argv[2]));
	addrlen         = sizeof(struct sockaddr_in6);

	// get addr from command line and convert it
  if(strcmp(argv[1],"localhost")==0)
  {
    if(inet_pton(AF_INET6, "::1", &dest.sin6_addr) != 1)
  	{
  		perror("inet_pton");
  		close(sockfd);
  		exit(EXIT_FAILURE);
  	}
  }
  else
  {
    if(inet_pton(AF_INET6, argv[1], &dest.sin6_addr) != 1)
  	{
  		perror("inet_pton");
  		close(sockfd);
  		exit(EXIT_FAILURE);
  	}
  }


  //add hash to message struct
  strcpy((char *) ps->hash,argv[4]);

  //add the length of the hash to our struct message
  ps->length= (unsigned short) strlen(argv[4]);


  //creation of the buffer
  buffer *b = new_buffer();

  //serializeMessage
  serializeMessage(ps,b);

  // send string
  	if(sendto(sockfd, b->data, 1024*sizeof(char), 0
  				,  (struct sockaddr *) &dest, addrlen) == -1)
  	{
  		perror("sendto");
  		close(sockfd);
  		exit(EXIT_FAILURE);
  	}

    //get type, so the client is waiting for a response
    if(ps->type==0)
    {
      // client is waiting for the socket that tells the number of ip incoming
      unsigned short nbrOccurences;
      if(recvfrom(sockfd, &nbrOccurences, sizeof(unsigned short), 0
            , (struct sockaddr *) &dest, &addrlen) == -1)
      {
        perror("recvfrom");
        close(sockfd);
        exit(EXIT_FAILURE);
      }

      //now we will receive all the ip we requested for the hash
      unsigned char *ips=malloc(sizeof(char)*ipSize*nbrOccurences);
      if(recvfrom(sockfd, ips, sizeof(char)*ipSize*nbrOccurences, 0
            , (struct sockaddr *) &dest, &addrlen) == -1)
      {
        perror("recvfrom");
        close(sockfd);
        exit(EXIT_FAILURE);
      }

      /*
      * Print all the matching IP for the requested hash
      */
      printIP6(ips, nbrOccurences,ps->hash);

    }

    //free the memory
		free(b);
		free(ps);

	// close the socket
	close(sockfd);

	return 0;
}
