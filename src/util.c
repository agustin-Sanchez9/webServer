#include <util.h>

//funciones privadas para reuso de codigo
// copia las cabeceras que devuelve el proceso CGI en res
void util_procesar_cabecera(respuesta *res,char *buffer,int tamanio,char *metodo);
// lee la salida del comando CGI y devuelve el buffer de memoria dinamica con la salida
char *util_leer_salida(int fd,int *tamanio);

// nuevo nodo lista
clave_valor *util_nuevo() {
	clave_valor *p = (clave_valor *) malloc(sizeof(clave_valor));
	memset(p,0,sizeof(clave_valor));
	return p;
}

// borra lista
void util_borrar(clave_valor *lista) {
	if ( !lista ) return;
	while(lista) {
		if ( lista->clave ) free(lista->clave);
		if ( lista->valor ) free(lista->valor);
		clave_valor *p = lista;
		lista = lista->siguiente;
		free(p);
	}
}
// agregar a la lista
void util_agregar(clave_valor *lista,char *clave,char *valor) {
	if ( !lista || !clave || !valor ) return;
	if ( lista->clave ) { 
		clave_valor *p = util_nuevo();
		memcpy(p,lista,sizeof(clave_valor));
		
		lista->clave = strdup(clave);
		lista->valor = strdup(valor);
		
		lista->siguiente = p;
	} else {

		lista->clave = strdup(clave);
		lista->valor = strdup(valor);
	}
}
// agrega "Content-Length" en lista con el valor valor
// se asegura que no se repita  "Content-Length" en la lista
void util_agregar_content(clave_valor *lista,long valor) {
	util_borrar_clave(lista,"Content-Length");
	char tmp[50];
	snprintf(tmp,50,"%ld",valor);
	util_agregar(lista,"Content-Length",tmp);
}

// borrar elemento de la lista
void util_borrar_clave(clave_valor *lista,char *clave) {
	if ( !lista || !clave ) return;
	clave_valor	*alista = lista, *p = lista;
	while(p) {
		if ( p->clave && strcmp(p->clave,clave) == 0 ) {
			if ( !p->siguiente ) {
				if ( p == lista ) {
					// es el primer elemento de la lista y no tiene siguiente
					if ( p->clave ) free(p->clave);
					if ( p->valor ) free(p->valor);
					p->clave = NULL;
					p->valor = NULL;
					return;
				} else {
					// no es el primer elemento de la lista y no tiene siguiente -> es el ultimo elemento de la lista
					if ( p->clave ) free(p->clave);
					if ( p->valor ) free(p->valor);
					p->clave = NULL;
					p->valor = NULL;
					alista->siguiente = NULL; // que el nodo anterior no apunte mas a este nodo que voy a borrar
					free(p); 
					return;
				}
			} else {
				// tiene nodo siguiente
				if ( p == lista ) {
					// es el primer elemento de la lista y tiene siguiente
					if ( p->clave ) free(p->clave);
					if ( p->valor ) free(p->valor);
					// copio el siguiente nodo en este y borro el siguiente
					clave_valor	*tmp = p->siguiente;
					memcpy(p,tmp,sizeof(clave_valor));
					free(tmp);
					return;
				} else {
					// no es el primer elemento de la lista y tiene siguiente -> es un elemento intermedio de la lista
					// que el nodo anterior apunte al siguiente de este nodo y borro este nodo
					alista->siguiente = p->siguiente;
					if ( p->clave ) free(p->clave);
					if ( p->valor ) free(p->valor);
					p->clave = NULL;
					p->valor = NULL;
					free(p); 
					return;
				}
			}
		}
		alista = p;
		p = p->siguiente;
	}
	return;
}

// buscar
char *util_buscar(clave_valor *lista,char *clave) {
	if ( !lista || !clave ) return NULL;	
	while(lista) {
		if ( lista->clave && strcmp(lista->clave,clave) == 0 ) return lista->valor;
		lista = lista->siguiente;
	}
	return NULL;
}

char *util_buscar_case(clave_valor *lista,char *clave) {
	if ( !lista || !clave ) return NULL;
	while(lista) {
		if ( lista->clave && strcasecmp(lista->clave,clave) == 0 ) return lista->valor;
		lista = lista->siguiente;
	}
	return NULL;
}

