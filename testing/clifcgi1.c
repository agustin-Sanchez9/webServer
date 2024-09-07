/*
 * clifcgi1.c
 * 
 * Copyright 2024 osboxes <osboxes@osboxes>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>

#define MAX 1024
#define PORT 8282
#define SA struct sockaddr

void func(int sockfd);
 
int main(int argc,char *argv[]) {
    int sockfd;
    struct sockaddr_in servaddr;
 
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));
 
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);
 
    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr))
        != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
    else
        printf("connected to the server..\n");
 
    // function for chat
    func(sockfd);
 
    // close the socket
    close(sockfd);
}

void func(int sockfd)
{
    char buff[MAX];
    int n;
    for (;;) {
        bzero(buff, sizeof(buff));
        n = 0;
        strcpy(buff,"POST /fcgi1 HTTP/1.1\r\nContent-Type: text/xml; charset=utf-8\r\nContent-Length: 19\r\nConnection: Keep-Alive\r\n\r\nfirst=Zara&last=Ali");
        
        printf("strlen(buff)=%ld\n",strlen(buff));
        n=write(sockfd, buff, strlen(buff));
        printf("envie %d bytes al servidor!\n",n);
        bzero(buff, sizeof(buff));
        n = read(sockfd, buff, sizeof(buff));
        printf("Lei %d bytes From Server : [%s]",n,buff);

		printf("espero 2 segundos y vuelvo a enviar...\n");
		sleep(2);
        n=write(sockfd, buff, strlen(buff));
        printf("envie %d bytes al servidor!\n",n);
        bzero(buff, sizeof(buff));
        n = read(sockfd, buff, sizeof(buff));
        printf("Lei %d bytes From Server : [%s]",n,buff);

        break;
    }
}
