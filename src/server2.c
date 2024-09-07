/*
 * server2.c
 * 
 * Servidor web basico simple basado en hilos
 * soporta CGI muy basico
 * no soporta FastCGI
 * soporta nueva tecnologia grchere para desarrollo web!
 *
 * compilar:
 * $ gcc -Wall -o server2 server2.c util.c -L . -I . -lpthread -ldl
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
#include <limits.h> // definicion PTHREAD_STACK_MIN
#include <pthread.h>
#include <dlfcn.h> // carga de librerias dinamicas
#include <util.h>

#define THREAD_STACK_SIZE PTHREAD_STACK_MIN+20480 //65536  el minimo requerido para iniciar un thread + 20K
int THREAD_POOL     = 1024;
pthread_t *thrpool  = NULL;
int *intpool        = NULL;

// agrego al pool int, NULL si pool esta lleno
int *ws_agregoInt();
void ws_quitoInt(int *);
// agrego hilo al pool de hilos
void ws_agregoHilo(pthread_t thr);
// quito hilo del pool de hilos
void ws_quitoHilo(pthread_t thr);
// cancelo hilos del pool
void ws_cancelo();

// parametros globales
clave_valor *param = NULL, *mime = NULL;

// flags globales
int termino = 0, enAccept=0;
sigjmp_buf jmpbuf;

// handle SIGUSR1
void ws_fin(int);
// handle SIGCHLD
void ws_fin_hijo(int);
// handle SIGALRM
void ws_alarm(int signo);

// pool de aplicaciones
aplicacion *apps = NULL;
int napps = 0;

// hilo atencion a clientes
void *ws_cliente(void *socket);

int main(int argc, char **argv) {
	int    i,rc,sd,sdcli,estado=0;
	char *p;
	//
	pthread_t thread_id;
	
	
	//struct sockaddr_un serveraddr;
	struct sockaddr_in serveraddr;
	
	//senales
	signal(SIGUSR1,ws_fin);
	signal(SIGCHLD,ws_fin_hijo);
	signal(SIGINT,ws_fin);
	signal(SIGALRM,ws_alarm);
	
	// leo parametros del archivo de configuracion
	printf("server: cargo parametros...\n");
	param = ws_parametros(argc,argv);
	
	// creo pool de enteros y pool de hilos
	printf("server: creo pool de hilos y enteros...\n");
	p = util_buscar_default(param,"server.pool","1024");
	THREAD_POOL = atoi(p);
	
	intpool = (int *) malloc(sizeof(int)*THREAD_POOL);
	memset(intpool,0,sizeof(int)*THREAD_POOL);
	thrpool	= (pthread_t *) malloc(sizeof(pthread_t)*THREAD_POOL);
	memset(thrpool,0,sizeof(pthread_t)*THREAD_POOL);	

	printf("server: inicio atributos de threads...\n");
	pthread_attr_t attr;
	if (pthread_attr_init(&attr) != 0) {
		printf("server: Error en inicializar atributos pthread\n");
		exit(-3);
	}
	// stack size (minimo 2MB en linux!!)
	if (pthread_attr_setstacksize(&attr,THREAD_STACK_SIZE) != 0) {
		printf("server: Error seteando stack size\n");
		exit(-4);
	}
	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) {
		printf("server: Error seteando detach state\n");
		exit(-5);
	}
	printf("server: fin atributos de threads\n");
	
	printf("server: cargo mime types...\n");
	
	mime = util_nuevo();
	util_cargar_mime(mime,util_buscar(param,"server.mime.types"));
	//util_listar(mime);

	p = util_buscar_default(param,"server.dyn.apps","none");
	if ( strcmp(p,"none") != 0 ) {
		printf("server: hay aplicaciones declaradas!..\n");
		napps=0;
		// cuento la cantidad de aplicaciones declaradas
		i=0;while(p[i]) { if ( p[i] == ',' ) napps++;i++; }
		napps++;
		printf("server: hay %d aplicaciones declaradas!..\n",napps);
		apps = (aplicacion *) malloc(sizeof(aplicacion)*napps);
		memset(apps,0,sizeof(aplicacion)*napps);
		// parsing de la declaracion de aplicaciones
		int a=0,z;
		i=0;
		while(p[i]) {
			// app : libreria : url , app : libreria : url ...
			// app
			z=0;
			while(p[i] && p[i] != ':' && z < 32) { apps[a].nombre[z] = p[i];z++;i++; }
			apps[a].nombre[z] = '\0';
			// libreria
			i++;z=0;
			while(p[i] && p[i] != ':' && z < 256) { apps[a].libreria[z] = p[i];z++;i++; }
			apps[a].libreria[z] = '\0';
			// url
			i++;z=0;
			while(p[i] && p[i] != ',' && z < 256) { apps[a].url[z] = p[i];z++;i++; }
			apps[a].url[z] = '\0';
			i++;
			a++; // siguiente app
		}
		aplicacion *aa = apps;
		for(i=0;i<napps;i++,aa++) {
			printf("server: app [%s] libreria [%s] url [%s]\n",aa->nombre,aa->libreria,aa->url);
			// proceso, si existe, archivo configuracion de app
			aa->param = ws_parametros_archivo(aa->nombre);

			aa->lib = dlopen (aa->libreria, RTLD_NOW);
			if ( !aa->lib ) {
				printf("server: app [%s] Error [%s] abriendo libreria [%s]\n",aa->nombre,dlerror(),aa->libreria);
			} else {
				// linkeo funciones
				aa->f_ini = dlsym(aa->lib, "app_ini");
				if ( !aa->f_ini ) printf("server: app [%s] Error [%s] vinculando funcion app_ini()\n",aa->nombre,dlerror());
				else {
					aa->buffer = aa->f_ini(param,aa->param,&aa->largo_buffer);
					printf("server: app [%s] ejecute app_ini buffer=[%p] largo_buffer=[%ld]\n",aa->nombre,aa->buffer,aa->largo_buffer);
				}
				aa->f_fin = dlsym(aa->lib, "app_fin");
				if ( !aa->f_fin ) printf("server: app [%s] Error [%s] vinculando funcion app_fin()\n",aa->nombre,dlerror());
				aa->f_get = dlsym(aa->lib, "app_get");
				if ( !aa->f_get ) printf("server: app [%s] Error [%s] vinculando funcion app_get()\n",aa->nombre,dlerror());
			}
		}
	}

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

	alarm(10);
	
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
			int *pparam = ws_agregoInt();
			memcpy(pparam,&sdcli,sizeof(int));
			printf("server: p=%p\n",pparam);
			int ptret = pthread_create(&thread_id,&attr,ws_cliente,pparam);
			if( ptret != 0) {
				char buferr[300];
				memset(buferr,0,300);
				strerror_r(errno,buferr,299);
				fprintf(stderr,"server: Error, no puedo crear hilo ret=%d errno=%d [%s]\n",ptret,errno,buferr);
				close(sdcli);
				exit(-4);
			}		
		}
	} 
	
	printf("server: comienzo salida...\n");
	if ( sd > 0 ) close(sd);

	printf("server: espero por finalizacion de procesos hijos...\n");
	while(wait(&estado) > 0) printf("server: ws_cliente(%d) fin!\n",estado/256);

	printf("server: cancelo hilos...\n");
	ws_cancelo();

	printf("server: borro lista de parametros...\n");
	util_borrar(param);

	printf("server: borro pool de hilos...\n");
	if ( thrpool ) free(thrpool);
	if ( intpool ) free(intpool);
	if ( apps ) {
		aplicacion *aa = apps;
		for(i=0;i<napps;i++,aa++) {
			if ( aa->f_fin ) {
				aa->f_fin(aa->buffer,&aa->largo_buffer);
				printf("server: app [%s] ejecute app_fin\n",aa->nombre);
			}
			if ( aa->lib )  dlclose(aa->lib);
		}
		free(apps);
		apps = NULL;
		napps = 0;
	}
	
	printf("server: fin!\n");
	return 0;
}

// hilo que atiende a cliente
void *ws_cliente(void *socket) {
	pthread_detach(pthread_self());
	int sdcli = *(int *) socket;
	printf("ws_client(%d): procesando cliente!\n",sdcli);
	if ( termino ) { 
		close(sdcli);
		ws_quitoInt(socket);
		pthread_exit(0);
	}

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
			ws_quitoInt(socket);
			pthread_exit(0);
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
		ws_quitoInt(socket);
		pthread_exit(0);
	}
	// parsing peticion
	printf("ws_cliente(%d): parsing peticion\n",sdcli);
	peticion *pet = util_peticion(buffer,param);
	respuesta *res = util_respuesta();
	printf("ws_cliente(%d): proceso peticion\n",sdcli);
	// chequeo si la peticion es una peticion de una aplicacion
	int app_procesado = 0;
	aplicacion *aa = apps;
	for(i=0;!app_procesado && i<napps;i++,aa++) {
		if ( aa->f_get && strncmp(pet->recurso,aa->url,strlen(aa->url)) == 0 ) {
			aa->f_get(aa->buffer,&aa->largo_buffer,param,aa->param,pet,res);
			printf("server: app [%s] ejecute app_get url=[%s] url_app=[%s]\n",aa->nombre,pet->recurso,aa->url);
			app_procesado = 1;
		}
	}	
	if ( !app_procesado ) util_procesar(sdcli,param,mime,pet,res);
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
	ws_quitoInt(socket);
	pthread_exit(0);
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
	while(wait(&estado) > 0) printf("server: ws_fin_hijo(): fin ws_cliente(%d) !\n",estado/256);
	printf("server: ws_fin_hijo(): fin!\n");
}


// agrego hilo al pool de hilos
void ws_agregoHilo(pthread_t thr) {
	pthread_t *p = thrpool;int i;
	for(i=0;i<THREAD_POOL && *p;i++,p++);
	if(i<THREAD_POOL) { 
		*p = thr;
	} else { // rayos!! se lleno pool!
		printf("ws_agregoHilo(): Error, thread pool lleno, debe ampliar parametro thread.pool\n");
	}
}
// quito hilo del pool de hilos
void ws_quitoHilo(pthread_t thr) {
	pthread_t *p = thrpool;int i;
	for(i=0;i<THREAD_POOL;i++,p++) {
		if (*p == thr) {
			*p = 0;
			break;
		}
	}
}
int *ws_agregoInt() {
	int *p = intpool;int i;
	for(i=0;i<THREAD_POOL && *p;i++,p++);
	if(i<THREAD_POOL) { 
		return p;
	} else { // rayos!! se lleno pool!
		printf("ws_agregoInt(): Error, int pool lleno\n");
		return NULL;
	}
}

void ws_quitoInt(int *p) {
	*p=0;
}

// cancelo hilos del pool
void ws_cancelo() {
	pthread_t *p = thrpool;int i,n=0;
	for(i=0;i<THREAD_POOL;i++,p++) {
		if (*p) {
			printf("ws_cancelo(): intento cancelar hilo %lu\n",*p);
			pthread_cancel(*p);
			*p = 0;
			n++;
		}
	}
	printf("ws_cancelo(): termine %d hilos del pool\n",n);
}
// handler alarma
void ws_alarm(int signo) {
	pthread_t *p = thrpool;int i,n;
	for(i=0,n=0;i<THREAD_POOL;i++,p++) {
		if ( *p ) n++;
	}
	printf("server: Hay %d hilos activos\n",n);
	alarm(10);
}

