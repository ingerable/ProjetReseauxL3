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
	char buf[1024];
	socklen_t addrlen;

	struct sockaddr_in6 my_addr;
	struct sockaddr_in6 client;

	// check the number of args on command line
	if(argc != 2)
	{
		printf("Usage: %s local_port\n", argv[0]);
		exit(-1);
	}

	// socket factory
	if((sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// init local addr structure and other params
	my_addr.sin6_family      = AF_INET6;
	my_addr.sin6_port        = htons(atoi(argv[1]));
	my_addr.sin6_addr 		= in6addr_any;
	addrlen                 = sizeof(struct sockaddr_in6);
	memset(buf,'\0',1024);

	// bind addr structure with socket
	if(bind(sockfd, (struct sockaddr *) &my_addr, addrlen) == -1)
	{
		perror("bind");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	// reception de la chaine de caracteres
	if(recvfrom(sockfd, buf, 1024, 0
				, (struct sockaddr *) &client, &addrlen) == -1)
	{
		perror("recvfrom");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	// print the received char
	printf("%s", buf);

	// close the socket
	close(sockfd);

	return 0;
}