char *util_buscar_default(clave_valor *lista,char *clave,char *defecto) {
	char *p = util_buscar(lista,clave);
	return (p ? p : defecto);
}
char *util_buscar_case_default(clave_valor *lista,char *clave,char *defecto) {
	char *p = util_buscar_case(lista,clave);
	return (p ? p : defecto);
}


void util_listar(clave_valor *lista) {
	if ( !lista ) return;
	while(lista) {
		printf("clave [%s]=[%s]\n",lista->clave,lista->valor);
		lista = lista->siguiente;
	}
}

//parsing
peticion *util_peticion(char *buffer,clave_valor *param) {
	peticion *pet = (peticion *) malloc(sizeof(peticion));
	memset(pet,0,sizeof(peticion));
	int i=0,z;
	//metodo
	while(i < 32 && *(buffer+i) && *(buffer+i) != ' ') { pet->metodo[i] = toupper(buffer[i]);i++; }
	//espacio
	while(*(buffer+i) && *(buffer+i) == ' ') i++;
	//recurso solicitado
	z=0;
	while(z < 1024 && *(buffer+i) && *(buffer+i) != ' ') { pet->recurso[z] = buffer[i];i++;z++; }
	//espacio
	while(*(buffer+i) && *(buffer+i) == ' ') i++;
	//protocolo/version
	//protocolo
	z=0;
	while(z < 16 && *(buffer+i) && *(buffer+i) != '/') { pet->protocolo[z] = buffer[i];i++;z++; }
	// /
	while(*(buffer+i) && *(buffer+i) == '/') i++;
	//version
	z=0;
	while(z < 16 && *(buffer+i) && *(buffer+i) != '\r' && *(buffer+i) != '\n') { pet->version[z] = buffer[i];i++;z++; }
	// \r\n
	while(*(buffer+i) && (*(buffer+i) == '\r' || *(buffer+i) == '\n')) i++;
	// clave: valor\r\n ...\r\n\r\n
	clave_valor *lst = util_nuevo();
	// proceso cabeceras
	do {
		char clave[1024];
		char valor[1024];
		memset(clave,0,1024);
		memset(valor,0,1024);
		// clave:
		z=0;
		while(z < 1024 && *(buffer+i) && *(buffer+i) != ':') { clave[z] = buffer[i];i++;z++; }
		i++; // :
		// espacio
		while(*(buffer+i) && *(buffer+i) == ' ') i++;
		// valor \r\n
		z=0;
		while(z < 1024 && *(buffer+i) && *(buffer+i) != '\r' && *(buffer+i) != '\n' ) { valor[z] = buffer[i];i++;z++; }
		util_agregar(lst,clave,valor);
	printf("util_peticion(): [%s]=[%s]\n",clave,valor);
		i+=2; // salta supuesto \r\n
	} while(*(buffer+i) && *(buffer+i) != '\r' && *(buffer+i) != '\n');
	
	// no hay mas cabeceras
	pet->cabecera = lst;
	char *largo = util_buscar_case(lst,"Content-Length");
	if ( largo != NULL ) pet->largo = atol(largo);
	else pet->largo = 0L;
	if ( pet->largo ) {
		i+=2; // salta otro supuesto \r\n
		pet->cuerpo = malloc(pet->largo+1);
		memset(pet->cuerpo,0,pet->largo+1);
		z=0;
		while(z <= pet->largo && *(buffer+i) ) { memcpy(pet->cuerpo+z,buffer+i,1);i++;z++; }		
	}
	// estimo, calculo ubicacion fisica del recurso solicitado
	if ( strcmp(pet->recurso,"/") == 0 ) strcpy(pet->recurso_fisico,util_buscar(param,"server.pagina"));
	else {
		// se solicito un nombre de recurso
		// corto recurso antes de indicar puerto o parametros
		z=0;
		while(z < 1024 && pet->recurso[z] && pet->recurso[z] != '?' && pet->recurso[z] != ':') { pet->recurso_fisico[z] = pet->recurso[z]; z++; }
		pet->recurso_fisico[z]='\0';
	
		if ( util_termina_con(pet->recurso_fisico,util_buscar(param,"server.cgi.extension")) ) {
			char tmp[1024];
			strcpy(tmp,util_buscar(param,"server.directorio.cgi"));
			strcat(tmp,pet->recurso_fisico);
			strcpy(pet->recurso_fisico,tmp);
		} else {
			// era un recurso no CGI, si comienza con / necesito que comience con ./ porque el servidor
			// esta parado en directorio [server.directorio]
			if ( pet->recurso_fisico[0] == '/' ) {
				char tmp[1024];
				strcpy(tmp,".");
				strcat(tmp,pet->recurso_fisico);
				strcpy(pet->recurso_fisico,tmp);
			}
		}
		
	}
  printf("metodo: [%s] recurso: [%s] recurso fisico: [%s] protocolo: [%s] vesion: [%s]\n",pet->metodo,pet->recurso,pet->recurso_fisico,pet->protocolo,pet->version);
  printf("largo: [%ld]\n",pet->largo);
  printf("cabeceras:\n");
  util_listar(pet->cabecera);
  printf("cuerpo:\n");
  for(z=0;z<pet->largo;z++) {
	  char c;
	  memcpy(&c,pet->cuerpo+z,1);
	  printf("%c",c);
  }
  printf("\n");
	return pet;
}

