/*
 * server1.c
 * 
 * Servidor web basico simple basado en procesos pesados
 * soporta CGI muy basico
 * no soporta FastCGI
 *
 * compilar:
 * $ gcc -Wall -o server1 server1.c util.c -L . -I .
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <util.h>


// parametros globales
clave_valor *param = NULL, *mime = NULL;

// flags globales
int termino = 0, enAccept=0;
sigjmp_buf jmpbuf;

// handle SIGUSR1
void ws_fin(int);
// handle SIGCHLD
void ws_fin_hijo(int);

// atencion a clientes
void ws_cliente(int sdcli,clave_valor *param,clave_valor *mime);

int main(int argc, char **argv) {
	int    rc,sd,sdcli,estado=0;
	char *p;
	//struct sockaddr_un serveraddr;
	struct sockaddr_in serveraddr;
	signal(SIGUSR1,ws_fin);
	signal(SIGCHLD,ws_fin_hijo);
	signal(SIGINT,ws_fin);
	
	// leo parametros del archivo de configuracion
	param = ws_parametros(argc,argv);
	
	printf("server: cargo mime types...\n");
	
	mime = util_nuevo();
	util_cargar_mime(mime,util_buscar(param,"server.mime.types"));
	//util_listar(mime);

	p = util_buscar_default(param,"server.directorio",".");
	chdir(p); // establezco posible nuevo directorio por defecto
	
	printf("server: inicio socket()...\n");
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0) {
		fprintf(stderr,"server Error: socket() error\n");
		exit(-1);
	}
	// seteo opciones socket server
	//const int enable = 1;
	//setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	//setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int));
	
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	p = util_buscar_default(param,"server.puerto","80");
	printf("server: puerto: [%d]\n",atoi(p));
	serveraddr.sin_port = htons(atoi(p));
	p = util_buscar_default(param,"server.ip","127.0.0.1");
	rc = inet_aton(p, &serveraddr.sin_addr);
	if ( rc == 0 ) {
		fprintf(stderr,"server Error: inet_aton() error ip[%s]\n",p);
		close(sd);		
		exit(-2);
	}

	printf("server: inicio bind() sd=%d...\n",sd);
	rc = bind(sd, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
	if (rc < 0)	{
		fprintf(stderr,"server Error: bind() error errno=%d\n",errno);
		close(sd);		
		exit(-2);
	}
	
	printf("server: inicio listen()...\n");
	p = util_buscar_default(param,"server.cola","1024");
	rc = listen(sd,atoi(p));
	if (rc < 0) {
		fprintf(stderr,"server Error: listen() error %d\n",errno);
		close(sd);		
		exit(-3);
	}

	printf("server: esperando conexiones...\n");

	if (sigsetjmp(jmpbuf,1)) {
		printf("server: Recibo salto hacia salida!\n");
	}
	
	while(!termino) {
		printf("server: Bloqueado en accept\n");
		enAccept = 1;
			sdcli = accept(sd, NULL, NULL);
		enAccept = 0;
		printf("server: fin accept()\n");
		if ( !termino && sdcli > 0) {
			pid_t pid = fork();
			if ( pid == 0 ) {
				signal(SIGUSR1,SIG_DFL);
				signal(SIGCHLD,SIG_DFL);
				signal(SIGINT,SIG_DFL);
				ws_cliente(sdcli,param,mime);
				exit(sdcli);
			} 
		}
	} 
	
	printf("server: comienzo salida...\n");
	if ( sd > 0 ) close(sd);

	printf("server: espero por finalizacion de procesos hijos...\n");
	while(wait(&estado) != -1) printf("server: ws_cliente(%d) fin!\n",estado/256);

	printf("server: borro lista de parametros...\n");
	util_borrar(param);
	
	printf("server: fin!\n");
	return 0;
}

// funcion ejecutada en proceso hijo
void ws_cliente(int sdcli,clave_valor *param,clave_valor *mime) {
	printf("ws_client(%d): procesando cliente!\n",sdcli);
	if ( termino ) { close(sdcli);return; }

	int bloque=1024,tamanio=bloque,i,rc;
	char *buffer = (char *) malloc(bloque);
	memset(buffer,0,bloque);
	// recibo peticion
	i=0;
	do {
		errno=0;
		rc = recv(sdcli,buffer+i,bloque,0);
		printf("ws_cliente(%d): rc=[%d] errno=[%d] i=[%d]\n",sdcli,rc,errno,i);
		if ( rc == -1 ) {
			printf("ws_cliente(%d): error! leyendo socket rc=[%d] errno=[%d][%s]\n",sdcli,rc,errno,strerror(errno));
			close(sdcli);
			printf("ws_cliente(%d): fin! (2)\n",sdcli);
			return;
		}
		if ( rc >= bloque ) {
			tamanio+=bloque;
			buffer = (char *) realloc(buffer,tamanio);
			memset(buffer+tamanio-bloque,0,bloque);
			i+=rc;
		}
	} while(rc >= bloque);
	printf("ws_cliente(%d): lei buffer=[%s] strlen(buffer)=[%ld] rc=[%d] \n",sdcli,buffer,strlen(buffer),rc);
	if ( rc == 0 ) {
		printf("ws_cliente(%d): lectura vacia de socket rc=[%d]\n",sdcli,rc);
		close(sdcli);
		printf("ws_cliente(%d): fin! (2)\n",sdcli);
		return;
	}
	// parsing peticion
	printf("ws_cliente(%d): parsing peticion\n",sdcli);
	peticion *pet = util_peticion(buffer,param);
	respuesta *res = util_respuesta();
	printf("ws_cliente(%d): proceso peticion\n",sdcli);
	util_procesar(sdcli,param,mime,pet,res);
	printf("ws_cliente(%d): envio respuesta\n",sdcli);
	util_enviar_respuesta(sdcli,res);
	printf("ws_cliente(%d): fin envio respuesta\n",sdcli);

	free(buffer);
	printf("ws_cliente(%d): borro lista peticion\n",sdcli);
	util_borrar_pet(pet);
	printf("ws_cliente(%d): borro lista respuesta\n",sdcli);
	util_borrar_res(res);
	
	close(sdcli);
	printf("ws_cliente(%d): fin!\n",sdcli);
	return;
}

/**
 * handler de se#al SIGUSR1
 * permito finalizar servidor desde el exterior enviando SIGUSR1
 */
void ws_fin(int signo) {
	if (termino) return; 
	termino=1;
	siglongjmp(jmpbuf,1);  // salto al final del servidor
}

/**
 * handler de se#al SIGCHLD
 * me entero de cuando finalizo un proceso hijo
 */
void ws_fin_hijo(int signo) {
	printf("server: ws_fin_hijo(): finalizo proceso hijo!\n");
	printf("server: ws_fin_hijo(): espero por finalizacion de procesos hijos...\n");
	int estado=0;
	while(wait(&estado) != -1) printf("server: ws_fin_hijo(): fin ws_cliente(%d) !\n",estado/256);
	printf("server: ws_fin_hijo(): fin!\n");
}


