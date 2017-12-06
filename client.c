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
#include "server.h"
#include "client.h"

unsigned short int addToBuff(char *bufCursor,char *s,unsigned short int length,unsigned short int cursor)
{
  for (unsigned short int i= 0; i < length; i++)
  {
    bufCursor[cursor+i] = s[i];
  }
  cursor = cursor + length+1;
  return cursor;
}


int main(int argc, char **argv)
{
    int sockfd;
    socklen_t addrlen;
    struct sockaddr_in6 dest;

    // check the number of args on command line
	if(argc != 5)
    {
        printf("USAGE: %s IP PORT COMMANDE HASH [IP]\n", argv[0]);
        exit(-1);
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
	if(inet_pton(AF_INET6, argv[1], &dest.sin6_addr) != 1)
	{
		perror("inet_pton");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

  //prepare buffer
  char buf[1024];
  unsigned short int cursor=0;

  cursor = addToBuff(buf,argv[3],3,cursor);
  printf("%s\n",buf );
  cursor = addToBuff(buf,argv[4],(unsigned)strlen(argv[4]),cursor);
  printf("%s\n",buf );

	// send string
	if(sendto(sockfd, argv[3], strlen(argv[3]), 0
				,  (struct sockaddr *) &dest, addrlen) == -1)
	{
		perror("sendto");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	// close the socket
	close(sockfd);

	return 0;
}

char *addField(char *buf, const char *field)
{
    while(*field!='\0')
    {
      buf++;
      *buf = *field;
      field++;
    }
    *buf='\0';
    buf++;
    return buf;
}