respuesta *util_respuesta() {
	respuesta *res = (respuesta *) malloc(sizeof(respuesta));
	memset(res,0,sizeof(respuesta));
	strcpy(res->protocolo,"HTTP");
	strcpy(res->version,"1.1");
	res->estado = 200;
	strcpy(res->mensaje,"OK");
	res->cabecera = util_nuevo();
	util_agregar(res->cabecera,"Server","grchere web server 1.0");
	util_agregar(res->cabecera,"Connection","close"); // ATENCION!! no olvidar esto ni escribir mal la cabecera!
	char timebuf[128];
	util_fecha_hora(timebuf,128);
	util_agregar(res->cabecera,"Date",timebuf);
	return res;
}

void util_procesar(int sd,clave_valor *param,clave_valor *mime,peticion *pet,respuesta *res) {
	char content[50],content2[30];
	snprintf(content,50,"CONTENT_LENGTH=%ld",pet->largo);
	snprintf(content2,30,"SERVER_PORT=%s",util_buscar(param,"server.puerto"));

	if ( strcmp(pet->metodo,"GET") == 0 ) {
		printf("util_procesar(): GET\n");
		if ( !util_archivo_existe(pet->recurso_fisico) ) {
			util_cargar_archivo_noesta(param,mime,res);
		} else { // el archivo existe
			if ( util_termina_con(pet->recurso_fisico,util_buscar(param,"server.cgi.extension")) ) {
				// es un proceso CGI GET -> el string cgi se encuentra en variable QUERY_STRING
				//                       -> el query string viene en pet->recurso luego de ?
				printf("util_procesar(): GET, ejecuto cgi en metodo GET\n");
				
/********************************************************************/	
				int reclen = strlen(pet->recurso),ii=0,zz;
				char *qry = (char *) malloc(reclen+20);
				memset(qry,0,reclen+20);
				strcpy(qry,"QUERY_STRING=");
				while(ii < reclen && pet->recurso[ii] && pet->recurso[ii] != '?') ii++;
				ii++; //  salto ?
				zz=strlen(qry);
				while(ii < reclen && pet->recurso[ii] ) { qry[zz]=pet->recurso[ii];ii++;zz++; }
				
				printf("util_procesar(): GET, QUERY_STRING=[%s]\n",qry);
				
				// ATENCION!! faltan agregar mas variables para cumplir con la especificacion CGI
				char *envp[] = { "REQUEST_METHOD=GET", qry, content, "SERVER_SOFTWARE=grchere web server 1.0", 
					"SERVER_NAME=127.0.0.1", "GATEWAY_INTERFACE=CGI/1.0", "SERVER_PROTOCOL=HTTP/1.1",
					content2, NULL };

				char *tests[] = { pet->recurso_fisico, NULL };

				int fd2[2]; 
				pipe(fd2);
				pid_t pid = fork();
				if (pid == 0) { // proceso hijo
					close(fd2[0]);
					dup2(fd2[1], STDOUT_FILENO); // dup2(fd2[1],1);
					errno=0;
					execvpe(tests[0], tests, envp);
					fprintf(stdout, "Content-Type: text/html\r\n\r\n<!DOCTYPE html><body><p>Error [%d,%s] ejecutando [%s], metodo GET</p></body></html>\n",errno,strerror(errno),tests[0]);
					exit(-1);
				} else { // proceso padre
					close(fd2[1]);

					/* Leo lineas de salida del comando */
					// leo la salida
					int tamanio=0;
					char *buffer = util_leer_salida(fd2[0],&tamanio);
					// en buffer tengo todo lo devuelto por el programa CGI
					// cierro archivos abiertos
					close(fd2[0]);
					
					util_procesar_cabecera(res,buffer,tamanio,"GET");

					// ya no necesito mas a buffer
					printf("util_procesar(): CGI,GET libero buffer\n");
					free(buffer);
					free(qry);
					printf("util_procesar(): CGI,GET espero por finalizacion de proceso cgi\n");
					wait(0);
					printf("util_procesar(): CGI,GET finalizo proceso cgi\n");
				}	

/********************************************************************/					
				
			} else {
				// asumo que es un archivo comun a descargar
				if ( !util_cargar_archivo(res,mime,pet->recurso_fisico) ) {
					// se supone que no puede salir por aca, puede ser falta de permisos sobre el archivo
					printf("util_procesar(): GET, cargar_archivo_noesta\n");
					util_cargar_archivo_noesta(param,mime,res);
				}
			}
		}
	} else if ( strcmp(pet->metodo,"POST") == 0 ) {
		printf("util_procesar(): POST\n");
		if ( !util_archivo_existe(pet->recurso_fisico) ) {
			util_cargar_archivo_noesta(param,mime,res);
		} else {
			if ( util_termina_con(pet->recurso_fisico,util_buscar(param,"server.cgi.extension")) ) {
				// es un proceso CGI POST -> el string se ingresa por teclado
				printf("util_procesar(): POST archivo cgi [%s] a ejecutar!\n",pet->recurso_fisico);
				
/********************************************************************/				
				// metodo POST pasa parametros por stdin
				// ATENCION!! faltan agregar mas variables para cumplir con la especificacion CGI
				char *envp[] = { "REQUEST_METHOD=POST", content, "SERVER_SOFTWARE=grchere web server 1.0", 
					"SERVER_NAME=127.0.0.1", "GATEWAY_INTERFACE=CGI/1.0", "SERVER_PROTOCOL=HTTP/1.1",
					content2, NULL };
				char *tests[] = { pet->recurso_fisico, NULL };
				
				int fd1[2],fd2[2]; 
				pipe(fd1);
				pipe(fd2);
				pid_t pid = fork();
				if (pid == 0) { // proceso hijo
					close(fd1[1]);
					close(fd2[0]);
					dup2(fd1[0], STDIN_FILENO);  // dup2(fd1[0],0);
					dup2(fd2[1], STDOUT_FILENO); // dup2(fd2[1],1);
					errno=0;
					execvpe(tests[0], tests, envp);
					fprintf(stdout, "Content-Type: text/html\r\n\r\n<!DOCTYPE html><body><p>Error [%d,%s] ejecutando [%s], metodo POST</p></body></html>\n",errno,strerror(errno),tests[0]);
					exit(-1);
				} else { // proceso padre
					close(fd1[0]);
					close(fd2[1]);

					/* Escribo entrada */
					write(fd1[1],pet->cuerpo,pet->largo); // escribo entrada

					/* Leo lineas de salida del comando */
					// leo la salida
					int tamanio=0;
					char *buffer = util_leer_salida(fd2[0],&tamanio);
					// en buffer tengo todo lo devuelto por el programa CGI
					// cierro archivos abiertos
					close(fd2[0]);
					close(fd1[1]);

					util_procesar_cabecera(res,buffer,tamanio,"POST");

					// ya no necesito mas a buffer
					printf("util_procesar(): CGI,POST libero buffer\n");
					free(buffer);
					printf("util_procesar(): CGI,POST espero por finalizacion de proceso cgi\n");
					wait(0);
					printf("util_procesar(): CGI,POST finalizo proceso cgi\n");
				}	
				
				
/********************************************************************/				
				
			} else {
				//envio post a un archivo que no es CGI ! -> no implementado por ahora
				printf("util_procesar(): uso POST para pedir recurso no CGI\n");
				if ( !util_cargar_archivo(res,mime,pet->recurso_fisico) ) {
					printf("util_procesar(): POST, cargar_archivo_noesta (2)\n");
					util_cargar_archivo_noesta(param,mime,res);
				} 
			}
		}
	} else {
		res->estado = 400; //bad request
		strcpy(res->mensaje,"Peticion Erronea");
	}
	printf("util_procesar(): fin\n");
}

