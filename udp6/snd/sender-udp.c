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


int main(int argc, char **argv)
{
    int sockfd;
    socklen_t addrlen;
    struct sockaddr_in6 dest;

    // check the number of args on command line
	if(argc != 4)
    {
        printf("USAGE: %s @dest port_num string\n", argv[0]);
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
