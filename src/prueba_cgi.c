/*
 * prueba_cgi.c
 * 
 * Copyright 2023 osboxes <osboxes@osboxes>
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
 * programa que sirve para probar script CGI desde linea de comandos
 * los ejecuta de tipo GET	
 * 
 * compilar:
 * $ gcc -Wall -o prueba_cgi prueba_cgi.c
 * 
 */
#define _GNU_SOURCE  // habilita funciones como strcasestr() etc
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024

void mayusculas(char *buffer);

int main(int argc, char **argv)
{
	if ( argc != 3 ) {
		printf("Forma de Uso:\n$ ./prueba_cgi <nombre ejecutable CGI> <metodo {GET|POST}>\n");
		return 1;
	}
	// primer argumento nombre del cgi
	char cmd[1024],qry[1024], qry2[1200], metodo[100], content[50];
	snprintf(cmd,1024,"%s",argv[1]);
	mayusculas(argv[2]);
	snprintf(metodo,100,"%s",argv[2]);

	printf("metodo: %s\n",metodo);

	printf("Indique Query String:");
	fgets(qry,1024,stdin);

	if ( strcmp(metodo,"GET") == 0 ) {
		// metodo GET  pasa parametros por variable QUERY_STRING
		
		qry[strlen(qry)-1]='\0';
		snprintf(content,50,"CONTENT_LENGTH=%ld",strlen(qry));
		
		sprintf(qry2, "QUERY_STRING=%s", qry);
		char *envp[] = { "REQUEST_METHOD=GET", qry2, content, NULL };
		char *tests[] = { cmd, NULL };
		execvpe(tests[0], tests, envp);
		fprintf(stderr, "Error ejecutando \"%s\"\n", tests[0]);

	} else {
		// metodo POST pasa parametros por stdin
		snprintf(content,50,"CONTENT_LENGTH=%ld",strlen(qry)-1);

		char *envp[] = { "REQUEST_METHOD=POST", content, NULL };
		char *tests[] = { cmd, NULL };
		
		int fd1[2],fd2[2]; 
		pipe(fd1);
		pipe(fd2);
		pid_t pid = fork();
		if (pid == 0) { // proceso hijo
			close(fd1[1]);
			close(fd2[0]);
			dup2(fd1[0], STDIN_FILENO);  // dup2(fd1[0],0);
			dup2(fd2[1], STDOUT_FILENO); // dup2(fd2[1],1);
			execvpe(tests[0], tests, envp);
			fprintf(stderr, "Error ejecutando \"%s\"\n", tests[0]);
			
		} else { // proceso padre
			close(fd1[0]);
			close(fd2[1]);
			/* Escribo entrada */
			int lei = 0;
			char linea[BUFFER_SIZE];
			write(fd1[1],qry,strlen(qry)); // escribo entrada
			/* Leo lineas de salida del comando */
			// leo la salida
			do {
				lei = read(fd2[0],linea,BUFFER_SIZE-1);
				if ( lei >= 0 ) {
					linea[lei]='\0';
					printf("%s",linea);
				}
			} while(lei > 0);
			wait(0);
		}	
		
	}
	return 0;
}

// pasa buffer a mayusculas
void mayusculas(char *buffer) {
	while(*buffer) {
		*buffer = toupper(*buffer);
		buffer++;
	}
}