void util_cargar_archivo_noesta(clave_valor *param,clave_valor *mime,respuesta *res) {
	util_cargar_archivo(res,mime,util_buscar(param,"server.pagina.no.encontrada"));
	res->estado = 404; // not found
	strcpy(res->mensaje,"Archivo no encontrado");
}

void util_borrar_pet(peticion *p) {
	if ( p->cabecera ) { util_borrar(p->cabecera);p->cabecera = NULL; }
	if ( p->cuerpo ) { free(p->cuerpo);p->cuerpo = NULL; p->largo = 0; }
}
void util_borrar_res(respuesta *r) {
	if ( r->cabecera ) { util_borrar(r->cabecera);r->cabecera = NULL; }
	if ( r->cuerpo ) { free(r->cuerpo);r->cuerpo = NULL; r->largo = 0; }
}

// carga archivo en res, siempre y cuando exista
int util_cargar_archivo(respuesta *res,clave_valor *mime,char *archivo) {
	//printf("util_cargar_archivo(): util_borrar_clave() Content-Length res->cabecera=%p\n",res->cabecera);
	//util_borrar_clave(res->cabecera,"Content-Length");
	//printf("util_cargar_archivo(): util_borrar_clave() Content-Type res->cabecera=%p\n",res->cabecera);
	//util_borrar_clave(res->cabecera,"Content-Type");
	printf("util_cargar_archivo(): antes de fopen() res->cabecera=%p\n",res->cabecera);
	char tarchivo[256];
	// si el nombre del archivo comienza con /, ignoro la barra inicial
	if ( *archivo == '/' ) strcpy(tarchivo,archivo+1);
	else strcpy(tarchivo,archivo);
	printf("util_cargar_archivo(): tarchivo=[%s]\n",tarchivo);
	FILE *f = fopen(tarchivo,"rb");
	if ( f == NULL ) { 
		res->largo = 0L;
		if ( res->cuerpo ) free(res->cuerpo);
		res->cuerpo = NULL;
		return 0;
	}
	fseek(f,0L,SEEK_END);
	res->largo = ftell(f);
	fseek(f,0L,SEEK_SET);
	res->cuerpo = malloc(res->largo);
	fread(res->cuerpo,res->largo,1,f);
	fclose(f);
	char tmp[256];
	snprintf(tmp,256,"%ld",res->largo);
	printf("util_cargar_archivo(): util_agregar_clave() Content-Length: %s res->cabecera=%p\n",tmp,res->cabecera);
	util_agregar(res->cabecera,"Content-Length",tmp);
	// agrego cabecera de fecha-hora de ultima modificacion del archivo
	util_archivo_fhmodif(tmp,256,tarchivo);
	util_agregar(res->cabecera,"Last-Modified",tmp);
	// analizo el mime del archivo, acorde con su extension
	clave_valor *p = mime;
	int encontre=0;
	printf("util_cargar_archivo(): chequeo mime type\n");
	while(p) {
		if ( util_termina_con(archivo,p->clave) ) {
			util_agregar(res->cabecera,"Content-Type",p->valor);
			encontre=1;
			break;
		}
		p = p->siguiente;
	}
	printf("util_cargar_archivo(): fin chequeo mime type, encontre=%d\n",encontre);
	if ( !encontre ) {
		util_agregar(res->cabecera,"Content-Type","application/octet-stream");
	}
	printf("util_cargar_archivo(): fin\n");
	return 1;
}

// devuelve verdad si str termina con suffix
int util_termina_con(const char *str, const char *suffix) {
	if ( !str ) return 0;
	if ( !suffix ) return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr) return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

// archivo tiene registros con formato: clave\tvalor\n
void util_cargar_mime(clave_valor *to,char *archivo) {
	FILE *f = fopen(archivo,"rb");
	if ( f == NULL ) return;
	char linea[1024];
	char clave[1024];
	char valor[1024];
	int i,z;
	while(fgets(linea,1024,f)) {
		i=0;
		memset(clave,0,1024);
		memset(valor,0,1024);
		//clave
		while(i < 1024 && linea[i] && linea[i] != '\t') { clave[i]=linea[i];i++; }
		i++;
		//valor
		z=0;
		while(i < 1024 && linea[i] && linea[i] != '\r' && linea[i] != '\n') { valor[z]=linea[i];i++;z++; }
		util_agregar(to,clave,valor);
//printf("util_cargar_mime(): clave [%s] valor [%s]\n",clave,valor);
		memset(linea,0,1024);
	}
	fclose(f);
}

void util_enviar_respuesta(int sd,respuesta *res) {
	char registro[1024];
	// falta la fecha-hora en formato, ej: Fri, 31 Dec 2003 23:59:59 GMT
	snprintf(registro,1024,"%s/%s %d %s\r\n",res->protocolo,res->version,res->estado,res->mensaje);
printf("util_enviar_respuesta(): [%s]\n",registro);		
	write(sd,registro,strlen(registro));
	// grabo cabeceras http
	clave_valor *p = res->cabecera;
printf("util_enviar_respuesta(): inicio envio de cabeceras\n");	
	while(p && p->clave && p->valor ) {
		snprintf(registro,1024,"%s: %s\r\n",p->clave,p->valor);
printf("util_enviar_respuesta(): [%s]\n",registro);		
		write(sd,registro,strlen(registro));
		p = p->siguiente;
	}
printf("util_enviar_respuesta(): fin envio de cabeceras\n");	
	strcpy(registro,"\r\n");
printf("util_enviar_respuesta(): registro: [%s] len registro=[%ld] sd=%d\n",registro,strlen(registro),sd);
	errno=0;
	int rc = write(sd,registro,strlen(registro)); // mata proceso cliente
printf("util_enviar_respuesta(): rc=%d errno=%d strerrno=[%s]\n",rc,errno,strerror(errno));
printf("util_enviar_respuesta(): envie retorno carro\\line feed\n");	
	if ( res->cuerpo ) {
		printf("util_enviar_respuesta(): envio cuerpo\n");	
		write(sd,res->cuerpo,res->largo);
	}
	printf("util_enviar_respuesta(): fin!\n");	
}

int util_archivo_existe(char *archivo) {
    struct stat buffer;
    return stat(archivo,&buffer) == 0 ? 1 : 0;
}

// devuelve la fecha-hora de ultima modificacion de archivo en el formato de fecha que requiere http
void util_archivo_fhmodif(char *to,int toLargo,char *archivo) {
    struct stat buffer;
    int rc = stat(archivo,&buffer);
    if ( rc == 0 ) { // todo ok, archivo existe, struct stat cargada
		struct tm tm;
		strftime(to, toLargo, "%a, %d %b %Y %H:%M:%S GMT", gmtime_r(&buffer.st_mtim.tv_sec, &tm));
	} else {
		// rayos! el archivo no existe!, devuelvo fecha-hora actual
		// devuelvo la fecha-hora actual ! se supone que no debo salir por aca!
		util_fecha_hora(to,toLargo);
	}
}

// devuelve la fecha-hora actual en el formato de fecha que requiere http
// toLargo = 128 es mas que suficiente para guardar la fecha
void util_fecha_hora(char *to,int toLargo) {
	time_t now = time(NULL);
	strftime(to, toLargo, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));	
}


// copia las cabeceras que devuelve el proceso CGI en res
void util_procesar_cabecera(respuesta *res,char *buffer,int tamanio,char *metodo) {
	printf("util_procesar_cabecera(): CGI,%s cgi buffer[%s]\n",metodo,buffer);
	// proceso cabeceras y luego el cuerpo
	int i=0,z;
	do {
		char clave[1024];
		char valor[1024];
		memset(clave,0,1024);
		memset(valor,0,1024);
		// clave:
		z=0;
		while(z < 1024 && *(buffer+i) && *(buffer+i) != ':') { clave[z] = buffer[i];i++;z++; }
		i++; // :
		// espacio
		while(*(buffer+i) && *(buffer+i) == ' ') i++;
		// valor \r\n
		z=0;
		while(z < 1024 && *(buffer+i) && *(buffer+i) != '\r' && *(buffer+i) != '\n' ) { valor[z] = buffer[i];i++;z++; }
		util_agregar(res->cabecera,clave,valor);
	printf("util_procesar_cabecera(): CGI,%s cabecera [%s]=[%s]\n",metodo,clave,valor);
		i+=2; // salta supuesto \r\n
	} while(*(buffer+i) && *(buffer+i) != '\r' && *(buffer+i) != '\n');
	i+=2; // salta otros supuestos \r\n
	// voy calculando el largo del body a medida que lo copio
	res->largo=0L;
	long lcuerpo = tamanio-i+2;
	res->cuerpo = malloc(lcuerpo);
	memset(res->cuerpo,0,lcuerpo);
	while(res->largo < lcuerpo && *(buffer+i) ) { memcpy(res->cuerpo+res->largo,buffer+i,1);i++;res->largo++; }
	// agrego cabecera Content-Length
	char content99[50];
	snprintf(content99,50,"%ld",res->largo);
	util_agregar(res->cabecera,"Content-Length",content99);
	// muestro cuerpo a devolver
	printf("util_procesar_cabecera(): CGI,%s largo=[%ld] cuerpo[",metodo,res->largo);
	lcuerpo=0L;
	while(lcuerpo < res->largo) {
		char c;
		memcpy(&c,res->cuerpo+lcuerpo,1);
		printf("%c",c);
		lcuerpo++;
	}
	printf("]\n");
	printf("util_procesar_cabecera(): fin!\n");
}

// lee la salida del comando CGI y devuelve el buffer de memoria dinamica con la salida, en tamanio devuelve el tamanio del buffer
char *util_leer_salida(int fd,int *tamanio) {
	int lei=0,i=0,bloque=1024;
	*tamanio=bloque;
	char *buffer = (char *) malloc(bloque);
	if ( buffer == NULL ) {
		*tamanio=0;
		printf("util_leer_salida(): Error en asignacion de bloque de memoria!\n");
		return NULL;
	}
	memset(buffer,0,bloque);
	
	/* Leo lineas de salida del comando */
	// leo la salida
	do {
		errno=0;
		lei = read(fd,buffer+i,bloque);
		if ( lei == -1 ) {
			printf("util_leer_salida(): Error [%d,%s] leyendo de file descriptor [%d]\n",errno,strerror(errno),fd);
			return buffer;
		}
		if ( lei >= bloque ) {
			*tamanio+=bloque;
			buffer = (char *) realloc(buffer,*tamanio);
			memset(buffer+(*tamanio)-bloque,0,bloque);
		}
		i+=lei;
	} while(lei >= bloque);
	return buffer;
}

// parametros
clave_valor *ws_parametros_archivo(char *archivo) {
	char *argv[2] = { archivo, NULL }; // ojo! archivo no incluye .conf solo nombre de app
	return ws_parametros(1,argv);
}

// parametros del servidor
clave_valor *ws_parametros(int argc,char **argv) {
	// archivo de configuracion
	char config[256];
	snprintf(config,256,"%s.conf",argv[0]);
	printf("ws_parametros: inicio lectura configuracion [%s]...\n",config);
	FILE *f = fopen(config,"r");
	if ( f == NULL ) return NULL;
	char linea[1024];
	int i,ii;
	clave_valor *param = util_nuevo();
	while(fgets(linea,1024,f)) {
		i=0;
		if ( strlen(linea) <= 1 ) continue;
		while(i < 1024 && (linea[i] && (linea[i] == ' ' || linea[i] == '\r' || linea[i] == '\n' || linea[i] == '\t'))) i++;
		if ( linea[i] == '#' ) continue;
		// linea que no es comentario
		char clave[1024];
		char valor[1024];
		//obtengo clave
		ii=0;
		while(i<1024 && linea[i] && linea[i] != ' ' && linea[i] != '=') clave[ii++]=linea[i++];
		clave[ii]='\0';
		while(i<1024 && linea[i] && (linea[i] == ' ' || linea[i] == '=')) i++;
		//otengo valor
		ii=0;
		while(i<1024 && linea[i] && linea[i] != '\n' && linea[i] != '\r' ) valor[ii++]=linea[i++];
		valor[ii]='\0';
		//printf("ws_parametros(): [%s]=[%s]\n",clave,valor);
		util_agregar(param,clave,valor);
	}
	fclose(f);
	util_listar(param);
	return param;
}

